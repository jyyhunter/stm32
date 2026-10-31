# SimpleFOC 上位机

## 启动

```powershell
cd D:\stm32code\SimpleFOC\host
python foc_host.py
```

也可以双击 `run_host.bat`。无硬件时可运行：

```powershell
python foc_host.py --demo
```

## 串口参数

- 端口：连接 USART3 的 USB 转串口，例如 CH340。
- 波特率：`115200`。
- 数据格式：8 数据位、无校验、1 停止位。

## MCU 协议

每条命令以换行结束：

| 命令 | 功能 |
|---|---|
| `START` | 使能驱动并进入零速模式 |
| `STOP` | 立即关闭功率输出 |
| `V,60` | 平滑 V/f 速度模式，范围 -90 到 90 RPM |
| `F,60` | 电压闭环 FOC 速度诊断模式，范围 -90 到 90 RPM |
| `S,60` | 编码器换相、缓慢电压调节的平滑速度模式 |
| `P,90` | 位置模式，目标相对零位 90°，范围 -720 到 720 |
| `ZERO` | 将当前机械位置设为零，并停止电机 |
| `K` | 心跳；使能状态下超过 2 秒未收到心跳会自动停机 |

MCU 每 100 ms 发送一行：

```text
FOC,<ms>,<enabled>,<mode>,<target_rpm>,<actual_rpm>,<target_deg>,<position_deg>,<uq>
```

`mode`：0=停止，1=速度，2=位置。

## 安全说明

该驱动板没有相电流采样，软件只能限制电压并通过速度反馈检测堵转。首次上电应使用限流电源，并确认固件中的母线电压与实际电源一致。上位机断开连接时不会依赖 PC 保持电机运行；MCU 心跳超时会主动关闭驱动。
