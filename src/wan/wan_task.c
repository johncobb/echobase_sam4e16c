/*
 * wan_task.c
 *
 *  Created on: Aug 19, 2015
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
#include "router_status_msg.h"
#include "wan_msg.h"
#include "cobs.h"
#include "ramdisk.h"

xSemaphoreHandle wan_start_signal = 0;

QueueHandle_t xWanQueue;

static usart_ring_buffer_t usart_buffer = {{0},0,0};

static void wan_handler_task(void *pvParameters);
static void wan_queue_task(void *pvParameters);
static void wan_handler_usart(void);
static void wan_handler_usart_helloworld(void);
static void parse_tag(uint8_t *data, tag_msg_t *tag);
static void parse_router_status(uint8_t *data, router_status_msg_t *msg);

static volatile uint32_t byte_count = 0;

static BaseType_t result;
static tag_msg_t *tag_next_msg = NULL;
static tag_msg_t *tag_msg;

void create_wan_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{

	wan_init();

	vSemaphoreCreateBinary(wan_start_signal);

	xWanQueue = xQueueCreate(10, COBS_BUFFER_LEN);

	xTaskCreate(	wan_handler_task,			/* The task that implements the command console. */
					(const int8_t *const) "WAN",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,			/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);

}

void create_wan_queue_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{
	ramdisk_init();

	xTaskCreate(	wan_queue_task,			/* The task that implements the command console. */
					(const int8_t *const) "WAN_PRC",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,			/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);
}



static volatile uint8_t cobs_buffer_index = 0;
static uint8_t cobs_buffer[COBS_BUFFER_LEN] = {0};
static uint8_t decoded_msg[COBS_BUFFER_LEN] = {0};
static uint8_t decoded_buffer[COBS_BUFFER_LEN] = {0};

#define TOKEN_END	'O'

// handle incoming cobs encoded messages from the usart
// once decoded enqueue to the xWanQueue
static void wan_handler_task(void *pvParameters)
{

	memset(cobs_buffer, '\0', COBS_BUFFER_LEN+1);

	printf("wan task started...\r\n");
	while(true) {

		// read the usart
		wan_handler_usart();

		// check for data
		if(usart_data_available(&usart_buffer)) {

			// read one byte
			uint8_t c = usart_data_read(&usart_buffer);
			// store in buffer
			cobs_buffer[cobs_buffer_index++] = c;

			// check to see if we reached a delimeter
			if(c == ((uint8_t) TOKEN_END)) {

				char * ptr = NULL;

				if((ptr = strstr(cobs_buffer, "HELLOWORLD"))) {
					cobs_buffer_index = 0;
					printf("hello world found\r\n");
				}

				// reset the buffer
				memset(cobs_buffer, '\0', COBS_BUFFER_LEN+1);

			}
		}


		vTaskDelay(100);

	}
}

//static void wan_handler_usart(void)
//{
//	memset(wan_rx_buffer, '\0', WAN_RX_BUFFER_SIZE+1);
//
//	byte_count = wan_handler_async(20);
//
//	if(byte_count > 0) {
//		printf("wan bytes_received: %lu\r\n", byte_count);
//		printf("wan_rx_buffer: %s\r\n", wan_rx_buffer);
//		printf("WAN task...\r\n");
//
//		for (int i=0; i<byte_count-1; i++) {
//
//			uint8_t b = wan_rx_buffer[i];
//
//			if(b == 0x00) {
//				decode_cobs(cobs_buffer, sizeof(cobs_buffer), decoded_msg);
//				result = xQueueSendToBack(xWanQueue, decoded_msg, 0);
//				cobs_buffer_index = 0;
//			} else {
//
//				if(cobs_buffer_index > COBS_BUFFER_LEN) {
//					cobs_buffer_index = 0;
//				}
//
//				cobs_buffer[cobs_buffer_index++] = b;
//			}
//		}
//	}
//}




static void wan_handler_usart(void)
{

	// reset the wan_rx_buffer before starting
	memset(wan_rx_buffer, '\0', WAN_RX_BUFFER_SIZE+1);

	// read the usart
	byte_count = wan_handler_async(10);

	// do we have data?
	if(byte_count > 0) {
		for (int i=0; i<byte_count-1; i++) {

			uint8_t c = wan_rx_buffer[i];

			// sanity check ;)
			if(isascii(c))
				usart_put_char(&usart_buffer, c);
		}
	}

}

// handle messages encoded to the xWanQueue
static void wan_queue_task(void *pvParameters)
{
	BaseType_t result;
	tag_msg_t tag_msg;
	router_msg_t router_msg;

	result = xQueueReceive(xWanQueue, decoded_buffer, WAN_QUEUE_TICKS);

	if(result == pdTRUE) {

		uint8_t cmd;

		cmd = decoded_buffer[0];

		switch (cmd) {
			case TAG:
				parse_tag(decoded_buffer, &tag_msg);
				// save to ramdisk
				break;
			case ROUTER_STATUS:
				parse_router_status(decoded_buffer, &router_msg);
				// enqueue into modem
				break;
		}

		// process the message
	}

	memset(decoded_buffer, 0, COBS_BUFFER_LEN);
}

static void parse_tag(uint8_t *data, tag_msg_t *tag)
{
	memcpy(tag, data, sizeof(tag_msg_t));
}

static void parse_router_status(uint8_t *data, router_status_msg_t *msg)
{
	memcpy(msg, data, sizeof(router_status_msg_t));
}



void handle_monitor_ramdisk(void)
{
	// handle pruning of ramdisk
	// we want to remove tags that have moved out of range of the network
	if(tag_next_msg != NULL) {
		tag_msg = tag_next_msg;

		if((clock_time() - tag_msg->lastSent) > MAX_TAG_MSG_AGE) {
			// possibly enqueue a message to server stating tag out of range
//			result = xQueueSendToBack(xModemQueue, tag_msg, 0);
			// remove message from ramdisk
			ramdisk_erase(*tag_msg);
		}


	} else {
		ramdisk_next(NULL, tag_next_msg);
	}
}


void process_ramdisk_msg(void)
{
	tag_msg_t tag_msg;

	tag_msg_t *target;
	tag_msg_t *result = NULL;


	ramdisk_find(target->tagMac, result);

	// tag not found in ramdisk
	if(result == NULL) {
		tag_msg.lastSent = clock_time();
		ramdisk_write(tag_msg);
	} else {
		// tag found so update current attributes
		result->tagRssi = tag_msg.tagRssi;
		result->tagBattery = tag_msg.tagBattery;
		result->tagTemperature = tag_msg.tagTemperature;

		// message was not found so enqueue to modem for transmitting
//		result = xQueueSendToBack(xModemQueue, tag_msg, 0);
	}
}
