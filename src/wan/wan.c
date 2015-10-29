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



freertos_usart_if wan_usart;

uint8_t wan_rx_buffer_index = 0;
uint8_t wan_rx_buffer[WAN_RX_BUFFER_SIZE+1] = {0};


static uint8_t receive_buffer[RX_BUFFER_SIZE_BYTES] = {0};

static void init_usart(Usart *usart_base);


uint8_t wan_init(void)
{
//	board_init_wan_usart();
	sysclk_enable_peripheral_clock(WAN_USART_ID);
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

