# ESP32 MCP2515 CAN 通信模块

[English](README_EN.md) | 中文文档

> 本项目基于 [Microver-Electronics/mcp2515-esp32-idf](https://github.com/Microver-Electronics/mcp2515-esp32-idf) 开源库开发，感谢原作者的贡献。

基于ESP32S3和MCP2515芯片的CAN总线通信模块，支持CAN 2.0A/B协议，提供稳定的发送和接收功能。

## 功能特性

- ✅ **CAN 2.0A/B协议支持**：支持标准帧和扩展帧
- ✅ **高速SPI通信**：10MHz SPI时钟，确保数据传输稳定性
- ✅ **中断驱动**：基于GPIO中断的高效消息处理
- ✅ **自动错误恢复**：总线关闭、错误被动等状态的自动恢复
- ✅ **实时任务处理**：FreeRTOS多任务架构，支持并发发送和接收
- ✅ **详细日志输出**：完整的调试和状态信息

## 硬件要求

### 开发板
- **Seeed XIAO ESP32S3** 或其他ESP32S3开发板

### CAN模块
- **MCP2515 CAN控制器模块**
- 支持SPI接口通信

### 连接方式

| ESP32S3引脚 | MCP2515模块 | 功能 |
|------------|------------|------|
| GPIO7      | SCK        | SPI时钟 |
| GPIO8      | SO/MISO    | SPI数据输入 |
| GPIO9      | SI/MOSI    | SPI数据输出 |
| GPIO44     | CS         | 片选信号 |
| GPIO43     | INT        | 中断信号 |
| 5V         | VCC        | 电源 |
| GND        | GND        | 地 |

## 软件环境

### 必需软件
- **ESP-IDF v5.4.1** 或更高版本
- **Python 3.8+**
- **Git**

### 安装ESP-IDF
```bash
# 克隆ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git

# 进入ESP-IDF目录
cd esp-idf

# 安装工具链
./install.sh

# 设置环境变量
source export.sh
```

## 项目结构

```
esp32-mcp2515/
├── CMakeLists.txt          # 项目CMake配置
├── main/                   # 主程序目录
│   ├── CMakeLists.txt      # 主程序CMake配置
│   ├── esp32-mcp2515.c     # 主应用程序
│   ├── mcp2515.c          # MCP2515驱动实现
│   ├── mcp2515.h          # MCP2515驱动头文件
│   ├── can.h              # CAN协议定义
│   ├── can_test.c         # CAN测试功能
│   └── can_test.h         # CAN测试头文件
├── sdkconfig              # ESP-IDF配置文件
├── README.md              # 中文文档（默认）
├── README_EN.md           # 英文文档
└── .gitignore             # Git忽略文件
```

## 编译和烧录

### 1. 克隆项目
```bash
git clone <your-repository-url>
cd esp32-mcp2515
```

### 2. 配置项目
```bash
idf.py menuconfig
```

在配置菜单中可以调整：
- **CAN波特率**：默认500kbps
- **SPI时钟频率**：默认10MHz
- **GPIO引脚配置**：根据实际硬件连接调整

### 3. 编译项目
```bash
idf.py build
```

### 4. 烧录到设备
```bash
idf.py flash
```

### 5. 监控串口输出
```bash
idf.py monitor
```

## 使用说明

### 启动后功能

1. **自动发送**：程序启动后每秒发送一条CAN消息
   - CAN ID: 0x123 (扩展帧)
   - 数据长度: 8字节
   - 数据内容: 计数器值 + 固定数据

2. **自动接收**：实时接收外部CAN设备发送的消息
   - 支持标准帧和扩展帧
   - 自动显示接收到的消息ID和数据

3. **错误处理**：自动处理CAN总线错误
   - 总线关闭状态自动恢复
   - 错误被动状态处理
   - 接收溢出清除

### 日志输出示例

```
I (275) CAN_MODULE: Starting ESP32 MCP2515 CAN application...
I (305) CAN_MODULE: MCP2515 bitrate set to 500kbps
I (415) CAN_MODULE: CAN message sent successfully, counter: 0
I (13205) CAN_MODULE: CAN message received - ID: 0x00000007, DLC: 8, Data: 08 00 00 00 00 00 00 00
```

## 配置参数

### CAN配置
- **波特率**: 500kbps (可配置)
- **时钟频率**: 8MHz
- **工作模式**: 正常模式
- **中断使能**: 发送、接收、错误中断

### SPI配置
- **时钟频率**: 10MHz
- **模式**: 0 (CPOL=0, CPHA=0)
- **数据位**: 8位
- **队列大小**: 7

### 任务配置
- **发送任务**: 优先级5，堆栈4KB
- **接收任务**: 优先级5，堆栈4KB
- **发送间隔**: 1000ms

## 故障排除

### 常见问题

1. **编译错误**
   - 确保ESP-IDF环境正确设置
   - 检查依赖库是否完整

2. **SPI通信失败**
   - 检查硬件连接是否正确
   - 确认MCP2515模块供电正常
   - 验证SPI引脚配置

3. **CAN通信问题**
   - 确认波特率设置一致
   - 检查CAN总线终端电阻
   - 验证外部设备连接

4. **中断不工作**
   - 检查GPIO43中断引脚连接
   - 确认中断配置正确

### 错误代码说明

- **ERROR_OK**: 操作成功
- **ERROR_FAIL**: 一般错误
- **ERROR_NOMSG**: 没有消息可读
- **ERROR_FAILTX**: 发送失败
- **ERROR_ALLTXBUSY**: 所有发送缓冲区忙

## 开发指南

### 添加自定义功能

1. **修改发送数据**
   ```c
   // 在can_send_task中修改
   can_frame_tx.can_id = 0x123 | CAN_EFF_FLAG; // 修改CAN ID
   can_frame_tx.data[0] = your_data;           // 修改数据
   ```

2. **添加消息过滤器**
   ```c
   // 在初始化后添加
   MCP2515_setFilter(RXF0, false, 0x123); // 只接收ID为0x123的消息
   ```

3. **修改发送频率**
   ```c
   // 在can_send_task中修改
   vTaskDelay(pdMS_TO_TICKS(100)); // 100ms发送一次
   ```

### 扩展功能建议

- **CAN网关功能**: 转发不同CAN总线间的消息
- **数据记录**: 保存CAN消息到Flash或SD卡
- **Web界面**: 通过WiFi提供Web配置界面
- **数据解析**: 解析特定协议的CAN消息

## 错误处理

### CAN错误状态

模块自动处理各种CAN错误状态：

- **错误被动 (0x40)**: 发送错误计数器 > 127，发送能力受限
- **总线关闭 (0x04)**: 发送错误计数器 > 255，完全停止发送
- **接收溢出**: 缓冲区溢出，自动清除

### 错误恢复

- **自动恢复**: 系统自动从错误状态恢复
- **错误监控**: 持续监控CAN总线状态
- **优雅降级**: 即使有错误也能继续运行

## 性能指标

- **发送速率**: 每秒1条消息 (可配置)
- **接收**: 实时中断驱动接收
- **错误恢复**: < 100ms恢复时间
- **内存使用**: 任务约8KB RAM

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交GitHub Issue
- 发送邮件至：[your-email@example.com]

---

**注意**: 使用前请确保硬件连接正确，并仔细阅读相关文档。 