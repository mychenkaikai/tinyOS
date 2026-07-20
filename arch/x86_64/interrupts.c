#include "interrupts.h"

#include "tinyos/console.h"
#include "tinyos/input.h"
#include "tinyos/port_io.h"

#define IDT_ENTRIES 256u
#define IDT_FLAG_INTERRUPT_GATE 0x8Eu
#define KERNEL_CODE_SELECTOR 0x08u
#define PIC1_COMMAND 0x20u
#define PIC1_DATA 0x21u
#define PIC2_COMMAND 0xA0u
#define PIC2_DATA 0xA1u
#define PIC_EOI 0x20u
#define PIT_COMMAND 0x43u
#define PIT_CHANNEL0 0x40u
#define PIT_BASE_FREQUENCY 1193182u
#define PIT_DEFAULT_TICK_HZ 100u
#define IRQ_BASE_VECTOR 32u

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct interrupt_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
};

#define DECLARE_ISR(vector) extern void interrupt_stub_##vector(void)

DECLARE_ISR(0);
DECLARE_ISR(1);
DECLARE_ISR(2);
DECLARE_ISR(3);
DECLARE_ISR(4);
DECLARE_ISR(5);
DECLARE_ISR(6);
DECLARE_ISR(7);
DECLARE_ISR(8);
DECLARE_ISR(9);
DECLARE_ISR(10);
DECLARE_ISR(11);
DECLARE_ISR(12);
DECLARE_ISR(13);
DECLARE_ISR(14);
DECLARE_ISR(15);
DECLARE_ISR(16);
DECLARE_ISR(17);
DECLARE_ISR(18);
DECLARE_ISR(19);
DECLARE_ISR(20);
DECLARE_ISR(21);
DECLARE_ISR(22);
DECLARE_ISR(23);
DECLARE_ISR(24);
DECLARE_ISR(25);
DECLARE_ISR(26);
DECLARE_ISR(27);
DECLARE_ISR(28);
DECLARE_ISR(29);
DECLARE_ISR(30);
DECLARE_ISR(31);
DECLARE_ISR(32);
DECLARE_ISR(33);
DECLARE_ISR(34);
DECLARE_ISR(35);
DECLARE_ISR(36);
DECLARE_ISR(37);
DECLARE_ISR(38);
DECLARE_ISR(39);
DECLARE_ISR(40);
DECLARE_ISR(41);
DECLARE_ISR(42);
DECLARE_ISR(43);
DECLARE_ISR(44);
DECLARE_ISR(45);
DECLARE_ISR(46);
DECLARE_ISR(47);

extern void x86_64_interrupt_stub_default(void);

static struct idt_entry g_idt[IDT_ENTRIES];
static volatile uint64_t g_timer_ticks = 0;
static bool g_interrupts_ready = false;

static void io_wait(void) {
    outb(0x80u, 0u);
}

static void set_idt_entry(uint8_t vector, void (*handler)(void)) {
    uint64_t address = (uint64_t)handler;

    g_idt[vector].offset_low = (uint16_t)(address & 0xFFFFu);
    g_idt[vector].selector = KERNEL_CODE_SELECTOR;
    g_idt[vector].ist = 0u;
    g_idt[vector].type_attributes = IDT_FLAG_INTERRUPT_GATE;
    g_idt[vector].offset_mid = (uint16_t)((address >> 16) & 0xFFFFu);
    g_idt[vector].offset_high = (uint32_t)((address >> 32) & 0xFFFFFFFFu);
    g_idt[vector].zero = 0u;
}

static void load_idt(void) {
    struct idt_pointer idtr;

    idtr.limit = (uint16_t)(sizeof(g_idt) - 1u);
    idtr.base = (uint64_t)&g_idt[0];
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}

static void pic_remap(void) {
    uint8_t master_mask = inb(PIC1_DATA);
    uint8_t slave_mask = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11u);
    io_wait();
    outb(PIC2_COMMAND, 0x11u);
    io_wait();

    outb(PIC1_DATA, IRQ_BASE_VECTOR);
    io_wait();
    outb(PIC2_DATA, IRQ_BASE_VECTOR + 8u);
    io_wait();

    outb(PIC1_DATA, 0x04u);
    io_wait();
    outb(PIC2_DATA, 0x02u);
    io_wait();

    outb(PIC1_DATA, 0x01u);
    io_wait();
    outb(PIC2_DATA, 0x01u);
    io_wait();

    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

