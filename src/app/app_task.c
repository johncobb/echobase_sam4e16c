/*
 * app_task.c
 *
 *  Created on: Jul 22, 2015
 *      Author: jcobb
 */

#include <stdint.h>
#include <string.h>


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "freertos_usart_serial.h"
#include "sysclk.h"
#include "comm.h"
#include "tcpip.h"
#include "wan.h"
#include "wan_task.h"
#include "app_task.h"
#include <cph.h>

QueueHandle_t xAppMessageQueue;
QueueHandle_t xTagMessageQueue;
QueueHandle_t xAnchorMessageQueue;
xSemaphoreHandle app_start_signal = 0;


static volatile bool start_task = false;
static volatile bool wait_ack = false;

static void app_handler_task(void *pvParameters);

static void app_handler(void);
static void app_handler_queue(void);
static void app_handler_msgqueue(void);
static void app_handler_anchorqueue(void);



static void log_tag_msg(tag_msg_t *msg);
static void log_anchor_msg(anchor_msg_t *msg);

static void app_data_handler(uint8_t *data, uint32_t len);
static sys_result parse_result(char * buffer, char * token, char ** ptr_out);



static uint8_t tmp_buffer[COBS_MSG_LEN] = {0};
static uint8_t socket_ascii_buffer[TAGMSG_ASCII_SIZE] = {0};


void create_app_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{

	// queue for handling outbound messages
	xAppMessageQueue = xQueueCreate(10, TAGMSG_ASCII_SIZE);
	// queue for handling tag messages
	xTagMessageQueue = xQueueCreate(10, TAGMSG_ASCII_SIZE);
	// queue for handling anchor messeage
	xAnchorMessageQueue = xQueueCreate(10, TAGMSG_ASCII_SIZE);

//	vSemaphoreCreateBinary(app_start_signal);

	xTaskCreate(	app_handler_task,			/* The task that implements the command console. */
					(const int8_t *const) "APP",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,			/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);

}

void app_start(void)
{
	printf("app_start\r\n");
	if(xSemaphoreTake(app_start_signal, portMAX_DELAY)) {
		start_task = true;
	}
}

void app_data_handler(uint8_t *data, uint32_t len)
{
	printf("app_data_handler: %s\r\n", data);

	if(wait_ack) {
		char * ptr = NULL;
		sys_result result = parse_result(data, "ACK", &ptr);

		if(result == SYS_OK) {
			printf("ack found!\r\n");
		}
	}
}

static sys_result parse_result(char * buffer, char * token, char ** ptr_out)
{
	sys_result result;

	char * ptr = NULL;

	if((ptr = strstr(buffer, token))) {
		if(ptr_out != NULL) {
			*ptr_out = ptr;
		}
		//printf("SYS_AT_OK\r\n");
		result = SYS_OK;
	} else {
		result = SYS_NOTFOUND;
	}

	return result;
}

static void app_handler_task(void *pvParameters)
{

#ifdef UNITTEST_WAN_ENABLE
	test_wan();
#endif
	app_handler();

}


void app_handler(void)
{
	BaseType_t result;

	uint8_t * ip_endpoint = IP_ENDPIONT;

	socket_connection_t sck_connection;

	while(true) {
		if(comm_network_state.context == 1) {
			start_task = true;
			break;
		}
		vTaskDelay(100);
	}


	while(true) {

		if(start_task) {

			start_task = false;

			printf("start task.\r\n");

			// create a new socket connection
			socket_newconnection(&sck_connection, ip_endpoint, DEFAULT_TCIP_CONNECTTIMEOUT);

			printf("sck0: %s:%d\r\n", sck_connection.socket->endpoint, sck_connection.socket->socket_conf.port);

			// establish a connection
			printf("cph_tcp_connect\r\n");
			tcp_result result = cph_tcp_connect(&sck_connection);

			if(result == SYS_TCP_OK) {
				printf("successfully connected.\r\n");

				// send a message once per second
				while(true) {

					// handle incoming messages
					app_handler_queue();
					// handle incoming tag messages
					app_handler_msgqueue();
					// handle incoming anchor messages
					app_handler_anchorqueue();

					// check the queue for messages
					result = xQueueReceive(xAppMessageQueue, socket_ascii_buffer, QUEUE_TICKS);

					// if we have a message send it
					if(result == pdTRUE) {
						printf("msg received.\r\n");
						result = cph_tcp_send(&sck_connection, socket_ascii_buffer, app_data_handler);
					}
//					result = cph_tcp_send(&sck_connection, packet, app_data_handler);

				}
			}
		}
	}

	while(true) {

		vTaskDelay(500);
	}
}

