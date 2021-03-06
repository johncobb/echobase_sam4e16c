/*
 * wan_proc.c
 *
 *  Created on: Oct 27, 2015
 *      Author: jcobb
 */

#include <stdint.h>
#include <string.h>
#include <ctype.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "freertos_usart_serial.h"
#include "sysclk.h"

#include "board.h"
#include "ring_buffer.h"
#include "wan.h"
#include "wan_task.h"
#include "wan_msg.h"
#include "cobs.h"
#include "ramdisk.h"
#include "wan_proc.h"

static uint8_t encoded_buffer[COBS_MSG_LEN] = {0};
static uint8_t decoded_buffer[COBS_BUFFER_LEN] = {0};
static tag_msg_t *tag_next_msg = NULL;
static tag_msg_t *tag_msg;



static void monitor_ramdisk(void);
static void log_tag_buffer(uint8_t * buffer);

void create_wan_proc_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{
	ramdisk_init();

	xTaskCreate(	wan_proc_task,						/* The task that implements the command console. */
					(const int8_t *const) "WAN_PRC",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,								/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);
}



// handle cobs encoded messages from xWanEncodedQueue
static void wan_proc_task(void *pvParameters)
{
	BaseType_t result;
	tag_msg_t tag_msg;
	anchor_msg_t anchor_msg;

	while (true) {
		// initialize buffers
		memset(encoded_buffer, '\0', COBS_MSG_LEN);
		memset(decoded_buffer, '\0', COBS_MSG_LEN);

		// read messages enqueued to xWanEncodedQueue from wan_task.c
		result = xQueueReceive(xWanEncodedQueue, encoded_buffer, WAN_QUEUE_TICKS);

		if(result == pdTRUE) {

			// decode the message
			decode_cobs(encoded_buffer, COBS_MSG_LEN, decoded_buffer);

			uint8_t cmd = decoded_buffer[0];

			// tmp_buffer used for temporary printf logging
			static uint8_t tmp_buffer[128] = {0};

			switch (cmd) {
				case TAG:

#ifdef LOG_MSG_COBSDECODED
					log_tag_buffer(decoded_buffer);
#endif

					parse_tag_msg(decoded_buffer, &tag_msg);

					// special logging code here
//					memset(tmp_buffer, '\0', 128);
//					wan_tagmsg_toascii(&tag_msg, tmp_buffer);
//					printf(tmp_buffer);
//					printf("\r\n");

					result = xQueueSendToBack( xWanMessagesQueue, &tag_msg, (TickType_t)0);

					if(result == pdTRUE) {
//						printf("enqueued successfully\r\n");
					} else {
						printf("failed to enqueue!\r\n");
					}

//					struct record_t record;
//					record.wan_msg = tag_msg;
//
//					// write to ramdisk
//					ramdisk_write(record);
					break;
				case ANCHOR_STATUS:

					parse_anchor_msg(decoded_buffer, &anchor_msg);

					// special logging code here
//					memset(tmp_buffer, '\0', 128);
//					wan_anchormsg_toascii(&anchor_msg, tmp_buffer);
//					printf(tmp_buffer);
//					printf("\r\n");

					result = xQueueSendToBack( xWanMessagesQueue, &anchor_msg, (TickType_t)0);

					if(result == pdTRUE) {
//						printf("enqueued successfully\r\n");
					} else {
						printf("failed to enqueue!\r\n");
					}

					break;
			}

			// process the message
		}
		vTaskDelay(50);
	}


}


void parse_tag_msg(uint8_t *data, tag_msg_t *tag)
{
	memcpy(tag, data, sizeof(tag_msg_t));
}

void parse_anchor_msg(uint8_t *data, anchor_msg_t *msg)
{
	memcpy(msg, data, sizeof(anchor_msg_t));
}


static void log_tag_buffer(uint8_t * decoded_buffer)
{
	printf("***** decoded ****\r\n");
	printf("messageType: %02x\r\n", decoded_buffer[0]);

	printf("anchorMac: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",	decoded_buffer[8],
																		decoded_buffer[7],
																		decoded_buffer[6],
																		decoded_buffer[5],
																		decoded_buffer[4],
																		decoded_buffer[3],
																		decoded_buffer[2],
																		decoded_buffer[1]);

	printf("anchorShort: %02x:%02x\r\n", 	decoded_buffer[10],
											decoded_buffer[9]);

	printf("tagMac: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",	decoded_buffer[18],
																	decoded_buffer[17],
																	decoded_buffer[16],
																	decoded_buffer[15],
																	decoded_buffer[14],
																	decoded_buffer[13],
																	decoded_buffer[12],
																	decoded_buffer[11]);

	printf("tagConfigSet: %02x:%02x\r\n",	decoded_buffer[20],
											decoded_buffer[19]);

	printf("tagSerial: %02x:%02x\r\n",		decoded_buffer[22],
											decoded_buffer[21]);

	printf("tagStatus: %02x:%02x\r\n",		decoded_buffer[24],
											decoded_buffer[23]);

	printf("tagLqi: %02x\r\n",	decoded_buffer[25]);

	printf("tagRssi: %02x\r\n",	decoded_buffer[26]);

	printf("tagBattery: %02x:%02x:%02x:%02x\r\n",	decoded_buffer[30],
													decoded_buffer[29],
													decoded_buffer[28],
													decoded_buffer[27]);

	printf("tagTemperature: %02x:%02x:%02x:%02x\r\n",	decoded_buffer[34],
														decoded_buffer[33],
														decoded_buffer[32],
														decoded_buffer[31]);
}


