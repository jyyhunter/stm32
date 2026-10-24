# STM32F103VET6 + ATK-MS601M 六轴姿态模块 · PlatformIO(VSCode) 工程

本目录是一套**可直接在 VSCode + PlatformIO 中开发、编译、下载、调试**的 STM32F103VET6 标准外设库（SPL）工程，
与原有的 Keil 工程（`../STM32固件/Project.uvprojx`）**源码完全一致、互不影响**：Keil 工程没有做任何修改，
本目录是一份独立的副本，使用 `arm-none-eabi-gcc`（PlatformIO 自动下载）编译。

> **编译状态：已在本机实测编译通过（2026-10-24）。**
> 
> - PlatformIO Core 6.2.0 + 平台 `ststm32` 20.0.0 + 工具链 `toolchain-gccarmnoneeabi` 1.70201.0（**GCC 7.2.1**）
> - 结果：`[SUCCESS]`，RAM 占用 **2788 / 65536 字节（4.3%）**，Flash 占用 **26772 / 524288 字节（5.1%）**
> - 产物：`.pio/build/genericSTM32F103VE/firmware.elf / .hex / .bin`
> - 工具链与平台已安装到 `C:\Users\Administrator\.platformio\`，在 VSCode 中打开本文件夹即可直接 Build，**无需再下载**。

---

## 1. 目录结构

```
PlatformIO固件/
├── platformio.ini            # PlatformIO 工程配置（芯片/框架/编译选项/下载方式）
├── linker/
│   └── STM32F103VETx_FLASH.ld # GCC 链接脚本（512K Flash / 64K RAM）
├── tools/
│   └── build_hex.py          # 编译后自动生成 .hex/.bin
├── src/                      # 全部源码（与 Keil 分组一一对应）
│   ├── Start/                # 启动文件、CMSIS、系统时钟
│   │   ├── startup_stm32f10x_hd.s   # ★ GCC 汇编启动文件（GNU as 语法，区别于 Keil 的 armasm 版）
│   │   ├── core_cm3.c/.h
│   │   ├── stm32f10x.h
│   │   └── system_stm32f10x.c/.h
│   ├── Library/              # STM32F10x 标准外设库 SPL（misc + 全部外设 .c/.h）
│   ├── System/               # Delay.c/.h（SysTick 延时）
│   ├── Hardware/             # Serial(USART1)、RingBuffer、MS901(USART3 及协议解析)
│   └── User/
│       ├── main.c
│       ├── app_protocol.c/.h       # 向上位机发送的文本帧
│       ├── stm32f10x_it.c/.h
│       ├── stm32f10x_conf.h
│       └── syscalls.c              # ★ GCC newlib 桩：_write 重定向到 USART1，_sbrk 堆
└── .vscode/
    └── extensions.json       # 推荐安装 PlatformIO IDE 扩展
