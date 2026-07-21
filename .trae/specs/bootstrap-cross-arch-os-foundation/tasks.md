# Tasks
- [x] Task 1: 明确第一阶段系统边界与目录骨架
  - [x] SubTask 1.1: 固化目录分层，至少覆盖 `boot`、`arch`、`platform`、`kernel`、`drivers`、`services`、`apps`
  - [x] SubTask 1.2: 选定 `x86_64 + QEMU` 为首发目标，并固定首发启动方案与构建入口
  - [x] SubTask 1.3: 在文档或实现中明确第一阶段不包含的高复杂度能力，防止范围失控

- [x] Task 2: 打通 x86_64 最小启动链路
  - [x] SubTask 2.1: 完成 bootloader 对接或最小引导流程
  - [x] SubTask 2.2: 成功进入内核入口并输出串口或屏幕启动日志
  - [x] SubTask 2.3: 提供可重复执行的 QEMU 启动脚本或命令

- [x] Task 3: 建立最小内核核心能力
  - [x] SubTask 3.1: 提供基础内存分配能力，至少满足早期启动和内核对象分配
  - [x] SubTask 3.2: 提供异常/中断与时钟基础设施
  - [x] SubTask 3.3: 提供最小任务调度或事件循环，让系统具备持续运行和驱动 GUI 的能力

- [x] Task 4: 打通基础显示与输入链路
  - [x] SubTask 4.1: 实现 framebuffer 或等价简单显示输出
  - [x] SubTask 4.2: 接入至少一种基础输入来源，优先键盘
  - [x] SubTask 4.3: 保证显示和输入接口不直接绑死在某个具体平台实现上

- [x] Task 5: 集成 GUI MVP
  - [x] SubTask 5.1: 接入 `LVGL` 或等价轻量 GUI 方案
  - [x] SubTask 5.2: 提供一个基础 GUI demo，例如桌面占位页、启动页或简单设置页
  - [x] SubTask 5.3: 验证 GUI demo 能在 `x86_64 + QEMU` 下稳定启动并响应基础输入

- [x] Task 6: 为 ARM64 与 RISC-V 移植预留抽象边界
  - [x] SubTask 6.1: 定义 `arch` 与 `platform` 的最小接口面
  - [x] SubTask 6.2: 标注 x86_64 实现中哪些部分未来需要替换为 `ARM64 virt` 与 `RISC-V virt` 实现
  - [x] SubTask 6.3: 准备第二阶段移植验证标准，确保不是重写一套新系统

- [x] Task 7: 定义 MCU 子集路线
  - [x] SubTask 7.1: 明确 MCU 版本复用哪些核心抽象，裁剪哪些系统能力
  - [x] SubTask 7.2: 选定首个 MCU 参考目标，例如某个 Cortex-M 开发板
  - [x] SubTask 7.3: 规划 MCU 上的 GUI、存储和调度最小路径，避免与 MPU 版本混淆

- [x] Task 8: 建立验证与演示基线
  - [x] SubTask 8.1: 为每个阶段定义最小可见成果
  - [x] SubTask 8.2: 规定启动、显示、输入、GUI、跨架构移植的验证方式
  - [x] SubTask 8.3: 保证后续每次迭代都能通过脚本或清单复验，而不是靠口头判断

- [x] Task 9: 修复核心分层边界泄漏并对齐当前规格说明
  - [x] SubTask 9.1: 让 `src/kernel/main.c` 与 `src/kernel/event_loop.c` 通过 `tinyos_arch_current()` 获取 tick、中断与 idle 能力，移除直接 `hlt` 和 `interrupts_*` 依赖
  - [x] SubTask 9.2: 将 `src/kernel/interrupts.c` 中 `IDT/PIC/PIT` 与 `port_io` 的 `x86_64` 专属实现收敛到 `arch/x86_64/` 边界，避免通用内核继续承载架构细节
  - [x] SubTask 9.3: 更新 `README.md`、`docs/release-0-scope.md` 等说明文档，明确当前阶段已经进入最小启动链路、基础内核、显示输入与 GUI MVP 基线，消除旧范围描述造成的歧义
  - [x] SubTask 9.4: 复验 `Task6` 抽象边界，确保后续 `ARM64 virt` 与 `RISC-V virt` 移植主要落在 `boot/`、`arch/`、`platform/`

