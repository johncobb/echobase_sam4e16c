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
static uint8_t tag_mac[8] = {0};
static uint8_t tag_short[2] = {0};
static uint8_t tag_cfg[2] = {0};
static uint8_t tag_serial[2] = {0};
static uint8_t tag_status[2] = {0};
static uint8_t tag_lqi = 0;
static uint8_t tag_rssi = 0;
static uint8_t tag_battery[2] = {0};
static uint8_t tag_temp[2] = {0};

// anchor variables
static uint8_t anchor_mac[8] = {0};
static uint8_t anchor_short[2] = {0};
static uint8_t anchor_reset = 0;
static uint8_t anchor_reset_task = 0;
static uint8_t anchor_serial[2] = {0};
static uint8_t anchor_config_set[2] = {0};
static uint8_t anchor_msg_count[4] = {0};
static uint8_t anchor_uptime[4] = {0};
static uint8_t anchor_battery[4] = {0};
static uint8_t anchor_temp[4] = {0};


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
	*((uint64_t*)anchor_mac) = msg->anchorMac;
	*((uint16_t*)anchor_short) = msg->anchorShort;
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
										anchor_mac[7],
										anchor_mac[6],
										anchor_mac[5],
										anchor_mac[4],
										anchor_mac[3],
										anchor_mac[2],
										anchor_mac[1],
										anchor_mac[0],
										anchor_short[1],
										anchor_short[0],
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

void wan_anchormsg_toascii(anchor_msg_t *msg, uint8_t * buffer)
{
	memset(buffer, '\0', ANCHORMSG_ASCII_SIZE);

	msg_type  = msg->messageType;
	*((uint64_t*)anchor_mac) = msg->anchorMac;
	*((uint16_t*)anchor_short) = msg->anchorShort;
	anchor_reset = msg->anchorReset;
	anchor_reset_task = msg->anchorResetTask;
	*((uint16_t*)anchor_serial) = msg->anchorSerial;
	*((uint16_t*)anchor_config_set) = msg->anchorConfigSet; // TODO: this should be a uint8_t
	*((uint32_t*)anchor_msg_count) = msg->anchorMsgCount;
	*((uint32_t*)anchor_uptime) = msg->anchorUptime;
	*((uint32_t*)anchor_battery) = msg->anchorBattery;
	*((uint32_t*)anchor_temp) = msg->anchorTemperature;

	sprintf(buffer, TCP_RTRMSG_BUFFER, 	msg_type,
										anchor_mac[7],
										anchor_mac[6],
										anchor_mac[5],
										anchor_mac[4],
										anchor_mac[3],
										anchor_mac[2],
										anchor_mac[1],
										anchor_mac[0],
										anchor_short[1],
										anchor_short[0],
										anchor_reset,
										anchor_reset_task,
										anchor_serial[1],
										anchor_serial[0],
										anchor_config_set[1],
										anchor_config_set[0],
										anchor_msg_count[3],
										anchor_msg_count[2],
										anchor_msg_count[1],
										anchor_msg_count[0],
										anchor_uptime[3],
										anchor_uptime[2],
										anchor_uptime[1],
										anchor_uptime[0],
										anchor_battery[3],
										anchor_battery[2],
										anchor_battery[1],
										anchor_battery[0],
										anchor_temp[3],
										anchor_temp[2],
										anchor_temp[1],
										anchor_temp[0]);
}

