# ESP32 MCP2515 CAN Communication Module

中文文档 | [English](README_EN.md)

> This project is based on the [Microver-Electronics/mcp2515-esp32-idf](https://github.com/Microver-Electronics/mcp2515-esp32-idf) open source library. Thanks to the original authors for their contributions.

A CAN bus communication module based on ESP32S3 and MCP2515 chip, supporting CAN 2.0A/B protocol with stable transmission and reception capabilities.

## Features

- ✅ **CAN 2.0A/B Protocol Support**: Standard and extended frames
- ✅ **High-Speed SPI Communication**: 10MHz SPI clock for stable data transmission
- ✅ **Interrupt-Driven**: Efficient message processing based on GPIO interrupts
- ✅ **Automatic Error Recovery**: Auto-recovery from bus-off and error passive states
- ✅ **Real-Time Task Processing**: FreeRTOS multi-task architecture supporting concurrent send/receive
- ✅ **Detailed Logging**: Complete debug and status information

## Hardware Requirements

### Development Board
- **Seeed XIAO ESP32S3** or other ESP32S3 development boards

### CAN Module
- **MCP2515 CAN Controller Module**
- SPI interface communication support

### Connection Diagram

| ESP32S3 Pin | MCP2515 Module | Function |
|------------|---------------|----------|
| GPIO7      | SCK           | SPI Clock |
| GPIO8      | SO/MISO       | SPI Data Input |
| GPIO9      | SI/MOSI       | SPI Data Output |
| GPIO44     | CS            | Chip Select |
| GPIO43     | INT           | Interrupt Signal |
| 5V         | VCC           | Power Supply |
| GND        | GND           | Ground |

## Software Environment

### Required Software
- **ESP-IDF v5.4.1** or higher
- **Python 3.8+**
- **Git**

### Install ESP-IDF
```bash
# Clone ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git

# Enter ESP-IDF directory
cd esp-idf

# Install toolchain
./install.sh

# Set environment variables
source export.sh
```

## Project Structure

```
esp32-mcp2515/
├── CMakeLists.txt          # Project CMake configuration
├── main/                   # Main program directory
│   ├── CMakeLists.txt      # Main program CMake configuration
│   ├── esp32-mcp2515.c     # Main application
│   ├── mcp2515.c          # MCP2515 driver implementation
│   ├── mcp2515.h          # MCP2515 driver header
│   ├── can.h              # CAN protocol definitions
│   ├── can_test.c         # CAN test functions
│   └── can_test.h         # CAN test header
├── sdkconfig              # ESP-IDF configuration file
├── README.md              # Chinese documentation (default)
├── README_EN.md           # English documentation
└── .gitignore             # Git ignore file
```

## Build and Flash

### 1. Clone Project
```bash
git clone <your-repository-url>
cd esp32-mcp2515
```

### 2. Configure Project
```bash
idf.py menuconfig
```

In the configuration menu, you can adjust:
- **CAN Baud Rate**: Default 500kbps
- **SPI Clock Frequency**: Default 10MHz
- **GPIO Pin Configuration**: Adjust according to actual hardware connections

### 3. Build Project
```bash
idf.py build
```

### 4. Flash to Device
```bash
idf.py flash
```

### 5. Monitor Serial Output
```bash
idf.py monitor
```

## Usage

### Startup Functions

1. **Auto Transmission**: Sends one CAN message per second after startup
   - CAN ID: 0x123 (extended frame)
   - Data Length: 8 bytes
   - Data Content: Counter value + fixed data

2. **Auto Reception**: Real-time reception of CAN messages from external devices
   - Supports standard and extended frames
   - Automatically displays received message ID and data

3. **Error Handling**: Automatic CAN bus error handling
   - Auto-recovery from bus-off state
   - Error passive state handling
   - Receive overflow clearing

### Log Output Example

```
I (275) CAN_MODULE: Starting ESP32 MCP2515 CAN application...
I (305) CAN_MODULE: MCP2515 bitrate set to 500kbps
I (415) CAN_MODULE: CAN message sent successfully, counter: 0
I (13205) CAN_MODULE: CAN message received - ID: 0x00000007, DLC: 8, Data: 08 00 00 00 00 00 00 00
```

## Configuration Parameters

### CAN Configuration
- **Baud Rate**: 500kbps (configurable)
- **Clock Frequency**: 8MHz
- **Operating Mode**: Normal mode
- **Interrupt Enable**: Transmit, receive, error interrupts

### SPI Configuration
- **Clock Frequency**: 10MHz
- **Mode**: 0 (CPOL=0, CPHA=0)
- **Data Bits**: 8-bit
- **Queue Size**: 7

### Task Configuration
- **Transmit Task**: Priority 5, Stack 4KB
- **Receive Task**: Priority 5, Stack 4KB
- **Transmit Interval**: 1000ms

## Troubleshooting

### Common Issues

1. **Build Errors**
   - Ensure ESP-IDF environment is properly set up
   - Check if dependency libraries are complete

2. **SPI Communication Failure**
   - Check if hardware connections are correct
   - Confirm MCP2515 module power supply is normal
   - Verify SPI pin configuration

3. **CAN Communication Issues**
   - Confirm baud rate settings are consistent
   - Check CAN bus termination resistors
   - Verify external device connections

4. **Interrupt Not Working**
   - Check GPIO43 interrupt pin connection
   - Confirm interrupt configuration is correct

### Error Code Reference

- **ERROR_OK**: Operation successful
- **ERROR_FAIL**: General error
- **ERROR_NOMSG**: No message available to read
- **ERROR_FAILTX**: Transmission failed
- **ERROR_ALLTXBUSY**: All transmit buffers busy

## Development Guide

### Adding Custom Features

1. **Modify Transmit Data**
   ```c
   // Modify in can_send_task
   can_frame_tx.can_id = 0x123 | CAN_EFF_FLAG; // Modify CAN ID
   can_frame_tx.data[0] = your_data;           // Modify data
   ```

2. **Add Message Filters**
   ```c
   // Add after initialization
   MCP2515_setFilter(RXF0, false, 0x123); // Only receive messages with ID 0x123
   ```

3. **Modify Transmit Frequency**
   ```c
   // Modify in can_send_task
   vTaskDelay(pdMS_TO_TICKS(100)); // Send every 100ms
   ```

### Extension Suggestions

- **CAN Gateway Function**: Forward messages between different CAN buses
- **Data Logging**: Save CAN messages to Flash or SD card
- **Web Interface**: Provide Web configuration interface via WiFi
- **Data Parsing**: Parse CAN messages for specific protocols

## Error Handling

### CAN Error States

The module automatically handles various CAN error states:

- **Error Passive (0x40)**: Transmit error counter > 127, limited transmission capability
- **Bus Off (0x04)**: Transmit error counter > 255, complete transmission stop
- **Receive Overflow**: Buffer overflow, automatically cleared

### Error Recovery

- **Automatic Recovery**: System automatically recovers from error states
- **Error Monitoring**: Continuous monitoring of CAN bus status
- **Graceful Degradation**: Continues operation even with errors

## Performance

- **Transmission Rate**: 1 message per second (configurable)
- **Reception**: Real-time interrupt-driven reception
- **Error Recovery**: < 100ms recovery time
- **Memory Usage**: ~8KB RAM for tasks

## License

This project is licensed under the MIT License. See LICENSE file for details.

## Contributing

Issues and Pull Requests are welcome to improve this project.

## Contact

For questions or suggestions, please contact:
- Submit GitHub Issue
- Email: [your-email@example.com]

---

**Note**: Please ensure hardware connections are correct and read relevant documentation before use. 