- [x] Task 10: 补齐本地 QEMU 环境并完成运行时基线验证
  - [x] SubTask 10.1: 在当前开发机安装或补齐 `qemu-system-x86_64` 运行时依赖
  - [x] SubTask 10.2: 执行 `make check-baseline`，完成镜像构建、QEMU 启动和串口日志校验
  - [x] SubTask 10.3: 如运行时验证暴露脚本或环境问题，进行最小修复并复验通过

- [x] Task 11: 将 x86 启动链收敛为 `UEFI-first` 启动介质
  - [x] SubTask 11.1: 收敛并固定首批支持的启动边界，明确 `QEMU + OVMF + raw disk image` 路线
  - [x] SubTask 11.2: 将镜像布局调整为 `ESP + BOOTX64.EFI + KERNEL.BIN`，避免继续依赖旧的 `legacy BIOS` 分段引导产物
  - [x] SubTask 11.3: 提供面向 `UEFI` 虚拟机与后续 U 盘写入的统一镜像产物和清晰命名，避免把开发中间产物误当可启动介质

- [x] Task 12: 建立首批 `UEFI` 虚拟机启动兼容基线
  - [x] SubTask 12.1: 明确首批受支持虚拟机配置为 `QEMU + OVMF`
  - [x] SubTask 12.2: 补充 `debugcon`、串口日志与屏幕截图验证记录，证明 `UEFI` loader、kernel handoff 与 framebuffer demo 已跑通
  - [x] SubTask 12.3: 记录当前不承诺支持的启动方式，例如 `legacy BIOS`、非 `OVMF` 虚拟机配置和未验证真机环境

- [ ] Task 13: 建立真机 `UEFI` U 盘启动路径
  - [ ] SubTask 13.1: 提供将当前原始镜像写入 U 盘的最小操作说明与数据风险提示
  - [ ] SubTask 13.2: 明确真机启动前提条件，例如 `x86_64 UEFI`、可从外部介质启动、以及串口/屏幕观察点
  - [ ] SubTask 13.3: 设计真机 `UEFI` 启动 smoke test 记录方式，至少能记录成功进入内核或失败停留阶段

- [ ] Task 14: 将启动介质验证纳入统一验收基线
  - [ ] SubTask 14.1: 扩展当前验证文档，覆盖 `QEMU + OVMF`、后续真机 U 盘启动和各自的验收口径
  - [ ] SubTask 14.2: 为镜像产物、启动方式、支持边界和排障入口补充复验清单
  - [ ] SubTask 14.3: 保证后续每次迭代都能区分“`QEMU + OVMF` 可跑”“其它虚拟机已验证”“真机 U 盘可跑”这三类状态，而不是混成一句“能启动”

# Task Dependencies
- [Task 2] depends on [Task 1]
- [Task 3] depends on [Task 2]
- [Task 4] depends on [Task 2] and [Task 3]
- [Task 5] depends on [Task 4]
- [Task 6] depends on [Task 1] and [Task 3]
- [Task 7] depends on [Task 1] and [Task 6]
- [Task 8] depends on [Task 2], [Task 5], [Task 6], and [Task 7]
- [Task 9] depends on [Task 6] and [Task 8]
- [Task 10] depends on [Task 8] and [Task 9]
- [Task 11] depends on [Task 2] and [Task 10]
- [Task 12] depends on [Task 11]
- [Task 13] depends on [Task 11]
- [Task 14] depends on [Task 12] and [Task 13]
