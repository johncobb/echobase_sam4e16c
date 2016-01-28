/*
 * http_task.c
 *
 *  Created on: Jan 27, 2016
 *      Author: jcobb
 */
#include <stdint.h>
#include <string.h>


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "sysclk.h"
#include "comm.h"
#include "tcpip.h"
#include "http_task.h"
#include <cph.h>



//QueueHandle_t xHttpQueue;

static uint32_t t_now = 0;
static volatile bool start_task = false;

static void http_handler_task(void *pvParameters);
static void task_handler(void);

static sys_result app_handler_ondatareceive(uint8_t *data, uint32_t len);
static sys_result parse_result(char * buffer, char * token, char ** ptr_out);

static bool is_connected = false;
static uint16_t connection_retries = 0;

void create_http_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{
	// queue for handling http messages
//	xPrintQueue = xHttpQueue(1, HTTP_MSG_SIZE);

	xTaskCreate(	http_handler_task,			/* The task that implements the command console. */
					(const int8_t *const) "HTTP",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,			/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);

}

static void http_handler_task(void *pvParameters)
{
	task_handler();
}

static void app_tcp_onconnect(void)
{
	is_connected = true;
	printf("app_tcp_onconnect\r\n");
}

static void app_tcp_ondisconnect(void)
{
	is_connected = false;
	printf("app_tcp_ondisconnect\r\n");
}

static void app_tcp_ondatareceive(uint8_t *data, uint32_t len)
{
	app_handler_ondatareceive(data, len);
}


static sys_result app_handler_ondatareceive(uint8_t *data, uint32_t len)
{
	sys_result result;
#ifdef HTTP_HTML_LOGCONTENT
	printf("app_handler_ondatareceive: %s\r\n", data);
#endif

	char * ptr = NULL;

	result = parse_result(data, HTTP_HTML_START, &ptr);

	if(result == SYS_OK) {
		printf("%s start tag found\r\n", HTTP_HTML_START);
		return result;
	}

	result = parse_result(data, HTTP_HTML_END, &ptr);

	if(result == SYS_OK) {
		printf("%s end tag found\r\n", HTTP_HTML_END);
		return result;
	}

	return result;
}



void task_handler(void)
{
	t_now = cph_get_millis();

	BaseType_t result;

	uint8_t * ip_endpoint = IP_ENDPIONT_HTTPTASK;

	socket_connection_t sck_connection;

	while(true) {
		if(comm_network_state.context == 1) {
			start_task = true;
			break;
		}
		vTaskDelay(100);
	}

	printf("starting http task.\r\n");

	// create and register the tcp event callback system
	socket_event_handler_t tcp_event_handler = {app_tcp_onconnect, app_tcp_ondisconnect, app_tcp_ondatareceive};

	cph_tcp_init2(&sck_connection, ip_endpoint, DEFAULT_TCIP_CONNECTTIMEOUT, &tcp_event_handler);

	printf("socket[%d]: ip_endpoint: %s port: %d\r\n", _socket_pool_index, ip_endpoint, sck_connection.socket->socket_conf.port);


	while(true) {

		if(start_task) {

			// establish a connection
			printf("cph_tcp_connect\r\n");
			tcp_result result = cph_tcp_connect(&sck_connection);

			if(result == SYS_TCP_OK) {
				printf("successfully connected.\r\n");


				while(true) {

					result = cph_tcp_send(&sck_connection, "GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: keep-alive\r\n\r\n");

					printf("cph_tcp_send result: %d\r\n", result);

					uint8_t data[1024] = {0};
					memset(data, '\0', 1024);

					while(true) {

						// bail if we lost our connection
						if(is_connected == false){
							printf("tcp_isconnected=false\r\n");
							break;
						}

						// call tcp receive so we can process data
						result = cph_tcp_receive(&sck_connection, data);

						vTaskDelay(50);
					}

					// delay 5 seconds to let the network settle before attempting
					// another connection.
					printf("waiting 5s.\r\n");
					vTaskDelay(5000);
					// now break back into main loop to reattempt connection
					break;
				}

			} else {
				printf("waiting 5s.\r\n");
				vTaskDelay(5000);
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
