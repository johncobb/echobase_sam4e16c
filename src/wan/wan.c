/*
 * wan.c
 *
 *  Created on: Aug 19, 2015
 *      Author: jcobb
 */
#include <stdint.h>
#include <string.h>
#include "sysclk.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "freertos_usart_serial.h"
#include "board.h"
#include "wan.h"


#define RX_BUFFER_SIZE_BYTES	128
#define RX_BUFFER_LEN			128

uint8_t packet_buffer_ascii[COBS_BUFFER_LEN] = {0};
// tag variables
static uint8_t msg_type = 0;
static uint8_t rtr_mac[8] = {0};
static uint8_t rtr_short[2] = {0};
static uint8_t tag_mac[8] = {0};
static uint8_t tag_short[2] = {0};
static uint8_t tag_cfg[2] = {0};
static uint8_t tag_serial[2] = {0};
static uint8_t tag_status[2] = {0};
static uint8_t tag_lqi = 0;
static uint8_t tag_rssi = 0;
static uint8_t tag_battery[2] = {0};
static uint8_t tag_temp[2] = {0};

// router variables
static uint8_t rtr_reset = 0;
static uint8_t rtr_reset_task = 0;
static uint8_t rtr_serial[2] = {0};
static uint8_t rtr_config_set = 0;
static uint8_t rtr_msg_count[4] = {0};
static uint8_t rtr_uptime[4] = {0};
static uint8_t rtr_battery[4] = {0};
static uint8_t rtr_temp[4] = {0};





freertos_usart_if wan_usart;

uint8_t wan_rx_buffer_index = 0;
uint8_t wan_rx_buffer[WAN_RX_BUFFER_SIZE+1] = {0};


static uint8_t receive_buffer[RX_BUFFER_SIZE_BYTES] = {0};

static void init_usart(Usart *usart_base);


uint8_t wan_init(void)
{
//	board_init_wan_usart();

	init_usart(WAN_USART);

	return 0;
}

static void init_usart(Usart *usart_base)
{

	freertos_peripheral_options_t driver_options = {
		receive_buffer,									/* The buffer used internally by the USART driver to store incoming characters. */
		RX_BUFFER_SIZE_BYTES,							/* The size of the buffer provided to the USART driver to store incoming characters. */
		configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,	/* The priority used by the USART interrupts. */
		USART_RS232,									/* Configure the USART for RS232 operation. */
		//UART_RS232,
//		(WAIT_TX_COMPLETE | USE_TX_ACCESS_MUTEX)		/* Use access mutex on Tx (as more than one task transmits) but not Rx. Wait for a Tx to complete before returning from send functions. */
		(USE_RX_ACCESS_MUTEX | USE_TX_ACCESS_MUTEX)
	};

	const sam_usart_opt_t usart_settings = {
		WAN_BAUD_RATE,
		US_MR_CHRL_8_BIT,
		US_MR_PAR_NO,
		US_MR_NBSTOP_1_BIT,
		US_MR_CHMODE_NORMAL,
		0 /* Only used in IrDA mode. */
	};

	/* Initialise the USART interface. */
	wan_usart = freertos_usart_serial_init(usart_base, &usart_settings, &driver_options);
	configASSERT(wan_usart);
}

uint8_t wan_config(void)
{
	return 0;
}


void WAN_SEND(uint8_t *cmd)
{
	uint8_t *output_string;

	portTickType max_block_time_ticks = 200UL / portTICK_RATE_MS;

	static char * tmp_buffer[400];

	output_string = (uint8_t *) tmp_buffer;

	strcpy((char *) output_string, (char *) cmd);

	freertos_usart_write_packet(wan_usart, output_string,
								strlen((char *) cmd),
								max_block_time_ticks);
}

uint32_t wan_handler_async(uint32_t millis)
{
	portTickType max_wait_millis = millis / portTICK_RATE_MS;

	uint32_t len = freertos_usart_serial_read_packet(wan_usart, wan_rx_buffer, WAN_RX_BUFFER_SIZE, max_wait_millis);

	//printf("modem_handler_async: %lu\r\n", len);
	return len;
}



