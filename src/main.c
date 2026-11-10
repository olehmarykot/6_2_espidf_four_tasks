#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Чим більше число, тим вищий пріоритет.
#define PRIORITY_LOW       1
#define PRIORITY_NORMAL    2
#define PRIORITY_HIGH      3
#define PRIORITY_REALTIME  4

static TaskHandle_t sensorHandle = NULL;

static void sensorReader(void *parameter)
{
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(500);

    while (1) {
        vTaskDelayUntil(&lastWake, interval);

        // Після Maintenance пропускаємо прострочені цикли,
        // не виконуючи кілька читань підряд.
        TickType_t now = xTaskGetTickCount();
        if ((TickType_t)(now - lastWake) >= interval) {
            lastWake = now;
        }

        printf("[Sensor] Data read...\n");
    }
}

static void workerTask(void *parameter)
{
    printf("[Worker] Processing event...\n");
    vTaskDelete(NULL);  // Видаляє саме цю задачу.
}

static void eventTrigger(void *parameter)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));

        BaseType_t result = xTaskCreate(
            workerTask, "Worker Task", 3072, NULL,
            PRIORITY_HIGH, NULL
        );

        if (result != pdPASS) {
            ESP_LOGE("Event", "Could not create Worker Task");
        }
    }
}

static void maintenanceMode(void *parameter)
{
    TickType_t lastWake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(10000));

        vTaskSuspend(sensorHandle);
        printf("[Maintenance] System paused for 2 seconds.\n");

        vTaskDelay(pdMS_TO_TICKS(2000));
        vTaskResume(sensorHandle);
    }
}

void app_main(void)
{
    // Спочатку створюємо Sensor, щоб отримати його handle.
    BaseType_t sensorCreated = xTaskCreate(
        sensorReader, "Sensor Reader", 3072, NULL,
        PRIORITY_NORMAL, &sensorHandle
    );

    if (sensorCreated != pdPASS) {
        ESP_LOGE("Main", "Could not create Sensor Reader");
        return;
    }

    if (xTaskCreate(eventTrigger, "Event Trigger", 3072, NULL,
                    PRIORITY_LOW, NULL) != pdPASS) {
        ESP_LOGE("Main", "Could not create Event Trigger");
    }

    if (xTaskCreate(maintenanceMode, "Maintenance Mode", 3072, NULL,
                    PRIORITY_REALTIME, NULL) != pdPASS) {
        ESP_LOGE("Main", "Could not create Maintenance Mode");
    }
}
