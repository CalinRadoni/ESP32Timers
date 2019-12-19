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

#ifndef ESP32Timers_H
#define ESP32Timers_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/timer.h"

struct ESP32TimerEvent
{
    timer_group_t group;
    timer_idx_t   index;
};

/**
 * Usage in a dedicated thread:
 * <pre>{@code
 * while (true) {
 *     ESP32TimerEvent event;
 *     xQueueReceive(timers.timerQueue, &event, portMAX_DELAY);
 *
 *     // handle the timer event
 * }
 * }</pre>
 */
class ESP32Timers
{
public:
    ESP32Timers(void);
    ~ESP32Timers(void);

    /**
     * @brief The queue for timer events
     *
     * The queue is created by the Create function and destroyed by the Destroy function or on the destructor
     */
    QueueHandle_t timerQueue;

    bool Create(void);
    void Destroy(void);

    /**
     * @brief Create and start a timer
     *
     * ESP32 has two groups of two timers:
     * - timer_group_t::TIMER_GROUP_0, timer_idx_t::TIMER_0
     * - timer_group_t::TIMER_GROUP_0, timer_idx_t::TIMER_1
     * - timer_group_t::TIMER_GROUP_1, timer_idx_t::TIMER_0
     * - timer_group_t::TIMER_GROUP_1, timer_idx_t::TIMER_1;
     *
     * @param group      Timer group
     * @param index      Timer index
     * @param periodMS   Sets the timer's periond in miliseconds
     * @param autoreload Autoreload the timer
     * @param singleShot Single shot or repeated triggering
     */
    bool CreateTimer(uint8_t group, uint8_t index, uint32_t periodMS, bool autoreload, bool singleShot);

    void DestroyTimer(uint8_t group, uint8_t index);

protected:
    // nothing here for now
};

extern ESP32Timers timers;

#endif
