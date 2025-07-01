#ifndef CAN_TEST_H_
#define CAN_TEST_H_

#include "can.h"
#include "mcp2515.h"
#include "esp_log.h"

// 测试消息ID
#define TEST_MSG_ID_1          0x123
#define TEST_MSG_ID_2          0x456
#define TEST_MSG_ID_EXT        0x18FF1234

// 测试函数声明
void can_loopback_test(void);
void can_send_test_messages(void);
void can_receive_test_messages(void);
void can_error_test(void);
void can_performance_test(void);
void can_filter_test(void);

// 测试状态
typedef enum {
    TEST_PASS = 0,
    TEST_FAIL = 1,
    TEST_TIMEOUT = 2
} test_result_t;

// 测试配置
typedef struct {
    uint32_t test_duration_ms;
    uint32_t message_interval_ms;
    uint32_t expected_messages;
    bool enable_loopback;
    bool enable_filters;
} can_test_config_t;

#endif /* CAN_TEST_H_ */ 