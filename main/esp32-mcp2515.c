#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "can.h"
#include "mcp2515.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

// GPIO引脚定义 - Seeed XIAO ESP32S3
#define PIN_NUM_MISO 8
#define PIN_NUM_MOSI 9
#define PIN_NUM_CLK  7
#define PIN_NUM_CS   44
#define PIN_NUM_INTERRUPT 43

#define TAG "CAN_MODULE"

// 全局变量
static CAN_FRAME_t can_frame_tx;
static CAN_FRAME_t can_frame_rx;
static QueueHandle_t can_rx_queue;
static bool can_interrupt_flag = false;

// 中断处理函数
static void IRAM_ATTR can_isr_handler(void* arg)
{
    can_interrupt_flag = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(can_rx_queue, &can_interrupt_flag, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// 初始化GPIO中断
static esp_err_t init_can_interrupt(void)
{
    esp_err_t ret;
    
    // 配置中断引脚为输入，上拉
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_INTERRUPT),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 安装GPIO中断服务
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "GPIO ISR service install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 添加中断处理函数
    ret = gpio_isr_handler_add(PIN_NUM_INTERRUPT, can_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO ISR handler add failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "CAN interrupt initialized successfully");
    return ESP_OK;
}

// SPI初始化
bool SPI_Init(void)
{
    if (MCP2515_Object == NULL) {
        ESP_LOGE(TAG, "MCP2515_Object is NULL! Call MCP2515_init() first.");
        return false;
    }
    esp_err_t ret;
    // SPI总线配置
    spi_bus_config_t bus_cfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0 // 无限制
    };
    // MCP2515 SPI设备配置
    spi_device_interface_config_t dev_cfg = {
        .mode = 0, // (0,0) - CPOL=0, CPHA=0
        .clock_speed_hz = 10000000, // 10MHz，降低速度确保稳定性
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .flags = 0,
        .pre_cb = NULL,
        .post_cb = NULL,
    };
    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "SPI bus initialize failed: %s", esp_err_to_name(ret));
        return false;
    }
    spi_device_handle_t spi_handle = NULL;
    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI device add failed: %s", esp_err_to_name(ret));
        return false;
    }
    MCP2515_Object->spi = spi_handle;
    return true;
}

