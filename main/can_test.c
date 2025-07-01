#include "can_test.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

#define TAG "CAN_TEST"

// 全局测试变量
static uint32_t test_message_counter = 0;
static uint32_t test_received_messages = 0;
static bool test_running = false;

// 回环测试
void can_loopback_test(void)
{
    ESP_LOGI(TAG, "Starting CAN loopback test...");
    
    // 设置回环模式
    ERROR_t result = MCP2515_setLoopbackMode();
    if (result != ERROR_OK) {
        ESP_LOGE(TAG, "Failed to set loopback mode: %d", result);
        return;
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // 等待模式切换
    
    // 发送测试消息
    CAN_FRAME_t test_frame;
    test_frame.can_id = TEST_MSG_ID_1;
    test_frame.can_dlc = 8;
    memset(test_frame.data, 0xAA, 8);
    
    result = MCP2515_sendMessageAfterCtrlCheck(&test_frame);
    if (result != ERROR_OK) {
        ESP_LOGE(TAG, "Failed to send test message: %d", result);
        return;
    }
    
    ESP_LOGI(TAG, "Test message sent, waiting for reception...");
    
    // 等待接收消息
    TickType_t start_time = xTaskGetTickCount();
    bool message_received = false;
    
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(1000)) {
        if (MCP2515_checkReceive()) {
            CAN_FRAME_t received_frame;
            result = MCP2515_readMessageAfterStatCheck(&received_frame);
            if (result == ERROR_OK) {
                ESP_LOGI(TAG, "Loopback test PASSED - Message received: ID=0x%03X, DLC=%d", 
                         received_frame.can_id, received_frame.can_dlc);
                message_received = true;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    if (!message_received) {
        ESP_LOGE(TAG, "Loopback test FAILED - No message received within timeout");
    }
    
    // 恢复正常模式
    MCP2515_setNormalMode();
    vTaskDelay(pdMS_TO_TICKS(100));
}

// 发送测试消息
void can_send_test_messages(void)
{
    ESP_LOGI(TAG, "Starting CAN send test...");
    
    CAN_FRAME_t test_frames[3];
    
    // 标准帧测试消息1
    test_frames[0].can_id = TEST_MSG_ID_1;
    test_frames[0].can_dlc = 4;
    test_frames[0].data[0] = 0x11;
    test_frames[0].data[1] = 0x22;
    test_frames[0].data[2] = 0x33;
    test_frames[0].data[3] = 0x44;
    
    // 标准帧测试消息2
    test_frames[1].can_id = TEST_MSG_ID_2;
    test_frames[1].can_dlc = 8;
    for (int i = 0; i < 8; i++) {
        test_frames[1].data[i] = 0x50 + i;
    }
    
    // 扩展帧测试消息
    test_frames[2].can_id = TEST_MSG_ID_EXT | CAN_EFF_FLAG;
    test_frames[2].can_dlc = 6;
    for (int i = 0; i < 6; i++) {
        test_frames[2].data[i] = 0x60 + i;
    }
    
    // 发送所有测试消息
    for (int i = 0; i < 3; i++) {
        ERROR_t result = MCP2515_sendMessageAfterCtrlCheck(&test_frames[i]);
        if (result == ERROR_OK) {
            ESP_LOGI(TAG, "Test message %d sent successfully - ID: 0x%08X", 
                     i, test_frames[i].can_id);
        } else {
            ESP_LOGE(TAG, "Failed to send test message %d: %d", i, result);
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // 间隔100ms
    }
}

// 接收测试消息
void can_receive_test_messages(void)
{
    ESP_LOGI(TAG, "Starting CAN receive test...");
    
    TickType_t start_time = xTaskGetTickCount();
    uint32_t received_count = 0;
    
    while ((xTaskGetTickCount() - start_time) < pdMS_TO_TICKS(5000)) { // 5秒超时
        if (MCP2515_checkReceive()) {
            CAN_FRAME_t received_frame;
            ERROR_t result = MCP2515_readMessageAfterStatCheck(&received_frame);
            if (result == ERROR_OK) {
                received_count++;
                ESP_LOGI(TAG, "Received message %lu - ID: 0x%08X, DLC: %d, Data: %02X %02X %02X %02X %02X %02X %02X %02X",
                         received_count, received_frame.can_id, received_frame.can_dlc,
                         received_frame.data[0], received_frame.data[1], received_frame.data[2], received_frame.data[3],
                         received_frame.data[4], received_frame.data[5], received_frame.data[6], received_frame.data[7]);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    ESP_LOGI(TAG, "Receive test completed - Received %lu messages", received_count);
}

// 错误测试
void can_error_test(void)
{
    ESP_LOGI(TAG, "Starting CAN error test...");
    
    // 检查当前错误状态
    uint8_t error_flags = MCP2515_getErrorFlags();
    ESP_LOGI(TAG, "Current error flags: 0x%02X", error_flags);
    
    // 尝试发送无效消息（DLC > 8）
    CAN_FRAME_t invalid_frame;
    invalid_frame.can_id = 0x999;
    invalid_frame.can_dlc = 10; // 无效DLC
    memset(invalid_frame.data, 0xFF, 8);
    
    ERROR_t result = MCP2515_sendMessageAfterCtrlCheck(&invalid_frame);
    if (result == ERROR_FAILTX) {
        ESP_LOGI(TAG, "Error test PASSED - Invalid message correctly rejected");
    } else {
        ESP_LOGE(TAG, "Error test FAILED - Invalid message was accepted");
    }
    
    // 检查错误计数器
    uint8_t tec = MCP2515_readRegister(MCP_TEC);
    uint8_t rec = MCP2515_readRegister(MCP_REC);
    ESP_LOGI(TAG, "Error counters - TEC: %d, REC: %d", tec, rec);
}

// 性能测试
void can_performance_test(void)
{
    ESP_LOGI(TAG, "Starting CAN performance test...");
    
    const uint32_t test_messages = 100;
    TickType_t start_time = xTaskGetTickCount();
    uint32_t sent_count = 0;
    uint32_t failed_count = 0;
    
    CAN_FRAME_t test_frame;
    test_frame.can_id = 0x100;
    test_frame.can_dlc = 8;
    
    for (uint32_t i = 0; i < test_messages; i++) {
        // 更新数据
        for (int j = 0; j < 8; j++) {
            test_frame.data[j] = (i + j) & 0xFF;
        }
        
        ERROR_t result = MCP2515_sendMessageAfterCtrlCheck(&test_frame);
        if (result == ERROR_OK) {
            sent_count++;
        } else {
            failed_count++;
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms间隔
    }
    
    TickType_t end_time = xTaskGetTickCount();
    uint32_t duration_ms = (end_time - start_time) * portTICK_PERIOD_MS;
    
    ESP_LOGI(TAG, "Performance test completed:");
    ESP_LOGI(TAG, "  Duration: %lu ms", duration_ms);
    ESP_LOGI(TAG, "  Messages sent: %lu", sent_count);
    ESP_LOGI(TAG, "  Messages failed: %lu", failed_count);
    ESP_LOGI(TAG, "  Success rate: %.1f%%", (float)sent_count / test_messages * 100);
    ESP_LOGI(TAG, "  Messages per second: %.1f", (float)sent_count / duration_ms * 1000);
}

// 过滤器测试
void can_filter_test(void)
{
    ESP_LOGI(TAG, "Starting CAN filter test...");
    
    // 设置过滤器 - 只接收ID为0x123的消息
    ERROR_t result = MCP2515_setFilter(RXF0, false, 0x123);
    if (result != ERROR_OK) {
        ESP_LOGE(TAG, "Failed to set filter: %d", result);
        return;
    }
    
    // 设置掩码 - 精确匹配
    result = MCP2515_setFilterMask(MASK0, false, 0x7FF);
    if (result != ERROR_OK) {
        ESP_LOGE(TAG, "Failed to set mask: %d", result);
        return;
    }
    
    ESP_LOGI(TAG, "Filter configured - only accepting messages with ID 0x123");
    
    // 发送应该被接收的消息
    CAN_FRAME_t accepted_frame;
    accepted_frame.can_id = 0x123;
    accepted_frame.can_dlc = 4;
    accepted_frame.data[0] = 0xAA;
    accepted_frame.data[1] = 0xBB;
    accepted_frame.data[2] = 0xCC;
    accepted_frame.data[3] = 0xDD;
    
    result = MCP2515_sendMessageAfterCtrlCheck(&accepted_frame);
    if (result == ERROR_OK) {
        ESP_LOGI(TAG, "Accepted message sent");
    }
    
    // 发送应该被过滤的消息
    CAN_FRAME_t filtered_frame;
    filtered_frame.can_id = 0x456;
    filtered_frame.can_dlc = 4;
    filtered_frame.data[0] = 0xFF;
    filtered_frame.data[1] = 0xEE;
    filtered_frame.data[2] = 0xDD;
    filtered_frame.data[3] = 0xCC;
    
    result = MCP2515_sendMessageAfterCtrlCheck(&filtered_frame);
    if (result == ERROR_OK) {
        ESP_LOGI(TAG, "Filtered message sent");
    }
    
    // 等待接收消息
    vTaskDelay(pdMS_TO_TICKS(500));
    
    uint32_t received_count = 0;
    while (MCP2515_checkReceive()) {
        CAN_FRAME_t received_frame;
        result = MCP2515_readMessageAfterStatCheck(&received_frame);
        if (result == ERROR_OK) {
            received_count++;
            ESP_LOGI(TAG, "Filter test - Received message with ID: 0x%03X", received_frame.can_id);
        }
    }
    
    ESP_LOGI(TAG, "Filter test completed - Received %lu messages", received_count);
    
    // 清除过滤器
    MCP2515_setFilter(RXF0, false, 0);
    MCP2515_setFilterMask(MASK0, false, 0);
} 