```

> 带 ★ 的三个文件是 **GCC 构建相对 Keil 新增/替换的内容**：
> 
> - `startup_stm32f10x_hd.s`：GNU 汇编启动文件（Keil 用的是 armasm 语法，无法被 gcc 汇编）；
> - `STM32F103VETx_FLASH.ld`：GCC 链接脚本（Keil 用芯片默认内存布局）；
> - `syscalls.c`：newlib 标准库的系统调用桩，并把 `printf`/`puts` 重定向到 USART1（Keil 用 MicroLib + `fputc`）。

---

## 2. 环境准备（仅第一次）

1. 安装 **VSCode**（本机已安装）。

2. 在 VSCode 扩展市场搜索并安装 **PlatformIO IDE**（作者 PlatformIO.PlatformIO）。

3. 安装后 PlatformIO 会在后台自动安装 PlatformIO Core（命令行）和相关组件，**耐心等待右下角进度条完成**。

4. 第一次编译时，PlatformIO 会自动下载：
   
   - `ststm32` 平台
   - `toolchain-gccarmnoneeabi`（ARM GCC 交叉编译器，约 100MB）
   - 下载/调试工具（OpenOCD、STM32CubeProgrammer 等，仅在执行 upload/debug 时下载）
   
   这些文件在国外服务器，若下载缓慢或失败：
   
   - 打开你的代理/VPN；
   - 或在系统环境变量中设置 PlatformIO 镜像（可选）：
     `PLATFORMIO_PLATFORMS_DIR` 等保持默认即可，国内一般开启代理后即可正常下载。
   
   > 本机情况：工具链已通过离线方式安装完成（见第 6 节“离线安装工具链”），无需重复下载。

> 本工程路径 `D:\stm32code\6轴姿态传感器\PlatformIO固件` 中，**路径中含中文“6轴姿态传感器”**。
> 现代 PlatformIO 一般可正常处理；若个别工具因中文路径报错，把整个 `PlatformIO固件` 文件夹复制到
> 纯英文路径（如 `D:\stm32code\PlatformIO固件` 或 `C:\pio\attitude`）后再用 VSCode 打开即可。

---

## 3. 打开 / 编译 / 下载 / 调试

### 打开工程

- VSCode → 文件 → 打开文件夹 → 选择 **`PlatformIO固件`**（包含 `platformio.ini` 的这一级）。
- 打开后右下角出现 PlatformIO 小图标（蚂蚁头像），左侧多一条 PlatformIO 工具栏。

### 编译（Build）

- 快捷键 `Ctrl+Alt+B`，或点左下角对勾 **(Build)**，或在命令行：
  
  ```
  pio run
  ```
- 成功后产物在：
  
  ```
  .pio/build/genericSTM32F103VE/firmware.elf
  .pio/build/genericSTM32F103VE/firmware.hex   ← 本工程额外生成
  .pio/build/genericSTM32F103VE/firmware.bin
  ```

### 下载（Upload）

- 方式 A：**ST-Link**（默认，推荐）。ST-Link 的 SWDIO/SWCLK/GND/3V3 接好后，点左箭头 **(Upload)**，或：
  
  ```
  pio run -t upload
  ```
- 方式 B：**USB-TTL 串口下载**（使用芯片内置 UART Bootloader，无需 ST-Link）：
  1. 把 `BOOT0` 接高电平（BOOT1 保持低），复位一次进入系统存储器启动；
  2. USB-TTL 的 TXD→PA10、RXD→PA9、GND→GND；
  3. 编辑 `platformio.ini`，把 `upload_protocol = stlink` 一段换成文件末尾注释中的 `serial` 配置；
  4. 首次需安装 `stm32flash`（PlatformIO 一般会自动提示安装 `tool-stm32flash`）；
  5. 执行 `pio run -t upload`，完成后把 BOOT0 接回低电平并复位。
     （也可用 FlyMcu / STM32 Flash Loader 直接烧录上面生成的 `firmware.hex`。）

### 调试（Debug）

- 用 ST-Link 连接后，点 PlatformIO 工具栏的 **Debug**（虫子播放键），可下断点、看寄存器/变量/外设。

### 串口监视（查看上位机文本帧）

- 点插头图标 **(Monitor)**，或：
  
  ```
  pio device monitor
  ```
  
  默认波特率 115200（已在 `platformio.ini` 中配置 `monitor_speed=115200`）。

---

## 4. 与 Keil 工程的对应关系

| 项目         | Keil（ARMCC）                                         | PlatformIO（arm-none-eabi-gcc）                      |
| ---------- | --------------------------------------------------- | -------------------------------------------------- |
| 芯片         | STM32F103VE，Cortex-M3                               | 同（board = genericSTM32F103VE）                      |
| 宏定义        | `STM32F10X_HD, USE_STDPERIPH_DRIVER`                | `build_flags` 中相同的两个 `-D`                          |
| 头文件路径      | `Start;Library;User;System;Hardware;Hardware\MS901` | `build_flags` 中对应 6 个 `-Isrc/...`                  |
| 启动文件       | `Start/startup_stm32f10x_hd.s`（armasm）              | `src/Start/startup_stm32f10x_hd.s`（GNU as，新文件）     |
| 链接布局       | 芯片默认（0x08000000/512K，0x20000000/64K）                | `linker/STM32F103VETx_FLASH.ld`（同地址/容量）            |
| printf 重定向 | MicroLib + `fputc`                                  | newlib + `syscalls.c` 的 `_write`（`fputc` 仍保留，两者兼容） |
| 业务源码       | User/Hardware/System/Library                        | 完全相同（副本）                                           |

> 串口功能、接线、文本帧协议、六轴/十轴切换等与 Keil 工程**完全一致**，详见根目录 `README.md` 与
> `../STM32固件/固件说明.md`。

### 六轴 / 十轴切换

在 `src/Hardware/MS901/atk_ms901m.h` 顶部修改宏：

```c
#define ATK_MODULE_MS601M   1   // 1 = ATK-MS601M 六轴（默认，加速度 ±4g）
                                // 0 = ATK-MS901M 十轴（加速度 ±2g，含磁力计/气压）
