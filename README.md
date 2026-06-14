# Samsung SM-S9380V GKI Kernel — OneUI 7 / Android 15

LKM-optimized kernel for **Samsung Galaxy SM-S9380V** (Snapdragon 8 Gen 3 / SM8650),
maintained as a long-term stable branch (养老分支) for local CLI compilation via
[Kokuban Kernel CI Center](https://github.com/YuzakiKokuban/Kokuban_Kernel_CI_Center).

## Source

- **Base:** SM-S9380V_HKTW_15_Opensource (Samsung Open Source)
- **Chipset:** SM8650 (Snapdragon 8 Gen 3)
- **Target:** OneUI 7 / Android 15 GKI (KMI generation 8)
- **Kernel version:** 6.6.x (android15-6.6)

## Features

### LKM Mode (default)
Pure kernel — **no built-in KSU/SUSFS**. KernelSU is injected at flash time
by the KSU manager app patching `init_boot`, keeping the kernel binary clean
and compatible with any KSU version.

### Resukisu Mode
Built-in ReSukiSU + SuSFS for users who prefer an all-in-one kernel image.
Build with `MODE=resukisu`.

### Common Features (both modes)
- **Security stack disabled:** UH, RKP, KDP, SECURITY_DEFEX, INTEGRITY, FIVE
- **Baseband Guard** LSM support
- **WildKernels performance patches** (f2fs, ext4, memory, TCP, scheduler)
- **NTSync** driver (Windows NT sync for Wine/Proton)
- **Re:Kernel** advanced driver
- **IPv6 NAT** with /proc/config.gz concealment
- **NTFS3** with LZX/XPRESS compression
- **ZRAM** with LZ4 default
- **TCP BBR** as default congestion control
- **SYSVIPC**, POSIX message queues, PID/IPC namespaces
- **Full IPSet** support

## Quick Start

```bash
# Clone with toolchain
git clone https://github.com/YuzakiKokuban/android_kernel_samsung_sm8750.git
cd android_kernel_samsung_sm8750
git checkout 7.0

# Download Samsung clang-r487747c toolchain to ../toolchain_samsung_sm8650/prebuilts/
# (from https://opensource.samsung.com)

# Build (LKM mode - default)
build/build.sh lkm

# Build (Resukisu mode)
MODE=resukisu build/build.sh resukisu

# Build and pack AnyKernel3 zip
PACK=1 ANYKERNEL_DIR=../AnyKernel3_sm9380 build/build.sh lkm
```

## CI Center Local Build

使用现有的 `s25_sm8750` 项目配置（已内置在 `Kokuban_Kernel_CI_Center/configs/projects.json`）：

```bash
# 在 Kokuban_Kernel_CI_Center 仓库中：
./kokuban local --project s25_sm8750 --branch 7.0

# 纯 LKM 模式（不启用 SuSFS/BBG，默认）：
./kokuban local --project s25_sm8750 --branch 7.0 --no-susfs --no-bbg

# 带 SuSFS（需要 resukisu 模式，详见 build/build.sh）：
./kokuban local --project s25_sm8750 --branch 7.0 --with-susfs
```

CI Center 会自动下载工具链到 `kernel_platform/prebuilts/`，无需手动配置。

## Build Dependencies

```bash
sudo apt-get install -y build-essential git bc bison flex libssl-dev \
  cpio lz4 libelf-dev dwarves pahole libdw-dev unzip zip ccache
```
