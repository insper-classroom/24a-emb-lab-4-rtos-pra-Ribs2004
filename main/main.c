/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

const uint TRIG_PIN = 6;
const uint ECHO_PIN = 5;

QueueHandle_t xQueueTime, xQueueDistance;
SemaphoreHandle_t xSemaphore;

void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);

    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);

    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);

    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);

    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}

void oled1_demo_1(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    char cnt = 15;
    while (1) {

        if (gpio_get(BTN_1_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_1_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 1 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_2_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_2_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 2 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_3_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_3_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 3 - ON");
            gfx_show(&disp);
        } else {

            gpio_put(LED_1_OLED, 1);
            gpio_put(LED_2_OLED, 1);
            gpio_put(LED_3_OLED, 1);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "PRESSIONE ALGUM");
            gfx_draw_string(&disp, 0, 10, 1, "BOTAO");
            gfx_draw_line(&disp, 15, 27, cnt,
                          27);
            vTaskDelay(pdMS_TO_TICKS(50));
            if (++cnt == 112)
                cnt = 15;

            gfx_show(&disp);
        }
    }
}

void oled1_demo_2(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    while (1) {

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 1, "Mandioca");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 2, "Batata");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 4, "Inhame");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}


void task_trigger(void *p) {
    gpio_init(TRIG_PIN);
    gpio_set_dir(TRIG_PIN, GPIO_OUT);

    while (true) {
        gpio_put(TRIG_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_put(TRIG_PIN, 0);
        xSemaphoreGive(xSemaphore);
        vTaskDelay(pdMS_TO_TICKS(990));
    }
}

void gpioCallback(uint gpio, uint32_t events) {

    uint32_t time;
    if (events == 0x4) {
        time = to_us_since_boot(get_absolute_time());
    } else if (events == 0x8) {
        time = to_us_since_boot(get_absolute_time());
    }
    xQueueSendFromISR(xQueueTime, &time, 0);
}

void task_echo(void *p) {
    gpio_init(ECHO_PIN);
    gpio_set_dir(ECHO_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(ECHO_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpioCallback);

    int times_read = 0;
    uint32_t trigger_times[2];

    while(true) {
        if (xQueueReceive(xQueueTime, &trigger_times[times_read], pdMS_TO_TICKS(1000)) == pdTRUE) {
            times_read++;
            double distance;
            if (times_read > 1) {
                distance = (trigger_times[1] - trigger_times[0]) * 0.01715;
                xQueueSend(xQueueDistance, &distance, 0);
                times_read = 0;
            }
        }  else {
                double distance = -1000.0;
                xQueueSend(xQueueDistance, &distance, 0);
                times_read = 0;
            }
    }
    
}

void task_oled(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    char distanceStr[12];
    double distance;
    const int maxWidth = 128;

    while (1) {
        if (xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
            vTaskDelay(100);
            if (xQueueReceive(xQueueDistance, &distance, pdMS_TO_TICKS(100)) == pdTRUE) {
                if (distance == -1000.0) {
                    gfx_clear_buffer(&disp);
                    gfx_draw_string(&disp, 0, 0, 2, "Falha");
                    gfx_show(&disp);
                    vTaskDelay(pdMS_TO_TICKS(150));
                } else {
                    gfx_clear_buffer(&disp);
                    snprintf(distanceStr, sizeof(distanceStr), "Dist: %d", (int) distance);
                    gfx_draw_string(&disp, 0, 0, 2, distanceStr);
                    int barLength = (int)((distance / 300.0) * maxWidth);
                    if (barLength > maxWidth) {
                        barLength = maxWidth;
                    }
                    gfx_draw_line(&disp, 0, 31, barLength, 31);
                    gfx_show(&disp);
                    vTaskDelay(pdMS_TO_TICKS(150));
                }
            }
        }
    }
}


int main() {
    stdio_init_all();

    xQueueTime = xQueueCreate(32, sizeof(uint32_t));
    xQueueDistance = xQueueCreate(32, sizeof(double));

    xSemaphore = xSemaphoreCreateBinary();

    xTaskCreate(task_oled, "OLED", 4095, NULL, 1, NULL);
    xTaskCreate(task_trigger, "Trigger", 4095, NULL, 1, NULL);
    xTaskCreate(task_echo, "Echo", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
