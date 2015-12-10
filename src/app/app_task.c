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
#include "wan_msg.h"
#include "wan_task.h"
#include "app_task.h"

xSemaphoreHandle app_start_signal = 0;


static volatile bool start_task = false;
static volatile bool wait_ack = false;

static void app_handler_task(void *pvParameters);

static void app_handler_socket(void);
static void app_handler_queue(void);

void create_app_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{

//	vSemaphoreCreateBinary(app_start_signal);

	xTaskCreate(	app_handler_task,			/* The task that implements the command console. */
					(const int8_t *const) "APP",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,			/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);

}

static void app_data_handler(uint8_t *data, uint32_t len);
static sys_result parse_result(char * buffer, char * token, char ** ptr_out);

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
//	app_handler_socket();
	app_handler_queue();

}

static uint8_t tmp_buffer[COBS_MSG_LEN] = {0};
uint8_t packet_buffer[COBS_BUFFER_LEN] = {0};

uint8_t message_type = 0;
uint8_t rtr_mac[8] = {0};
uint8_t rtr_short[4] = {0};
uint8_t tag_mac[8] = {0};
uint8_t tag_cfg[2] = {0};
uint8_t tag_serial[2] = {0};
uint8_t tag_status[2] = {0};
uint8_t tag_lqi = 0;
uint8_t tag_rssi = 0;
uint8_t tag_battery[4] = {0};
uint8_t tag_temp[4] = {0};

static void app_handler_queue(void)
{
	tag_msg_t *msg;


	BaseType_t result;

	memset(tmp_buffer, 0, sizeof(tmp_buffer));



	while(true) {

		result = xQueueReceive(xWanMessagesQueue, tmp_buffer, QUEUE_TICKS);

		if(result == pdTRUE) {


			uint8_t cmd = tmp_buffer[0];

			if(cmd == TAG) {
				msg = (tag_msg_t*) tmp_buffer;
				memset(packet_buffer, '\0', sizeof(packet_buffer));

//				memcpy(rtr_mac, (uint8_t*)(msg->routerMac), 8);
				*((uint64_t*)rtr_mac) = msg->routerMac;


//				sprintf(packet_buffer, "routerMac: %01611x\r\n", msg->routerMac );



				printf("routerMac: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",	rtr_mac[7],
																					rtr_mac[6],
																					rtr_mac[5],
																					rtr_mac[4],
																					rtr_mac[3],
																					rtr_mac[2],
																					rtr_mac[1],
																					rtr_mac[0]);

				uint8_t * packet = "20000000,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\r";


//				printf("routerMac: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",	decoded_buffer[8],
//																					decoded_buffer[7],
//																					decoded_buffer[6],
//																					decoded_buffer[5],
//																					decoded_buffer[4],
//																					decoded_buffer[3],
//																					decoded_buffer[2],
//																					decoded_buffer[1]);


			}



			// handle the message
		}
		vTaskDelay(100);
	}
}


void app_handler_socket(void)
{
	uint8_t * ip_endpoint = "bs.cphandheld.com";
//	uint8_t * ip_endpoint = "96.27.198.215";
//	uint8_t * ip_endpoint2 = "96.27.198.215";

//	uint8_t * packet = "20000000,01,01,00000007802F6399,E16B,00000007802DE16B,1973,16,0001,0000,00C3,0000,0001,EE000000,470000EE\r";
	uint8_t * packet = "20000000,01,00000007802F6399,E16B,00000007802DE16B,1973,16,0001,0000,00C3,0000,0001,0000\r";

	socket_connection_t sck_connection;
//	socket_connection_t sck_connection2;

//	xSemaphoreGive(app_start_signal);

	while(true) {
		if(comm_network_state.context == 1) {
			start_task = true;
			break;
		}
		vTaskDelay(100);
	}

	BaseType_t result;

	while(true) {

		if(start_task) {

			start_task = false;

			printf("start task.\r\n");

			// create a new socket connection
			socket_newconnection(&sck_connection, ip_endpoint, DEFAULT_TCIP_CONNECTTIMEOUT);
//			socket_newconnection(&sck_connection2, ip_endpoint2, DEFAULT_TCIP_CONNECTTIMEOUT);

			printf("sck0: %s:%d\r\n", sck_connection.socket->endpoint, sck_connection.socket->socket_conf.port);
//			printf("sck1: %s:%d\r\n", sck_connection2.socket->endpoint, sck_connection2.socket->socket_conf.port);

			// establish a connection
			printf("cph_tcp_connect\r\n");
			tcp_result result = cph_tcp_connect(&sck_connection);
//			printf("cph_tcp_connect\r\n");
//			tcp_result result2 = cph_tcp_connect(&sck_connection2);


			if(result == SYS_TCP_OK) {
				printf("successfully connected.\r\n");

				// send a message once per second
				while(true) {
					result = cph_tcp_send(&sck_connection, packet, app_data_handler);

				}
			}
		}
	}


	while(true) {

		vTaskDelay(500);
	}
}