// CAN发送任务
void can_send_task(void *pvParameters)
{
    uint32_t message_counter = 0;
    
    while(1) {
        // 准备发送的CAN消息
        can_frame_tx.can_id = 0x123 | CAN_EFF_FLAG; // 扩展帧ID
        can_frame_tx.can_dlc = 8;
        can_frame_tx.data[0] = (message_counter >> 24) & 0xFF;
        can_frame_tx.data[1] = (message_counter >> 16) & 0xFF;
        can_frame_tx.data[2] = (message_counter >> 8) & 0xFF;
        can_frame_tx.data[3] = message_counter & 0xFF;
        can_frame_tx.data[4] = 0xAA;
        can_frame_tx.data[5] = 0xBB;
        can_frame_tx.data[6] = 0xCC;
        can_frame_tx.data[7] = 0xDD;
        
        // 检查CAN状态，如果处于错误状态则尝试恢复
        uint8_t error_flags = MCP2515_getErrorFlags();
        if (error_flags != 0) {
            ESP_LOGW(TAG, "CAN error detected before sending, flags: 0x%02X", error_flags);
            
            // 清除错误标志
            if (error_flags & EFLG_RX0OVR) {
                MCP2515_clearRXnOVR();
            }
            
            // 如果处于总线关闭状态，尝试恢复
            if (error_flags & EFLG_TXBO) {
                ESP_LOGW(TAG, "Bus-off detected, attempting recovery...");
                MCP2515_setNormalMode();
                vTaskDelay(pdMS_TO_TICKS(100));
                
                // 重新检查状态
                error_flags = MCP2515_getErrorFlags();
                if (error_flags & EFLG_TXBO) {
                    ESP_LOGE(TAG, "Bus-off recovery failed, skipping send");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                }
            }
        }
        
        // 发送消息
        ERROR_t result = MCP2515_sendMessageAfterCtrlCheck(&can_frame_tx);
        if (result == ERROR_OK) {
            ESP_LOGI(TAG, "CAN message sent successfully, counter: %lu", message_counter);
            message_counter++;
        } else {
            ESP_LOGE(TAG, "Failed to send CAN message, error: %d", result);
            
            // 检查发送后的错误状态
            error_flags = MCP2515_getErrorFlags();
            if (error_flags != 0) {
                ESP_LOGE(TAG, "CAN error flags after send: 0x%02X", error_flags);
                
                // 如果是总线关闭，等待更长时间
                if (error_flags & EFLG_TXBO) {
                    ESP_LOGE(TAG, "Bus-off after send, waiting for recovery...");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1秒发送一次
    }
}

// CAN接收任务
void can_receive_task(void *pvParameters)
{
    bool interrupt_flag;
    
    while(1) {
        // 等待中断信号，添加超时
        if (xQueueReceive(can_rx_queue, &interrupt_flag, pdMS_TO_TICKS(1000))) {
            can_interrupt_flag = false;
            
            // 直接读取中断状态，不等待
            uint8_t interrupts = MCP2515_getInterrupts();
            
            // 检查接收中断
            if (interrupts & (CANINTF_RX0IF | CANINTF_RX1IF)) {
                // 尝试读取消息
                ERROR_t result = MCP2515_readMessageAfterStatCheck(&can_frame_rx);
                if (result == ERROR_OK) {
                    ESP_LOGI(TAG, "CAN message received - ID: 0x%08X, DLC: %d, Data: %02X %02X %02X %02X %02X %02X %02X %02X",
                             (unsigned int)can_frame_rx.can_id, can_frame_rx.can_dlc,
                             can_frame_rx.data[0], can_frame_rx.data[1], can_frame_rx.data[2], can_frame_rx.data[3],
                             can_frame_rx.data[4], can_frame_rx.data[5], can_frame_rx.data[6], can_frame_rx.data[7]);
                } else if (result == ERROR_NOMSG) {
                    // 如果状态检查失败，尝试直接读取
                    ERROR_t direct_result = MCP2515_readMessage(RXB0, &can_frame_rx);
                    if (direct_result == ERROR_OK) {
                        ESP_LOGI(TAG, "CAN message received - ID: 0x%08X, DLC: %d, Data: %02X %02X %02X %02X %02X %02X %02X %02X",
                                 (unsigned int)can_frame_rx.can_id, can_frame_rx.can_dlc,
                                 can_frame_rx.data[0], can_frame_rx.data[1], can_frame_rx.data[2], can_frame_rx.data[3],
                                 can_frame_rx.data[4], can_frame_rx.data[5], can_frame_rx.data[6], can_frame_rx.data[7]);
                    } else {
                        // 尝试读取RX1
                        direct_result = MCP2515_readMessage(RXB1, &can_frame_rx);
                        if (direct_result == ERROR_OK) {
                            ESP_LOGI(TAG, "CAN message received - ID: 0x%08X, DLC: %d, Data: %02X %02X %02X %02X %02X %02X %02X %02X",
                                     (unsigned int)can_frame_rx.can_id, can_frame_rx.can_dlc,
                                     can_frame_rx.data[0], can_frame_rx.data[1], can_frame_rx.data[2], can_frame_rx.data[3],
                                     can_frame_rx.data[4], can_frame_rx.data[5], can_frame_rx.data[6], can_frame_rx.data[7]);
                        }
                    }
                }
            }
            
            // 检查发送中断 - 需要特殊处理
            if (interrupts & (CANINTF_TX0IF | CANINTF_TX1IF | CANINTF_TX2IF)) {
                // 清除发送中断标志 - 发送完成后必须清除
                MCP2515_clearTXInterrupts();
            }
            
            // 检查错误中断
            if (interrupts & CANINTF_ERRIF) {
                uint8_t error_flags = MCP2515_getErrorFlags();
                
                // 处理错误状态
                if (error_flags & EFLG_RX0OVR) {
                    ESP_LOGE(TAG, "RX0 overflow detected, clearing");
                    MCP2515_clearRXnOVR();
                }
                
                if (error_flags & EFLG_TXBO) {
                    ESP_LOGE(TAG, "Bus-off state detected, attempting recovery");
                    // 尝试恢复总线
                    MCP2515_setNormalMode();
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                
                MCP2515_clearERRIF();
            }
            
            // 最后清除所有中断标志，确保没有遗漏
            MCP2515_clearInterrupts();
            
        } else {
            // 超时，检查是否有未处理的中断
            uint8_t interrupts = MCP2515_getInterrupts();
            if (interrupts != 0) {
                // 处理未处理的中断
                if (interrupts & (CANINTF_TX0IF | CANINTF_TX1IF | CANINTF_TX2IF)) {
                    MCP2515_clearTXInterrupts();
                } else {
                    MCP2515_clearInterrupts();
                }
            }
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32 MCP2515 CAN application...");
    
    // 创建CAN接收队列
    can_rx_queue = xQueueCreate(10, sizeof(bool));
    if (can_rx_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create CAN RX queue");
        return;
    }
    
    // 初始化MCP2515
    ERROR_t result = MCP2515_init();
    if (result != ERROR_OK) {
        ESP_LOGE(TAG, "MCP2515 initialization failed: %d", result);
        return;
    }
    ESP_LOGI(TAG, "MCP2515 initialized successfully");
    
    // 初始化SPI
    if (!SPI_Init()) {
        ESP_LOGE(TAG, "SPI initialization failed");
        return;
    }
    
    // 初始化CAN中断
    if (init_can_interrupt() != ESP_OK) {
        ESP_LOGE(TAG, "CAN interrupt initialization failed");
        return;
    }
    
    // 复位MCP2515
    result = MCP2515_reset();
    if (result != ERROR_OK) {
        ESP_LOGE(TAG, "MCP2515 reset failed: %d", result);
        return;
    }
    ESP_LOGI(TAG, "MCP2515 reset completed");
    
    // 设置波特率 - 使用500kbps，更稳定
    result = MCP2515_setBitrate(CAN_500KBPS, MCP_8MHZ);
    if (result != ERROR_OK) {
        ESP_LOGE(TAG, "MCP2515 set bitrate failed: %d", result);
        return;
    }
    ESP_LOGI(TAG, "MCP2515 bitrate set to 500kbps");
    
    // 设置正常模式
    result = MCP2515_setNormalMode();
    if (result != ERROR_OK) {
        ESP_LOGE(TAG, "MCP2515 set normal mode failed: %d", result);
        return;
    }
    ESP_LOGI(TAG, "MCP2515 set to normal mode");
    
    // 等待模式切换完成
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 检查MCP2515状态
    uint8_t canstat = MCP2515_readRegister(MCP_CANSTAT);
    ESP_LOGI(TAG, "MCP2515 CANSTAT: 0x%02X", canstat);
    
    // 创建发送和接收任务
    xTaskCreate(can_send_task, "can_send", 4096, NULL, 5, NULL);
    xTaskCreate(can_receive_task, "can_receive", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "CAN application started successfully");
    
    // 主循环 - 保持系统运行
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}