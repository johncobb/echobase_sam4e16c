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
#include <cph.h>

xSemaphoreHandle app_start_signal = 0;


static volatile bool start_task = false;
static volatile bool wait_ack = false;

static void app_handler_task(void *pvParameters);

static void app_handler_socket(void);
static void app_handler_queue(void);

static void log_tag_msg(tag_msg_t *msg);

static void app_data_handler(uint8_t *data, uint32_t len);
static sys_result parse_result(char * buffer, char * token, char ** ptr_out);


//[0] - BaseStationId,           uint32_t
//[1] - MessageType,             uint8_t
//[2] - RouterMacAddress,        uint64_t
//[3] - RouterMacShortAddress,   uint16_t
//[4] - TagMacAddress,           uint64_t
//[5] - TagMacShortAddress,      uint32_t
//[6] - ConfigSet,               uint8_t
//[7] - Serial,                  uint8_t
//[8] - Status,                  uint16_t
//[9] - LQI ,                    uint8_t
//[10] - Rssi,                   uint8_t
//[11] - Battery,                uint16_t
//[12] - Temperature,            uint16_t

static uint8_t tmp_buffer[COBS_MSG_LEN] = {0};


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

	app_handler_queue();
//	app_handler_socket();

}


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

#ifdef LOG_TAGMSG
				log_tag_msg(msg);
#endif

#ifdef WAN_TAGMSG_FORMAT_ASCII
				wan_tagmsg_toascii(msg, packet_buffer_ascii);
#endif


			} else if (cmd == ROUTER_STATUS) {
				printf("router status message\r\n");

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



static void log_tag_msg(tag_msg_t *msg)
{

	// first copy msg into ascii buffer
	wan_tagmsg_toascii(msg, packet_buffer_ascii);

	printf("***** tag msg ascii ****\r\n");
	printf("%s\r\n", packet_buffer_ascii);

//
//				printf("msgType: %02x\r\n", msg_type);
//				printf("routerMac: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",	rtr_mac[7],
//																					rtr_mac[6],
//																					rtr_mac[5],
//																					rtr_mac[4],
//																					rtr_mac[3],
//																					rtr_mac[2],
//																					rtr_mac[1],
//																					rtr_mac[0]);
//
//				printf("routerShort: %02x:%02x\r\n",	rtr_short[1],
//														rtr_short[0]);
//
//				printf("tagMac: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n",	tag_mac[7],
//																				tag_mac[6],
//																				tag_mac[5],
//																				tag_mac[4],
//																				tag_mac[3],
//																				tag_mac[2],
//																				tag_mac[1],
//																				tag_mac[0]);
//
//				printf("tagShort: %02x:%02x\r\n",		tag_short[1],
//														tag_short[0]);
//
//				printf("tagConfig: %02x:%02x\r\n",		tag_cfg[1],
//														tag_cfg[0]);
//
//				printf("tagSerial: %02x:%02x\r\n",		tag_serial[1],
//														tag_serial[0]);
//
//				printf("tagStatus: %02x:%02x\r\n",		tag_status[1],
//														tag_status[0]);
//
//				printf("tagLqi: %02x\r\n", tag_lqi);
//				printf("tagRssi: %02x\r\n", tag_rssi);
//
//				printf("tagBattery: %02x:%02x\r\n",		tag_battery[3],
//														tag_battery[2],
//														tag_battery[1],
//														tag_battery[0]);
//
//				printf("tagTemperature: %02x:%02x\r\n",	tag_temp[3],
//														tag_temp[2],
//														tag_temp[1],
//														tag_temp[0]);

}











