# 跨架构个人操作系统基础骨架 Spec

## Why
当前只有方向性想法，还没有落成可执行的范围定义、阶段边界和验收标准。若不先收敛，项目很容易在启动链、架构移植、GUI、MCU 适配之间来回发散，最后什么都沾一点但没有一条链路真正跑通。

## What Changes
- 定义一个以 `x86_64 + QEMU` 为首发目标的最小操作系统基础骨架
- 将 `x86_64` 启动目标从“仅在 QEMU 中验证”提升为“先在 `QEMU + OVMF` 下完成 `UEFI-first` 启动闭环，并为后续 U 盘真机启动保留同一镜像形态”
- 约束第一阶段只打通 `启动 -> 内核入口 -> 基础日志 -> 基本内存管理 -> 定时器/任务 -> framebuffer GUI demo`
- 明确 `ARM64/RISC-V (MPU)` 为第二阶段移植目标，要求最大化复用内核核心抽象
- 明确 `ARM Cortex-M / RISC-V MCU` 为轻量子集目标，不要求与 MPU 版本具备相同系统能力
- 定义统一的目录分层、平台抽象边界和验证路径
- **BREAKING** 明确排除早期实现中的高复杂度目标，包括完整用户态、POSIX 兼容、SMP、GPU 驱动、完整网络栈和高性能优化

## Impact
- Affected specs: 启动链、UEFI 镜像格式、虚拟机兼容性、后续真机启动介质、架构抽象、内核核心、设备驱动、图形界面、跨架构移植流程
- Affected code: `boot/`、`arch/`、`platform/`、`kernel/`、`drivers/`、`services/`、`apps/`、构建脚本、ESP/磁盘镜像生成脚本、QEMU 启动脚本、后续真机启动文档

## ADDED Requirements
### Requirement: 以 x86_64 为起点的最小可运行系统
系统 SHALL 先在 `x86_64 + QEMU` 上提供最小可运行骨架，并以此作为后续跨架构移植的参考实现。

#### Scenario: 成功进入内核
- **WHEN** 用户使用标准构建命令生成镜像并启动 `QEMU x86_64`
- **THEN** 系统进入内核入口并输出可见的启动日志

#### Scenario: 成功提供最小图形输出
- **WHEN** 系统初始化完成基础显示设备
- **THEN** 用户可以看到 framebuffer 级别的图形输出或 GUI 占位界面

### Requirement: 提供可作为 UEFI 虚拟机启动盘的 x86 启动镜像
系统 SHALL 为 `x86_64` 生成一种可重复构建的 `UEFI` 原始磁盘镜像；当前阶段该镜像至少能在受支持的 `UEFI` 虚拟机中作为启动盘使用，并保持后续写入 U 盘进行真机验证的基本形态。

#### Scenario: UEFI 虚拟机启动镜像
- **WHEN** 用户将生成的镜像作为磁盘挂载到受支持的 `x86_64 UEFI` 虚拟机
- **THEN** 系统能够进入内核并输出与当前阶段一致的启动日志和界面

#### Scenario: 后续 U 盘镜像形态
- **WHEN** 开发者将当前镜像形态规划到后续真机阶段
- **THEN** 镜像布局不应依赖“只能被单一调试脚本识别”的专用格式，而应保持 `ESP + BOOTX64.EFI + KERNEL.BIN` 的可延展结构

### Requirement: 明确首批支持的固件与启动兼容边界
系统 SHALL 先明确并收敛首批支持的固件与硬件边界，优先打通 `UEFI + QEMU/OVMF + x86_64 raw disk image` 路线，而不是同时承诺 `legacy BIOS`、多种虚拟机平台和所有真机主板。

#### Scenario: 首批支持边界
- **WHEN** 开发者描述当前阶段的可启动范围
- **THEN** 规格必须明确首批目标至少包含 `QEMU + OVMF`，并定义后续真机 `UEFI` 启动的预期边界与暂不支持项

#### Scenario: 范围外启动方式
- **WHEN** 用户尝试在未承诺的 `legacy BIOS`、其它未验证虚拟机平台、或真机 `UEFI` 环境下运行镜像
- **THEN** 项目文档应明确这类场景属于后续阶段，不应被误表述为当前已支持

### Requirement: 分离架构抽象、平台适配和内核核心
系统 SHALL 将 CPU 架构相关逻辑、板级/虚拟平台初始化逻辑、以及可复用的内核核心逻辑分层组织，避免把平台细节硬编码进内核核心。

#### Scenario: 新平台接入
- **WHEN** 开发者新增一个 `ARM64` 或 `RISC-V` 虚拟平台
- **THEN** 主要改动集中在 `boot/`、`arch/`、`platform/` 层，而非大规模重写 `kernel/core`

### Requirement: 明确 MPU 与 MCU 的能力分层
系统 SHALL 将 `x86_64 / ARM64 / RISC-V virt` 视为 MPU 类目标，将 `Cortex-M / RISC-V MCU` 视为轻量子集目标，并允许两类平台在能力上分层实现。

