/**
This file is part of ESP32Timers esp-idf component
(https://github.com/CalinRadoni/ESP32Timers)
Copyright (C) 2019+ by Calin Radoni

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "ESP32Timers.h"

const char* TAG = "example";

static const uint32_t timerPeriod = 5; // ms
static const uint32_t timerPPS = 1000 / timerPeriod;

extern "C" {

    static void TimerTask(void *taskParameter) {
        uint32_t timerTicks = 0;
        ESP32TimerEvent timerEvent;

        for(;;) {
            if (xQueueReceive(timers.timerQueue, &timerEvent, portMAX_DELAY) == pdPASS) {
                if (timerEvent.group == timer_group_t::TIMER_GROUP_0) {
                    if (timerEvent.index == timer_idx_t::TIMER_0) {
                        timerTicks++;
                        if (timerTicks >= timerPPS) {
                            timerTicks -= timerPPS;
                            ESP_LOGI(TAG, "a second passed");
                        }
                    }
                }
            }
        }

        // the next lines are here only for "completion"
        timers.DestroyTimer(timer_group_t::TIMER_GROUP_0, timer_idx_t::TIMER_0);
        timers.Destroy();
        vTaskDelete(NULL);
    }

    void app_main()
    {
        if (!timers.Create()) {
            ESP_LOGE(TAG, "Failed to create the timers object !");
            esp_restart();
        }

        if (!timers.CreateTimer(timer_group_t::TIMER_GROUP_0, timer_idx_t::TIMER_0, timerPeriod, true, false)) {
            ESP_LOGE(TAG, "Failed to create the timer G0T0 !");
            esp_restart();
        }

        TaskHandle_t xHandleTimerTask = NULL;
        xTaskCreate(TimerTask, "Timer handling task", 2048, NULL, uxTaskPriorityGet(NULL) + 5, &xHandleTimerTask);
        if (xHandleTimerTask != NULL) {
            ESP_LOGI(TAG, "Timer task created.");
        }
        else {
            ESP_LOGE(TAG, "Failed to create the timer task !");
            esp_restart();
        }
    }
}