void wan_tagmsg_toascii(tag_msg_t *msg, uint8_t * buffer)
{
	memset(buffer, '\0', TAGMSG_ASCII_SIZE);


	msg_type  = msg->messageType;
	*((uint64_t*)rtr_mac) = msg->routerMac;
	*((uint16_t*)rtr_short) = msg->routerShort;
	*((uint64_t*)tag_mac) = msg->tagMac;
	// since we don't receive the macShort we need to build it
	tag_short[0] = tag_mac[0];
	tag_short[1] = tag_mac[1];
	tag_short[2] = tag_mac[2];
	tag_short[3] = tag_mac[3];
	*((uint16_t*)tag_cfg) = msg->tagConfigSet;
	*((uint16_t*)tag_serial) = msg->tagSerial;
	*((uint16_t*)tag_status) = msg->tagStatus;
	tag_lqi = msg->tagLqi;
	tag_rssi = msg->tagRssi;
	*((uint16_t*)tag_battery) = msg->tagBattery;
	*((uint16_t*)tag_temp) = msg->tagTemperature;


	sprintf(buffer, TCP_APPMSG_BUFFER, 	msg_type,
										rtr_mac[7],
										rtr_mac[6],
										rtr_mac[5],
										rtr_mac[4],
										rtr_mac[3],
										rtr_mac[2],
										rtr_mac[1],
										rtr_mac[0],
										rtr_short[1],
										rtr_short[0],
										tag_mac[7],
										tag_mac[6],
										tag_mac[5],
										tag_mac[4],
										tag_mac[3],
										tag_mac[2],
										tag_mac[1],
										tag_mac[0],
										tag_short[1],
										tag_short[0],
										tag_cfg[1],
										tag_cfg[0],
										tag_serial[1],
										tag_serial[0],
										tag_status[1],
										tag_status[0],
										tag_lqi,
										tag_rssi,
										tag_battery[1],
										tag_battery[0],
										tag_temp[1],
										tag_temp[0]);



}

void wan_routermsg_toascii(router_msg_t *msg, uint8_t * buffer)
{
	memset(buffer, '\0', RTRMSG_ASCII_SIZE);

	msg_type  = msg->messageType;
	*((uint64_t*)rtr_mac) = msg->routerMac;
	*((uint16_t*)rtr_short) = msg->routerShort;
	rtr_reset = msg->routerReset;
	rtr_reset_task = msg->resetTask;
	*((uint16_t*)rtr_serial) = msg->routerSerial;
	rtr_config_set = msg->routerConfigSet;
	*((uint32_t*)rtr_msg_count) = msg->routerMsgCount;
	*((uint32_t*)rtr_uptime) = msg->routerUptime;
	*((uint32_t*)rtr_battery) = msg->routerBattery;
	*((uint32_t*)rtr_temp) = msg->routerTemperature;

	sprintf(buffer, TCP_RTRMSG_BUFFER, 	msg_type,
										rtr_mac[7],
										rtr_mac[6],
										rtr_mac[5],
										rtr_mac[4],
										rtr_mac[3],
										rtr_mac[2],
										rtr_mac[1],
										rtr_mac[0],
										rtr_short[1],
										rtr_short[0],
										rtr_reset,
										rtr_reset_task,
										rtr_serial[1],
										rtr_serial[0],
										rtr_config_set,
										rtr_msg_count[3],
										rtr_msg_count[2],
										rtr_msg_count[1],
										rtr_msg_count[0],
										rtr_uptime[3],
										rtr_uptime[2],
										rtr_uptime[1],
										rtr_uptime[0],
										rtr_battery[3],
										rtr_battery[2],
										rtr_battery[1],
										rtr_battery[0],
										rtr_temp[3],
										rtr_temp[2],
										rtr_temp[1],
										rtr_temp[0]);
}

