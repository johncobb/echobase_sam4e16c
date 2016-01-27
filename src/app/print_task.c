/*
 * print_task.c
 *
 *  Created on: Dec 21, 2015
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
#include "print_task.h"
#include <cph.h>

QueueHandle_t xPrintQueue;

static uint32_t t_now = 0;
static uint32_t t_heartbeat_timeout = 0;

static volatile bool start_task = false;
static volatile bool wait_ack = false;

static void print_handler_task(void *pvParameters);
static void task_handler(void);

static sys_result app_handler_ondatareceive(uint8_t *data, uint32_t len);
static sys_result parse_result(char * buffer, char * token, char ** ptr_out);

static uint8_t printer_send_buffer[128] = {0};


void create_printer_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{

	// queue for handling print messages
	xPrintQueue = xQueueCreate(1, PRINTCMD_MSG_SIZE);


//	vSemaphoreCreateBinary(app_start_signal);

	xTaskCreate(	print_handler_task,			/* The task that implements the command console. */
					(const int8_t *const) "PRT",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,			/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);

}


static void print_handler_task(void *pvParameters)
{
	task_handler();
}


static void app_tcp_onconnect(void)
{
	printf("app_tcp_onconnect\r\n");
}

static void app_tcp_ondisconnect(void)
{
	printf("app_tcp_ondisconnect\r\n");
}

static void app_tcp_ondatareceive(uint8_t *data, uint32_t len)
{
//	printf("app_tcp_ondatareceive: bytes=%d data=%s\r\n", len, data);
	app_handler_ondatareceive(data, len);
}

tcp_event_handler_t tcp_event_handler = {app_tcp_onconnect, app_tcp_ondisconnect, app_tcp_ondatareceive};

#define 	HTML_START			"<!doctype html>"
#define 	HTML_END			"</html>"
//#define 	HTML_LOGCONTENT		1

// dummy html parser to demonstrate parsing callback data.
static sys_result app_handler_ondatareceive(uint8_t *data, uint32_t len)
{
	sys_result result;
#ifdef HTML_LOGCONTENT
	printf("app_handler_ondatareceive: %s\r\n", data);
#endif

	char * ptr = NULL;

	result = parse_result(data, HTML_START, &ptr);

	if(result == SYS_OK) {
		printf("<!doctype html> start tag found\r\n");
		return result;
	}

	result = parse_result(data, HTML_END, &ptr);

	if(result == SYS_OK) {
		printf("</html> end tag found\r\n");
		return result;
	}

	return result;
}


static uint8_t connection_retries = 0;

// todo: starting of hb code
// if((t_now - t_heartbeat_timeout) > 5000) {
// 		t_heartbeat_timeout = t_now;

void task_handler(void)
{
	t_now = cph_get_millis();

	BaseType_t result;

	uint8_t * ip_endpoint = IP_ENDPIONT_PRINTTASK;

	// todo: some other fun ips to hit
//	uint8_t * ip_endpoint = "appserver02.cphandheld.com";
//	uint8_t * ip_endpoint = "96.27.198.215";


	socket_connection_t sck_connection;

	while(true) {
		if(comm_network_state.context == 1) {
			start_task = true;
			break;
		}
		vTaskDelay(100);
	}

	printf("starting print task.\r\n");


	cph_tcp_init(&sck_connection, ip_endpoint, DEFAULT_TCIP_CONNECTTIMEOUT);

	printf("socket[%d]: ip_endpoint: %s port: %d\r\n", _socket_pool_index, ip_endpoint, sck_connection.socket->socket_conf.port);


	while(true) {

		if(start_task) {

//			start_task = false;

			// establish a connection
			printf("cph_tcp_connect\r\n");
			tcp_result result = cph_tcp_connect(&sck_connection);

			if(result == SYS_TCP_OK) {
				printf("successfully connected.\r\n");

				// tcp connected while loop
				//				while(tcp_isconnected) {

				while(true) {


					result = cph_tcp_send(&sck_connection, "GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: keep-alive\r\n\r\n");


					printf("cph_tcp_send result: %d\r\n", result);

					uint8_t data[1024] = {0};
					memset(data, '\0', 1024);

					while(true) {

						// bail if we lost our connection
						if(tcp_isconnected == false){
							printf("tcp_isconnected=false\r\n");
							break;
						}

						// call tcp receive so we can process data
						result = cph_tcp_receive(&sck_connection, data);


//						if(result == SYS_TCP_OK) {
//							printf("SYS_TCP_OK\r\n");
//
//						} else {
//							printf("cph_tcp_receive: error(%d)", result);
//						}

//						printf("waiting 1s.\r\n");
						vTaskDelay(500);
					}

				}

			} else {
				connection_retries++;
				printf("retrying connection...%d\r\n", connection_retries);
				vTaskDelay(1000);

			}

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


//static sys_result socket_onreceive_callback(uint8_t *data, uint32_t len)
//{
//	printf("socket_receive_cb: bytes=%d data=%s\r\n", len, data);
//	return SYS_OK;
//}
//
//static sys_result socket_handler_cb(uint8_t *data, uint32_t len)
//{
//	sys_result result;
//	printf("app_data_handler: %s\r\n", data);
//
//	if(wait_ack) {
//		char * ptr = NULL;
//		result = parse_result(data, "ACK", &ptr);
//
//		if(result == SYS_OK) {
//			printf("ack found!\r\n");
//		}
//	}
//
//	return result;
//}


