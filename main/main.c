#include "board.h"

// 보드의 LED 핀 번호에 맞게 수정하세요 (보통 2, 8, 또는 13)
#define BLINK_GPIO 2 

void app_main(void)
{
    // 1. GPIO 핀 설정 (출력 모드)
    gpio_reset_pin(PIR_SENSOR_PIN);
    gpio_set_direction(PIR_SENSOR_PIN, GPIO_MODE_OUTPUT);

    ESP_LOGI("LED_TEST", "LED 블링크 테스트를 시작합니다!");

    while (1) {
        // 2. LED 켜기 (High)
        printf("LED ON\n");
        gpio_set_level(PIR_SENSOR_PIN, 1);
        vTaskDelay(500 / portTICK_PERIOD_MS); // 0.5초 대기

        // 3. LED 끄기 (Low)
        printf("LED OFF\n");
        gpio_set_level(PIR_SENSOR_PIN, 0);
        vTaskDelay(500 / portTICK_PERIOD_MS); // 0.5초 대기
    }
}