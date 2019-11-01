/**
This file is part of ESP32Timers esp-idf component from
pax-devices (https://github.com/CalinRadoni/pax-devices)
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

#include "ESP32Timers.h"

// -----------------------------------------------------------------------------

ESP32Timers timers;

// -----------------------------------------------------------------------------

const uint8_t queueLength = 8;

struct isrTimerInfo
{
    timer_group_t group;
    timer_idx_t   index;
    bool          single;
};

void IRAM_ATTR timer_ISR(void *param) {
    isrTimerInfo* arg = (isrTimerInfo*) param;
    if (arg == nullptr) return;

    timg_dev_t *timerGroup = nullptr;
    if (arg->group == timer_group_t::TIMER_GROUP_0) {
        timerGroup = &TIMERG0;
    }
    else {
        timerGroup = &TIMERG1;
    }

    // clear the interrupt status bit
    if (arg->index == timer_idx_t::TIMER_0) {
        timerGroup->int_clr_timers.t0 = 1;
    }
    else {
        timerGroup->int_clr_timers.t1 = 1;
    }

    // enable alarm if needed
    if (!arg->single) {
        if (arg->index == timer_idx_t::TIMER_0) {
            timerGroup->hw_timer[0].config.alarm_en = TIMER_ALARM_EN;
        }
        else {
            timerGroup->hw_timer[1].config.alarm_en = TIMER_ALARM_EN;
        }
    }

    ESP32TimerEvent event;
    event.group = arg->group;
    event.index = arg->index;
    if (timers.timerQueue != 0)
        xQueueSendFromISR(timers.timerQueue, &event, NULL);
}

// -----------------------------------------------------------------------------

ESP32Timers::ESP32Timers(void)
{
    timerQueue = 0;
}

ESP32Timers::~ESP32Timers(void)
{
    Destroy();
}

bool ESP32Timers::Create(void)
{
    timerQueue = xQueueCreate(queueLength, sizeof(isrTimerInfo));

    return timerQueue == 0 ? false : true;
}

void ESP32Timers::Destroy(void)
{
    DestroyTimer(0, 0);
    DestroyTimer(0, 1);
    DestroyTimer(1, 0);
    DestroyTimer(1, 1);

    if (timerQueue != 0) {
        vQueueDelete(timerQueue);
        timerQueue = 0;
    }
}

void ESP32Timers::DestroyTimer(uint8_t group, uint8_t index)
{
    isrTimerInfo arg;
    arg.group = (timer_group_t) group;
    arg.index = (timer_idx_t) index;

    if (arg.group >= timer_group_t::TIMER_GROUP_MAX) return;
    if (arg.index >= timer_idx_t::TIMER_MAX) return;

    timer_pause(arg.group, arg.index);
    timer_disable_intr(arg.group, arg.index);
    timer_set_alarm(arg.group, arg.index, timer_alarm_t::TIMER_ALARM_DIS);
}

bool ESP32Timers::CreateTimer(uint8_t group, uint8_t index, uint32_t periodMS, bool autoreload, bool singleShot)
{
    DestroyTimer(group, index);

    isrTimerInfo arg;
    arg.group  = (timer_group_t) group;
    arg.index  = (timer_idx_t) index;
    arg.single = singleShot;

    if (arg.group >= timer_group_t::TIMER_GROUP_MAX) return false;
    if (arg.index >= timer_idx_t::TIMER_MAX) return false;

    timer_config_t config;
    config.alarm_en    = false;
    config.counter_en  = false;
    config.intr_type   = timer_intr_mode_t::TIMER_INTR_LEVEL;
    config.counter_dir = timer_count_dir_t::TIMER_COUNT_UP;
    config.auto_reload = autoreload ? timer_autoreload_t::TIMER_AUTORELOAD_EN : timer_autoreload_t::TIMER_AUTORELOAD_DIS;
    config.divider     = 80; // 80 MHz / 80 -> 1 MHz timer clock

    esp_err_t err = timer_init(arg.group, arg.index, &config);
    if (err != ESP_OK) return false;

    err = timer_set_counter_value(arg.group, arg.index, 0ULL);
    if (err != ESP_OK) return false;

    uint64_t alarmValue = periodMS;
    alarmValue *= 80000;
    alarmValue /= config.divider;

    err = timer_set_alarm_value(arg.group, arg.index, alarmValue);
    if (err != ESP_OK) return false;

    err = timer_set_alarm(arg.group, arg.index, timer_alarm_t::TIMER_ALARM_EN);
    if (err != ESP_OK) return false;

    err = timer_enable_intr(arg.group, arg.index);
    if (err != ESP_OK) return false;

    err = timer_isr_register(arg.group, arg.index, timer_ISR, &arg, ESP_INTR_FLAG_IRAM, NULL);
    if (err != ESP_OK) return false;

    err = timer_start(arg.group, arg.index);
    if (err != ESP_OK) return false;

    return true;
}