#### Scenario: MCU 子集约束
- **WHEN** 系统面向 MCU 目标移植
- **THEN** 可以省略虚拟内存、用户态和复杂 VFS，但必须保留统一的任务、时间、内存分配、设备和 GUI 接口抽象

#### Scenario: MCU 首个参考板
- **WHEN** 项目进入首个 MCU 移植里程碑
- **THEN** SHALL 固定 `STM32F429I-DISC1` 作为首个参考板，优先验证 Cortex-M 轻量子集而不是同时铺开多块板卡

#### Scenario: MCU 最小服务路径
- **WHEN** 开发者规划 MCU GUI、存储与调度方案
- **THEN** SHALL 采用 `定时器 tick -> 共享 event loop -> LCD/GUI 单屏 demo -> 小型持久化设置存储` 的最小路径，而不是引入完整 RTOS 调度器、VFS 或桌面级图形栈

### Requirement: 第一阶段 GUI 采用简单显示路径
系统 SHALL 在第一阶段使用简单图形输出路径，优先采用 `framebuffer + LVGL` 或等价的轻量方式，不引入复杂 GPU 驱动依赖。

#### Scenario: GUI MVP
- **WHEN** 开发者完成第一阶段 GUI 集成
- **THEN** 系统至少能显示一个基础界面、处理基本输入事件，并运行一个简单 demo

### Requirement: 第一阶段功能边界可控
系统 SHALL 将第一阶段范围限制在最小内核和图形链路，不在首轮实现中引入高复杂度系统能力。

#### Scenario: 范围外能力
- **WHEN** 开发者评估首轮实现内容
- **THEN** 不应把完整进程隔离、动态链接、SMP、完整网络栈、复杂文件系统缓存和 GPU 驱动纳入第一阶段交付

### Requirement: 建立清晰的阶段性验证路径
系统 SHALL 为每一阶段定义可验证的里程碑，确保每个阶段都能形成用户可见的进展，而不是只停留在内部重构。

#### Scenario: 第一阶段验收
- **WHEN** 第一阶段完成
- **THEN** 用户能够在 `QEMU + OVMF` 中看到 `UEFI` loader 日志、进入内核入口，并在 GOP framebuffer 上看到基础 GUI/demo 输出

#### Scenario: 启动介质验收
- **WHEN** 当前阶段加入虚拟机与真机介质目标
- **THEN** 用户能够获得明确的镜像构成、受支持虚拟机配置、真机启动前提以及排障入口

#### Scenario: 第二阶段验收
- **WHEN** 第二阶段完成
- **THEN** 同一套核心抽象能够在 `ARM64 virt` 和 `RISC-V virt` 上复用并完成最小启动验证

### Requirement: 提供可执行的本地运行时验证环境
系统 SHALL 在开发机上提供完成 `x86_64 + QEMU + OVMF` 运行时验证所需的宿主机依赖和复验步骤，避免验证长期停留在“脚本已写好但环境未就绪”的状态。

#### Scenario: 本地环境就绪
- **WHEN** 开发者执行运行时验证或基线检查
- **THEN** 宿主机已具备 `qemu-system-x86_64` 与可用的 `OVMF` 固件文件，相关脚本可以实际启动虚拟机而不是只输出缺少依赖的错误

#### Scenario: 运行时基线完成
- **WHEN** 开发者执行 `make check-baseline`
- **THEN** 系统能够完成镜像构建、`QEMU + OVMF` 启动、串口/`debugcon` 日志捕获和关键启动/GUI 标记校验

### Requirement: 提供真机 U 盘启动说明与最小验证流程
系统 SHALL 在后续真机阶段提供将 `UEFI` 镜像写入 U 盘、选择启动介质、识别支持边界和收集失败现象的最小操作说明，以便真机验证不是口头目标。

#### Scenario: 真机验证准备
- **WHEN** 开发者准备在真机上进行 U 盘启动测试
- **THEN** 文档必须包含镜像写入方式、数据风险提示、目标机器 `UEFI` 前提条件和最小观察点

## MODIFIED Requirements
### Requirement: 项目目标定义
项目的近期目标从“做一个能跨 x86、ARM、RISC-V、MCU 的个人操作系统”细化为“先做一个在 `x86_64 + QEMU/OVMF` 上完成 `UEFI-first` 启动、内核 handoff 和 framebuffer demo 的个人操作系统基础骨架，同时为后续真机 `UEFI` 启动与 `ARM64/RISC-V` 移植及 `MCU` 子集演进预留清晰抽象边界”。

## REMOVED Requirements
### Requirement: 首阶段追求统一的完整跨平台系统能力
**Reason**: MPU 与 MCU 的系统模型差异过大，首阶段强行统一会显著增加复杂度并拖垮进度。
**Migration**: 先统一接口与目录分层，再在后续阶段分别落地 MPU 完整能力与 MCU 轻量子集。