```

改完重新编译即可。

---

## 5. 常见问题

- **首次编译卡在下载工具链**：网络问题，开代理重试；或切换到稳定网络后再 `pio run`。
- **提示找不到 `platformio` 命令**：重启 VSCode，或使用 PlatformIO 工具栏按钮（不依赖命令行）。
- **中文路径报错**：把 `PlatformIO固件` 复制到纯英文路径再打开（见第 2 节）。
- **`#error "Please select first the target STM32F10x device..."`**：说明 `STM32F10X_HD` 宏未生效，
  检查 `platformio.ini` 的 `build_flags` 是否含 `-D STM32F10X_HD`。
- **下载失败 / 找不到芯片**：确认 ST-Link 驱动、SWD 接线、芯片被复位；或改用串口下载。
- **Keil 还能正常用吗**：能。本目录是独立副本，不修改、不依赖 `../STM32固件` 中的任何 Keil 文件；
  两套工程可并行维护。

---

## 6. 离线安装工具链（网络缓慢时）

本工程已在本机装好工具链，正常情况下无需本节操作。仅当换了电脑 / 重装环境，且 PlatformIO 自动下载工具链卡死时，
按下面方法手动离线安装（已实测可用）：

1. 从 PlatformIO 注册表获取工具链包（需要能访问 `api.registry.platformio.org`）：
   - 下载地址（Windows，GCC 7.2.1，约 100MB，ststm32 20.x 要求 `<1.80000`）：
     
     ```
     https://dl.registry.platformio.org/download/platformio/tool/toolchain-gccarmnoneeabi/1.70201.0/toolchain-gccarmnoneeabi-windows-1.70201.0.tar.gz
     ```
   - 用 curl 断点续传下载（支持中断后继续）：
     
     ```
     curl -L --retry 8 --retry-delay 5 --retry-all-errors -C - ^
       -o toolchain.tar.gz ^
       "https://dl.registry.platformio.org/download/platformio/tool/toolchain-gccarmnoneeabi/1.70201.0/toolchain-gccarmnoneeabi-windows-1.70201.0.tar.gz"
     ```
   - 校验 SHA256 应为 `05e08e99fd6fff2e0236ddab54f472b1daa6648f83cd2cce5f47955f983012a9`。
2. 解压到 PlatformIO 包目录（包内根目录直接是 `bin/ lib/ arm-none-eabi/ share/ package.json`）：
   
   ```
   mkdir "%USERPROFILE%\.platformio\packages\toolchain-gccarmnoneeabi"
   tar -xzf toolchain.tar.gz -C "%USERPROFILE%\.platformio\packages\toolchain-gccarmnoneeabi"
   ```
3. 在该目录新建一个名为 `.piopm` 的文件（无扩展名），内容为一行 JSON：
   
   ```
   {"type": "tool", "name": "toolchain-gccarmnoneeabi", "version": "1.70201.0", "spec": {"owner": "platformio", "id": null, "name": "toolchain-gccarmnoneeabi", "requirements": null, "uri": null}}
   ```
4. 回到工程目录执行 `pio run`，应能直接识别并编译（输出中可见 `toolchain-gccarmnoneeabi @ 1.70201.0 (7.2.1)`）。

---

## 7. GCC 构建相对 Keil 源码做的 3 处必要修改（均只在本目录副本内）

标准外设库 + 早期 CMSIS 是按 ARMCC/旧 GCC 编写的，在 GCC 7.2.1（较新的 binutils）下编译需要三处小改动。
这些改动**只在 `PlatformIO固件/src` 与 `linker` 副本内，Keil 工程原样未动**：

1. **`src/Start/core_cm3.c` —— 修复 `strexb/strexh/strex: registers may not be the same`**
   三个 `__STREXx` 函数的内联汇编输出操作数由 `"=r" (result)` 改为 **`"=&r" (result)`**（early-clobber），
   强制编译器给结果寄存器和输入值分配不同寄存器。这是 ST 官方对该问题的标准修复。

2. **`linker/STM32F103VETx_FLASH.ld` —— 补充 `MEMORY` 区域声明**
   加入：
   
   ```
   MEMORY
   {
     FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
     RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 64K
   }
   ```
   
   否则链接器报 `memory region 'FLASH'/'RAM' not declared`。

3. **`src/Start/startup_stm32f10x_hd.s` —— 去掉末尾的 BootRAM 表项**
   删除向量表末尾的 8 个 `.word 0` 和 `.word BootRAM`，使中断向量表在 `DMA2_Channel4_5_IRQHandler` 处结束，
   与 Keil 版启动文件完全一致（本工程不用 RAM 启动，无需该表项）；否则链接报 `undefined reference to BootRAM`。

另外 `src/User/syscalls.c` 为 GCC 新增文件（newlib 系统调用桩），`fputc` 重定向在 `Serial.c` 中保留，二者兼容。
