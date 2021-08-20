/*
 * This file is part of the EasyLogger Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-04-28
 */
#include <stdio.h>
#include <string.h>
#include "elog.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

static SemaphoreHandle_t output_lock;

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
TaskHandle_t async_thread_handler = NULL;
static void async_output(void *arg);
#endif

/**
 * EasyLogger port initialize
 *
 * @return result
 */
ElogErrCode elog_port_init(void) {
    ElogErrCode result = ELOG_NO_ERR;

    output_lock = xSemaphoreCreateMutex();
    configASSERT(output_lock != NULL);

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
    BaseType_t xReturned  = xTaskCreate(async_output, "elog_async", 1024, NULL, configMAX_PRIORITIES - 1, &async_thread_handler);
    configASSERT(xReturned  == pdPASS);
#endif
    
    return result;
}

/**
 * EasyLogger port deinitialize
 *
 */
void elog_port_deinit(void) {
}

/**
 * output log port interface
 *
 * @param log output of log
 * @param size log size
 */
void elog_port_output(const char *log, size_t size) {
	printf("%.*s", size, log);
}

/**
 * output lock
 */
void elog_port_output_lock(void) {
	xSemaphoreTake(output_lock, portMAX_DELAY);
}

/**
 * output unlock
 */
void elog_port_output_unlock(void) {
	xSemaphoreGive(output_lock);
}

/**
 * get current time interface
 *
 * @return current time
 */
const char *elog_port_get_time(void) {
	static char cur_system_time[16] = { 0 };
	snprintf(cur_system_time, 16, "tick:%010lud", xTaskGetTickCount());
	return cur_system_time;
}

/**
 * get current process name interface
 *
 * @return current process name
 */
const char *elog_port_get_p_info(void) {
    return "";
}

/**
 * get current thread name interface
 *
 * @return current thread name
 */
const char *elog_port_get_t_info(void) {
	static char name[configMAX_TASK_NAME_LEN];
	char* taskName = pcTaskGetName(xTaskGetCurrentTaskHandle());
	strncpy(name, taskName, sizeof(name));
	return name;
}

#ifdef ELOG_ASYNC_OUTPUT_ENABLE
void elog_async_output_notice(void) {
	xTaskNotifyGive(async_thread_handler);
}

static void async_output(void *arg) {
    size_t get_log_size = 0;
    static char poll_get_buf[ELOG_LINE_BUF_SIZE - 4];

    while(true) {
        /* waiting log */
    	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        /* polling gets and outputs the log */
        while(true) {

#ifdef ELOG_ASYNC_LINE_OUTPUT
            get_log_size = elog_async_get_line_log(poll_get_buf, sizeof(poll_get_buf));
#else
            get_log_size = elog_async_get_log(poll_get_buf, sizeof(poll_get_buf));
#endif

            if (get_log_size) {
                elog_port_output(poll_get_buf, get_log_size);
            } else {
                break;
            }
        }
    }
}
#endif
