# STM32F103VET6 标准外设库 · PlatformIO 工程模板

一个干净、自包含、可直接复制使用的工程模板。复制 → 改名 → 烧录即可开始新项目。
已通过 GCC 编译：

```
RAM:   4.0% (2620 / 65536 字节)
Flash: 5.0% (26380 / 524288 字节)
[SUCCESS]
```

## 一、快速开始（基于模板新建项目）

**方法1：使用生成脚本（推荐）**
在 `D:\chatgpt\stm32\` 下，右键 `NewProject.ps1` → 用 PowerShell 运行，输入新项目名；
或在终端执行：
```powershell
.\NewProject.ps1 -Name 我的新项目
```
脚本会复制模板、清理构建产物，并用 VSCode 打开。

**方法2：手动复制**
1. 复制整个 `STM32F103VE_SPL_Template` 文件夹，改成你的项目名；
2. 删除其中的 `.pio` 文件夹（旧构建缓存，可选）；
3. 用 VSCode 打开新文件夹（安装好 PlatformIO 扩展）。

> 首次打开时 PlatformIO 会自动下载工具链/OpenOCD，无需手动安装。

## 二、默认示例（验证环境用）

上电后现象：
- 板载 LED 每 500ms 翻转一次；
- USART1（PA9/PA10，115200）周期打印 `running: 0,1,2…`。

确认现象正常即说明环境 OK；之后删除/替换示例逻辑即可。

## 三、目录结构

```
STM32F103VE_SPL_Template/
├── platformio.ini        # 工程配置（芯片、宏、烧录、调试、串口）
├── .gitignore
├── src/                  # ★ 你的应用代码放这里
│   ├── main.c            # 主程序：LED/USART 初始化示例
│   ├── stm32f10x_it.c/.h # 中断服务函数
│   ├── stm32f10x_conf.h  # 标准库配置（用到哪些外设头）
│   ├── syscalls.c        # newlib 系统调用桩：printf 重定向(_write)、malloc(_sbrk)
│   ├── system_stm32f10x.c# 系统时钟配置（默认 72MHz）
│   └── startup_stm32f10x_hd.s  # GCC 启动文件（高密度 HD）
├── lib/
│   └── StdPeriph_Driver/ # ST 标准外设库 V3.5.0（一般不用动）
│       ├── library.json
│       └── src/          # 23 个标准库 .c/.h + stm32f10x.h + core_cm3.h
└── linker/
    └── STM32F103VE_FLASH.ld   # 链接脚本（512K Flash / 64K RAM）
```

## 四、在模板上开发

### 1. 改 LED 引脚（按你的板子）
`main.c` 顶部：
```c
#define LED_PORT   GPIOC
#define LED_PIN    GPIO_Pin_13   // 最小系统板常见 PC13；其他板子改这里
```

### 2. 添加自己的代码 / 模块
- 简单逻辑：直接写在 `src/main.c` 的 `while(1)` 里；
- 新模块：在 `src/` 下新建 `xxx.c` / `xxx.h`（例如 `led.c`、`dht11.c`），PIO 自动编译；
- 可复用模块：在 `lib/` 下新建文件夹，放入源码（带不带 `library.json` 均可）。

### 3. 启用某外设
- 例：用串口接收 → 在 `stm32f10x_it.c` 实现 `USART1_IRQHandler`（文件中有模板）；
- 用定时器/外部中断/PWM/ADC → 标准库已在 `lib` 中，直接调用对应 `RCC/GPIO/TIM/...` 函数。

### 4. 常用命令
```bash
pio run                    # 编译
pio run -t upload          # ST-Link 烧录
pio device monitor         # 串口监视器（115200）
pio run -t upload -t monitor   # 烧录 + 开串口
pio debug                  # 在线调试
pio run -t clean            # 清理
```

## 五、关键配置说明

| 项 | 值 / 位置 |
|----|-----------|
| 芯片 | STM32F103VET6，Cortex-M3，512KB Flash / 64KB RAM |
| 密度宏 | `STM32F10X_HD`（高密度，platformio.ini） |
| 库宏 | `USE_STDPERIPH_DRIVER` |
| 时钟 | 默认 72MHz（system_stm32f10x.c，需 HSE=8MHz） |
| 串口 | USART1：PA9 TX / PA10 RX，115200 |
| 烧录/调试 | ST-Link（SWD）+ OpenOCD |

## 六、常见问题

- **HSE 不是 8MHz**：改 `src/system_stm32f10x.c` 的时钟宏与 `HSE_VALUE`，否则时钟/延时不准。
- **`printf` 无输出**：确认已初始化 USART1，USB转串口接到 PA9（交叉），波特率 115200。
- **不用 printf/串口**：可删除 `syscalls.c` 中相关内容或改用其他串口（改 `_write`）。
- **要换其他 F103 型号**：改 `platformio.ini` 的 `board`、密度宏（如 C8 用 `STM32F10X_MD`、
  启动文件换 `md.s`、链接脚本换对应型号）。
