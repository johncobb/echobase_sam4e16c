/*
 * test_wan.c
 *
 *  Created on: Dec 17, 2015
 *      Author: jcobb
 */
#include <stdint.h>
#include <string.h>


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "comm.h"
#include "wan.h"
#include "wan_msg.h"
#include "wan_proc.h"
#include "unit_test.h"

void run_wan_test(void);

static uint8_t tmp_buffer[TAGMSG_ASCII_SIZE] = {0};
static uint8_t printf_buffer[TAGMSG_ASCII_SIZE] = {0};

void test_wan(void)
{

	printf(UNITTEST_MESSAGE_WAN);

	while (true) {
		run_wan_test();
		vTaskDelay(100);
	}

}
void run_wan_test(void)
{
	BaseType_t result;

	tag_msg_t *tag_msg;
	anchor_msg_t *anchor_msg;

	uint8_t cmd;

	memset(tmp_buffer, 0, sizeof(tmp_buffer));
	memset(printf_buffer, 0, sizeof(printf_buffer));

	result = xQueueReceive(xWanMessagesQueue, tmp_buffer, QUEUE_TICKS);

	if(result == pdTRUE) {

		cmd = tmp_buffer[0];

		if (cmd == TAG) {
			tag_msg = (tag_msg_t*) tmp_buffer;
			wan_tagmsg_toascii(tag_msg, printf_buffer);
			printf(printf_buffer);
			printf("\r\n");
			return;
		}

		if (cmd == ANCHOR_STATUS) {
			anchor_msg = (anchor_msg_t*) tmp_buffer;
			wan_anchormsg_toascii(anchor_msg, printf_buffer);
			printf(printf_buffer);
			printf("\r\n");
			return;
		}

	}

}
