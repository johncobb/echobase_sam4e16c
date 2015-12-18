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
#include "wan_msg.h"
#include "cobs.h"
#include "ramdisk.h"

//#define RUN_HELLOWWORLD_TEST	1

xSemaphoreHandle wan_start_signal = 0;

QueueHandle_t xWanQueue;
QueueHandle_t xWanEncodedQueue;
QueueHandle_t xWanMessagesQueue;
QueueHandle_t xWanHelloWorldQueue;

static usart_ring_buffer_t wan_ring_buffer = {{0},0,0};

static void wan_handler_task(void *pvParameters);
static void wan_handler_usart(void);

void wan_handler_messages(void);

#ifdef RUN_HELLOWWORLD_TEST
void wan_handler_helloworld(void);
#endif


static volatile uint32_t byte_count = 0;

static BaseType_t result;

void create_wan_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{

	wan_init();

	vSemaphoreCreateBinary(wan_start_signal);

	xWanQueue = xQueueCreate(10, COBS_MSG_LEN);
	xWanEncodedQueue = xQueueCreate(10, COBS_MSG_LEN);
	xWanMessagesQueue = xQueueCreate(10, COBS_MSG_LEN);

#ifdef RUN_HELLOWWORLD_TEST
//	xWanHelloWorldQueue = xQueueCreate(10, COBS_MSG_LEN);
#endif

	xTaskCreate(	wan_handler_task,			/* The task that implements the command console. */
					(const int8_t *const) "WAN",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,			/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);

}





typedef struct
{
	uint16_t count;
	uint8_t msg[10+1];

}hello_world_msg_t;

static volatile uint8_t cobs_buffer_index = 0;
static uint8_t cobs_buffer[COBS_BUFFER_LEN+1] = {0};
static uint8_t cobs_encoded_msg[COBS_MSG_LEN] = {0};


// handle incoming cobs encoded messages from the usart
// once decoded enqueue to the xWanQueue
static void wan_handler_task(void *pvParameters)
{
#ifdef RUN_HELLOWWORLD_TEST
	wan_handler_helloworld();
#else
	wan_handler_messages();

#endif

}

static void wan_handler_usart(void)
{
	// reset the wan_rx_buffer before starting
	memset(wan_rx_buffer, '\0', WAN_RX_BUFFER_SIZE+1);

	// read the usart
	byte_count = wan_handler_async(10);

	// do we have data?
	if(byte_count > 0) {
		for (int i=0; i<byte_count; i++) {

			uint8_t c = wan_rx_buffer[i];

			usart_put_char(&wan_ring_buffer, c);
		}
	}
}

void wan_handler_messages(void)
{
	// initialize buffers
	memset(cobs_buffer, '\0', COBS_BUFFER_LEN);
	memset(cobs_encoded_msg, '\0', COBS_MSG_LEN);

	printf("wan task started...\r\n");
	while(true) {

		// read the usart
		wan_handler_usart();

		// check for data
		uint8_t byte_count = usart_data_available(&wan_ring_buffer);
//		printf("byte_count: %d\r\n", byte_count);

		for(int i=0; i<byte_count; i++) {

			// read one byte
			uint8_t c = usart_data_read(&wan_ring_buffer);
			// store in buffer
			cobs_buffer[cobs_buffer_index++] = c;

			// check to see if we reached a delimeter
			if(c == WAN_TOKEN_END) {

//				printf("wan message found\r\n");
//				printf("msg found buffer_index: %d\r\n", cobs_buffer_index);


//				printf("msg: %02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n",	cobs_buffer[0],
//																		cobs_buffer[1],
//																		cobs_buffer[2],
//																		cobs_buffer[3],
//																		cobs_buffer[4],
//																		cobs_buffer[5],
//																		cobs_buffer[6],
//																		cobs_buffer[7],
//																		cobs_buffer[8]);

				memcpy(cobs_encoded_msg, cobs_buffer, COBS_MSG_LEN);

				// enqueue messages to xWanEncodedQueue for parsing by wan_proc.c
				result = xQueueSendToBack( xWanEncodedQueue, cobs_encoded_msg, (TickType_t)0);

				if(result == pdTRUE) {
//					printf("enqueued successfully\r\n");
				} else {
					printf("failed to enqueue\r\n");
				}

				// reset the buffers
				memset(cobs_encoded_msg, '\0', COBS_MSG_LEN);
				memset(cobs_buffer, '\0', COBS_BUFFER_LEN+1);
				// reset the buffer index
				cobs_buffer_index = 0;

			}
		}

		vTaskDelay(100);

	}
}






#ifdef RUN_HELLOWWORLD_TEST

static char * ptr = NULL;
uint16_t hello_world_count = 0;
#define TOKEN_END '\r'

void wan_handler_helloworld(void)
{
	memset(cobs_buffer, '\0', COBS_BUFFER_LEN+1);

	printf("wan task started...\r\n");
	while(true) {

		// read the usart
		wan_handler_usart();

		// check for data
		uint8_t byte_count = usart_data_available(&wan_ring_buffer);

		for(int i=0; i<byte_count; i++) {

			// read one byte
			uint8_t c = usart_data_read(&wan_ring_buffer);
			// store in buffer
			cobs_buffer[cobs_buffer_index++] = c;

			// check to see if we reached a delimeter
			if(c == ((uint8_t) TOKEN_END)) {

				if((ptr = strstr(cobs_buffer, "HELLOWORLD"))) {
					cobs_buffer_index = 0;
					printf("hello found!\r\n");

					BaseType_t result;
					hello_world_msg_t msg = {0, {0}};

					msg.count = hello_world_count++;
					memcpy(msg.msg, cobs_buffer, COBS_MSG_LEN);
#ifdef RUN_HELLOWWORLD_TEST
//					result = xQueueSendToBack( xWanHelloWorldQueue, &msg, (TickType_t)0);
//
//					if(result == pdTRUE) {
//						printf("enqueued successfully\r\n");
//					} else {
//						printf("failed to enqueue\r\n");
//					}
#endif
				}

				// reset the buffer
				memset(cobs_buffer, '\0', COBS_BUFFER_LEN+1);

			}
		}

		vTaskDelay(500);

	}
}
#endif








