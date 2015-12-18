/*
 * app_task.h
 *
 *  Created on: Jul 22, 2015
 *      Author: jcobb
 */

#ifndef APP_TASK_H_
#define APP_TASK_H_




QueueHandle_t xAppMessageQueue;
extern xSemaphoreHandle app_start_signal;


void app_start(void);

void create_app_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority);
void app_task_unittest(void);

#endif /* APP_TASK_H_ */
