/*
 * wan_task.h
 *
 *  Created on: Aug 19, 2015
 *      Author: jcobb
 */

#ifndef WAN_TASK_H_
#define WAN_TASK_H_

#include "ring_buffer.h"


#define WAN_TOKEN_END		0
#define WAN_QUEUE_TICKS		16


static usart_ring_buffer_t wan_ring_buffer;


void create_app_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority);

#endif /* WAN_TASK_H_ */