static void pic_set_masks(uint8_t master_mask, uint8_t slave_mask) {
    outb(PIC1_DATA, master_mask);
    outb(PIC2_DATA, slave_mask);
}

static void pic_send_eoi(uint8_t vector) {
    if (vector >= (IRQ_BASE_VECTOR + 8u)) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

static void pit_init(uint32_t frequency_hz) {
    uint16_t divisor;

    if ((frequency_hz == 0u) || (frequency_hz > PIT_BASE_FREQUENCY)) {
        frequency_hz = PIT_DEFAULT_TICK_HZ;
    }

    divisor = (uint16_t)(PIT_BASE_FREQUENCY / frequency_hz);
    if (divisor == 0u) {
        divisor = 1u;
    }

    outb(PIT_COMMAND, 0x36u);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFFu));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFFu));
}

static void install_idt_handlers(void) {
    uint32_t vector;

    for (vector = 0; vector < IDT_ENTRIES; ++vector) {
        set_idt_entry((uint8_t)vector, x86_64_interrupt_stub_default);
    }

#define INSTALL_ISR(vector) set_idt_entry((uint8_t)(vector), interrupt_stub_##vector)
    INSTALL_ISR(0);
    INSTALL_ISR(1);
    INSTALL_ISR(2);
    INSTALL_ISR(3);
    INSTALL_ISR(4);
    INSTALL_ISR(5);
    INSTALL_ISR(6);
    INSTALL_ISR(7);
    INSTALL_ISR(8);
    INSTALL_ISR(9);
    INSTALL_ISR(10);
    INSTALL_ISR(11);
    INSTALL_ISR(12);
    INSTALL_ISR(13);
    INSTALL_ISR(14);
    INSTALL_ISR(15);
    INSTALL_ISR(16);
    INSTALL_ISR(17);
    INSTALL_ISR(18);
    INSTALL_ISR(19);
    INSTALL_ISR(20);
    INSTALL_ISR(21);
    INSTALL_ISR(22);
    INSTALL_ISR(23);
    INSTALL_ISR(24);
    INSTALL_ISR(25);
    INSTALL_ISR(26);
    INSTALL_ISR(27);
    INSTALL_ISR(28);
    INSTALL_ISR(29);
    INSTALL_ISR(30);
    INSTALL_ISR(31);
    INSTALL_ISR(32);
    INSTALL_ISR(33);
    INSTALL_ISR(34);
    INSTALL_ISR(35);
    INSTALL_ISR(36);
    INSTALL_ISR(37);
    INSTALL_ISR(38);
    INSTALL_ISR(39);
    INSTALL_ISR(40);
    INSTALL_ISR(41);
    INSTALL_ISR(42);
    INSTALL_ISR(43);
    INSTALL_ISR(44);
    INSTALL_ISR(45);
    INSTALL_ISR(46);
    INSTALL_ISR(47);
#undef INSTALL_ISR
}

static void halt_forever(void) {
    x86_64_interrupts_disable();
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void handle_exception(const struct interrupt_frame *frame) {
    console_write("Unhandled exception vector=");
    console_write_u64(frame->vector);
    console_write(" error=");
    console_write_hex64(frame->error_code);
    console_write(" rip=");
    console_write_hex64(frame->rip);
    console_write_char('\n');
    halt_forever();
}

void x86_64_interrupt_dispatch(struct interrupt_frame *frame) {
    if (frame->vector < IRQ_BASE_VECTOR) {
        handle_exception(frame);
        return;
    }

    if (frame->vector == IRQ_BASE_VECTOR) {
        ++g_timer_ticks;
    }

    if (frame->vector == (IRQ_BASE_VECTOR + 1u)) {
        input_handle_platform_irq();
    }

    if (frame->vector < (IRQ_BASE_VECTOR + 16u)) {
        pic_send_eoi((uint8_t)frame->vector);
    }
}

void x86_64_interrupts_init(uint32_t tick_hz) {
    install_idt_handlers();
    load_idt();
    pic_remap();
    pic_set_masks(0xFCu, 0xFFu);
    pit_init(tick_hz);
    g_interrupts_ready = true;
}

void x86_64_interrupts_enable(void) {
    __asm__ volatile ("sti");
}

void x86_64_interrupts_disable(void) {
    __asm__ volatile ("cli");
}

uint64_t x86_64_interrupts_ticks(void) {
    return g_timer_ticks;
}

bool x86_64_interrupts_ready(void) {
    return g_interrupts_ready;
}