static void app_handler_queue(void)
{
	BaseType_t result;

	tag_msg_t *tag_msg;
	anchor_msg_t *anchor_msg;

	memset(tmp_buffer, 0, sizeof(tmp_buffer));

	result = xQueueReceive(xWanMessagesQueue, tmp_buffer, QUEUE_TICKS);

	if(result == pdTRUE) {

		uint8_t cmd = tmp_buffer[0];

		if(cmd == TAG) {

			tag_msg = (tag_msg_t*) tmp_buffer;
			result = xQueueSendToBack( xTagMessageQueue, tag_msg, (TickType_t)0);

			if (result == pdTRUE) {
				printf("tag_msg enqueued successfully\r\n");
			}

			#ifdef LOG_TAGMSG
				log_tag_msg(tag_msg);
			#endif
		} else if (cmd == ANCHOR_STATUS) {

			anchor_msg = (anchor_msg_t*) tmp_buffer;
			result = xQueueSendToBack( xAnchorMessageQueue, tag_msg, (TickType_t)0);

			if (result == pdTRUE) {
				printf("anchor_msg enqueued successfully\r\n");
			}

			#ifdef LOG_ANCHORMSG
				log_anchor_msg(anchor_msg);
			#endif
		}

	}
	vTaskDelay(100);

}

static void app_handler_msgqueue(void)
{
	BaseType_t result;
	tag_msg_t *tag_msg;

	memset(tmp_buffer, 0, sizeof(tmp_buffer));

	result = xQueueReceive(xTagMessageQueue, tmp_buffer, QUEUE_TICKS);

	if(result == pdTRUE) {
		tag_msg = (tag_msg_t*) tmp_buffer;

		static uint8_t packet_buffer_ascii[TAGMSG_ASCII_SIZE] = {0};

		wan_tagmsg_toascii(tag_msg, packet_buffer_ascii);

		result = xQueueSendToBack(xAppMessageQueue, tag_msg, (TickType_t)0);
	}

}

static void app_handler_anchorqueue(void)
{
	BaseType_t result;
	anchor_msg_t *anchor_msg;

	memset(tmp_buffer, 0, sizeof(tmp_buffer));

	result = xQueueReceive(xAnchorMessageQueue, tmp_buffer, QUEUE_TICKS);

	if(result == pdTRUE) {
		anchor_msg = (anchor_msg_t*) tmp_buffer;

		static uint8_t packet_buffer_ascii[TAGMSG_ASCII_SIZE] = {0};

		wan_anchormsg_toascii(anchor_msg, packet_buffer_ascii);

		result = xQueueSendToBack(xAppMessageQueue, anchor_msg, (TickType_t)0);
	}
}

void app_task_unittest(void)
{
	uint8_t * ip_endpoint = "bs.cphandheld.com";

	uint8_t * packet = "20000000,01,00000007802F6399,E16B,00000007802DE16B,1973,16,0001,0000,00C3,0000,0001,0000\r";

	socket_connection_t sck_connection;

	while(true) {
		if(comm_network_state.context == 1) {
			start_task = true;
			break;
		}
		vTaskDelay(100);
	}

	while(true) {

		if(start_task) {

			start_task = false;

			printf("start task.\r\n");

			// create a new socket connection
			socket_newconnection(&sck_connection, ip_endpoint, DEFAULT_TCIP_CONNECTTIMEOUT);

			printf("sck0: %s:%d\r\n", sck_connection.socket->endpoint, sck_connection.socket->socket_conf.port);

			// establish a connection
			printf("cph_tcp_connect\r\n");
			tcp_result result = cph_tcp_connect(&sck_connection);

			if(result == SYS_TCP_OK) {
				printf("successfully connected.\r\n");

				// send a message once per second
				while(true) {

					result = cph_tcp_send(&sck_connection, packet, app_data_handler);

					vTaskDelay(500);

				}
			}
		}
	}
}


void log_tag_msg(tag_msg_t *tag_msg)
{
	static uint8_t printf_buffer[TAGMSG_ASCII_SIZE] = {0};

	wan_tagmsg_toascii(tag_msg, printf_buffer);

	printf(printf_buffer);
	printf("\r\n");
}

void log_anchor_msg(anchor_msg_t *anchor_msg)
{
	static uint8_t printf_buffer[ANCHORMSG_ASCII_SIZE] = {0};

	wan_anchormsg_toascii(anchor_msg, printf_buffer);
	printf(printf_buffer);
	printf("\r\n");
	return;
}






