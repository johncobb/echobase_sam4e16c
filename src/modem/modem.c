/*
 * modem.c
 *
 *  Created on: Jun 16, 2015
 *      Author: jcobb
 */

#include <stdint.h>
#include <string.h>
#include <cph.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "modem.h"
#include "modem_defs.h"
#include "telit_modem_api.h"
#include "socket.h"

#define MODEM_RX_BUFFER_LEN						128
#define MODEM_NOCARRIER_BUFFER_LEN				10

// copy the bytes into the local comm_rx_buffer. we will accumulate until
// we see the token we're after.
// memcpy(&comm_rx_buffer[comm_rx_buffer_index], socket->rx_buffer, socket->bytes_received);


freertos_usart_if modem_usart;

uint8_t modem_rx_buffer_index = 0;
static uint8_t modem_rx_buffer[MODEM_RX_BUFFER_SIZE+1] = {0};

modem_status_t modem_status;

xSemaphoreHandle config_signal = 0;

void pause(void);
void reset_modem_event_buffers(void);



uint32_t timeout = 0;

char * ptr = NULL;

uint32_t bytes_received;
uint8_t rx_buffer[MODEM_RX_BUFFER_LEN+1];

uint8_t modem_nocarrier_buffer[MODEM_NOCARRIER_BUFFER_LEN] = {0};
uint8_t nocarrier_window_index = 0;


void reset_rx_buffer(void)
{
	bytes_received = 0;
	memset(modem_rx_buffer, '\0', sizeof(modem_rx_buffer));
}

void reset_modem_event_buffers(void)
{
	memset(modem_nocarrier_buffer, '\0', sizeof(modem_nocarrier_buffer));
	nocarrier_window_index = 0;
}

void pause(void)
{
	printf("pausing 10 sec.\r\n");
	vTaskDelay(10000);
	printf("resuming...\r\n");
}




/* The buffer provided to the USART driver to store incoming character in. */
static uint8_t receive_buffer[MODEM_RX_BUFFER_SIZE] = {0};

static void init_hw(void);
static void init_usart(Usart *usart_base);

void init_hw(void)
{

	//printf("initializing modem hardware...\r\n");
	//SEND_AT("init_hw\r\n");
	// Sanity check make sure pins are low
	pio_set_pin_low(MDM_ENABLE_IDX);
	pio_set_pin_low(MDM_RESET_IDX);
	pio_set_pin_low(MDM_ONOFF_IDX);

	printf("checking modem power monitor\r\n");

	// Chek for power on

	uint8_t num_tries = 0;
	uint32_t powmon = 0;

	while(true) {

		if(num_tries++ == 3) break;

		vTaskDelay(3000);

		powmon = pio_get_pin_value(MDM_POWMON_IDX);
		vTaskDelay(10);

		printf("modem_powmon=%d\r\n", powmon);

		if(powmon == 0) {
			printf("modem_powmon: LOW\r\n");
			break;
		} else {
			//printf("toggle modem enable high-low\r\n");
			pio_set_pin_low(MDM_ENABLE_IDX);
			vTaskDelay(20);
			pio_set_pin_high(MDM_ENABLE_IDX);
		}
	}

	while(true) {
		printf("toggle modem_onoff: HIGH\r\n");
		printf("wait 3 sec...\r\n");
		pio_set_pin_high(MDM_ONOFF_IDX);

		vTaskDelay(3000);

		printf("toggle modem_onoff: LOW\r\n");
		printf("wait 3 sec...\r\n");
		pio_set_pin_low(MDM_ONOFF_IDX);

		vTaskDelay(2000);

		powmon = pio_get_pin_value(MDM_POWMON_IDX);

		if(powmon == 1) {
			printf("modem_powmon: HIGH\r\n");
			printf("modem powered on!\r\n");
			break;
		}

	}

	printf("init_hw done\r\n");
}

// deprecated simplified init. does not gurantee reset
//void init_hw2(void)
//{
//	//printf("initializing modem hardware...\r\n");
//	// Sanity check make sure pins are low
//	pio_set_pin_low(MDM_ENABLE_IDX);
//	pio_set_pin_low(MDM_RESET_IDX);
//	pio_set_pin_low(MDM_ONOFF_IDX);
//
//	while(true) {
//		pio_set_pin_high(MDM_ONOFF_IDX);
//		vTaskDelay(3000);
//
//		pio_set_pin_low(MDM_ONOFF_IDX);
//		vTaskDelay(2000);
//		if(pio_get_pin_value(MDM_POWMON_IDX) == 1)
//			break;
//	}
//
//	printf("init_hw done\r\n");
//}

sys_result modem_init(void)
{
	// hardware must be initialized before usart
	init_hw();

	// configure usart pins (must be done after init_hw call)
	board_init_modem_usart();

	// init usart
	init_usart(MODEM_USART);

	// reset buffers used to detect carrier signals from modem
	reset_modem_event_buffers();

	return SYS_OK;
}

sys_result modem_config_handler(void)
{
	char * ptr = NULL;

	sys_result sys_status;

	sys_status = handle_result(MODEM_TOKEN_OK, &ptr);

	if(sys_status == SYS_AT_OK) {
		//at_cmd++;
		printf("SYS_OK\r\n");
		printf("buffer:\r\n%s\r\n", ptr);
	} else if (sys_status == SYS_ERR_AT_FAIL) {
		printf("SYS_ERR_AT_FAIL\r\n");
	} else if (sys_status == SYS_ERR_AT_NOCARRIER) {
		printf("SYS_ERR_AT_NO_CARRIER\r\n");
	} else if(sys_status == SYS_ERR_AT_TIMEOUT) {
		printf("SYS_ERR_AT_TIMEOUT\r\n");
		printf("buffer:\r\n%s\r\n", ptr);
		//return SYS_ERR_AT_FAIL;
	}

	return sys_status;
}

static void init_usart(Usart *usart_base)
{
	freertos_peripheral_options_t driver_options = {
		receive_buffer,									/* The buffer used internally by the USART driver to store incoming characters. */
		MODEM_RX_BUFFER_SIZE,							/* The size of the buffer provided to the USART driver to store incoming characters. */
		configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,	/* The priority used by the USART interrupts. */
		USART_RS232,									/* Configure the USART for RS232 operation. */
		//UART_RS232,
		//(WAIT_TX_COMPLETE | USE_TX_ACCESS_MUTEX)		/* Use access mutex on Tx (as more than one task transmits) but not Rx. Wait for a Tx to complete before returning from send functions. */
		(USE_RX_ACCESS_MUTEX | USE_TX_ACCESS_MUTEX)
	};

	const sam_usart_opt_t usart_settings = {
		DIALER_BAUD_RATE,
		US_MR_CHRL_8_BIT,
		US_MR_PAR_NO,
		US_MR_NBSTOP_1_BIT,
		US_MR_CHMODE_NORMAL,
		0 /* Only used in IrDA mode. */
	};

	/* Initialise the USART interface. */
	modem_usart = freertos_usart_serial_init(usart_base,
			&usart_settings,
			&driver_options);
	configASSERT(modem_usart);
}

void SEND_AT(uint8_t *cmd)
{
	uint8_t *output_string;

	portTickType max_block_time_ticks = 200UL / portTICK_RATE_MS;

	static char * tmp_buffer[400];

	output_string = (uint8_t *) tmp_buffer;

	strcpy((char *) output_string, (char *) cmd);

	freertos_usart_write_packet(modem_usart, output_string,
								strlen((char *) cmd),
								max_block_time_ticks);
}

void SEND_RAW(uint8_t *cmd)
{
	uint8_t *output_string;

	portTickType max_block_time_ticks = 200UL / portTICK_RATE_MS;

	static char * tmp_buffer[400];

	output_string = (uint8_t *) tmp_buffer;

	strcpy((char *) output_string, (char *) cmd);

	freertos_usart_write_packet(modem_usart, output_string,
								strlen((char *) cmd),
								max_block_time_ticks);
}

uint32_t modem_handler_async(uint32_t millis)
{
	portTickType max_wait_millis = millis / portTICK_RATE_MS;

	uint32_t len = freertos_usart_serial_read_packet(modem_usart, modem_rx_buffer, MODEM_RX_BUFFER_SIZE, max_wait_millis);

	//printf("modem_handler_async: %lu\r\n", len);
	return len;
}


#define TEMP_BUFFER_LEN			9
#define NOCARRIER_WINDOW_MAX	(MODEM_NOCARRIER_BUFFER_LEN-1) // once less than the total buffer

sys_result handle_modem_events(uint8_t *data, int len)
{
	sys_result result = SYS_OK;

	for (int i=0; i<len; i++) {

		// get a reference to the current byte
		uint8_t c = data[i];

		// guard against overwriting the last byte in our buffer once
		// we've hit the NOCARRIER_WINDOW_MAX since we want to copy
		// the last 9 bytes received into slots 0 thru 8 after which
		// we will copy the current byte received into the last slot 9
		if(nocarrier_window_index < NOCARRIER_WINDOW_MAX)
			modem_nocarrier_buffer[nocarrier_window_index] = c;

		// if we reached the end check to see
		// if we have the token we're looking for
		// either way reset the index for the next round of bytes
		if(nocarrier_window_index == NOCARRIER_WINDOW_MAX) {

			// prep the tmp_buffer
			uint8_t tmp_buffer[TEMP_BUFFER_LEN] = {0};
			memset(tmp_buffer, 0, sizeof(tmp_buffer));

			// __________
			// \nno carri
			// nno carrie
			// no carrier

			// copy bytes 1 thru 9 into our tmp_buffer
			// we don't care about byte 0 because its about
			// to be bumped out of the window we care about
			memcpy(tmp_buffer, &modem_nocarrier_buffer[1], sizeof(tmp_buffer));

			// reset our primary buffer
			memset(modem_nocarrier_buffer, 0, sizeof(modem_nocarrier_buffer));

			// copy the 9 bytes from tmp_buffer into slots 0 thru 8
			memcpy(modem_nocarrier_buffer, tmp_buffer, sizeof(tmp_buffer));

			// copy the last byte received into the last byte of the buffer
			modem_nocarrier_buffer[nocarrier_window_index] = c;

			char * ptr = NULL;

			if((ptr = strstr(modem_nocarrier_buffer, MODEM_TOKEN_NOCARRIER))) {
				printf("holy shit it worked\r\n");
				reset_modem_event_buffers();
				result = SYS_ERR_AT_NOCARRIER;
				break;
			}
		}
		// move index to next slot
		if(nocarrier_window_index < NOCARRIER_WINDOW_MAX)
			nocarrier_window_index++;
	}

	return result;
}

sys_result handle_result(char * token, char ** ptr_out)
{

	// SPECIAL CASE FOR HANDLING ASYNCHRONOUS DATA
//	if(token == NULL) {
//		*ptr_out = modem_rx_buffer;
//		return SYS_OK;
//	}

	//if((ptr = strstr(input_string, token))) {
	if((ptr = strstr(modem_rx_buffer, token))) {
		if(ptr_out != NULL) {
			*ptr_out = ptr;
		}
		//printf("SYS_AT_OK\r\n");
		return SYS_AT_OK;
	} else if ((ptr = strstr(modem_rx_buffer, MODEM_TOKEN_ERROR))) {
		if(ptr_out != NULL) {
			*ptr_out = ptr;
		}
		printf("SYS_ERR_AT_FAIL\r\n");
		return SYS_ERR_AT_FAIL;
	} else if((ptr = strstr(modem_rx_buffer, MODEM_TOKEN_NOCARRIER))) {
		if(ptr_out != NULL) {
			*ptr_out = ptr;
		}
		printf("SYS_ERR_AT_NOCARRIER\r\n");
		return SYS_ERR_AT_NOCARRIER;
	}

	// set ptr_out to the rx_buffer for troubleshooting
	if(ptr_out != NULL) {
		*ptr_out = modem_rx_buffer;
	}

}

sys_result handle_result_ex(uint8_t * rx_buffer, char * token, char ** ptr_out)
{
	//if((ptr = strstr(input_string, token))) {
	if((ptr = strstr(rx_buffer, token))) {
		if(ptr_out != NULL) {
			*ptr_out = ptr;
		}
		//printf("SYS_AT_OK\r\n");
		return SYS_AT_OK;
	} else if ((ptr = strstr(rx_buffer, MODEM_TOKEN_ERROR))) {
		if(ptr_out != NULL) {
			*ptr_out = ptr;
		}
		printf("SYS_ERR_AT_FAIL\r\n");
		return SYS_ERR_AT_FAIL;
	} else if((ptr = strstr(rx_buffer, MODEM_TOKEN_NOCARRIER))) {
		if(ptr_out != NULL) {
			*ptr_out = ptr;
		}
		printf("SYS_ERR_AT_NOCARRIER\r\n");
		return SYS_ERR_AT_NOCARRIER;
	}

	// set ptr_out to the rx_buffer for troubleshooting
	if(ptr_out != NULL) {
		*ptr_out = rx_buffer;
	}
}

uint32_t handle_stream(uint8_t *data, uint32_t len, uint32_t millis)
{
	portTickType max_wait_millis = millis / portTICK_RATE_MS;

	bytes_received = freertos_usart_serial_read_packet(modem_usart, data, len, max_wait_millis);

	return bytes_received;
}

uint32_t read_modem(void)
{
	portTickType max_wait_millis = 20 / portTICK_RATE_MS;

	memset(rx_buffer, '\0', MODEM_RX_BUFFER_LEN+1);
	return freertos_usart_serial_read_packet(modem_usart, rx_buffer, MODEM_RX_BUFFER_LEN, max_wait_millis);
}

uint32_t modem_copy_buffer(uint8_t *data)
{
	memcpy(data, modem_rx_buffer, bytes_received);
	return bytes_received;
}




//void init_modem_buffer(void);
//void init_modem_lines(void);
////sys_result process_modem(char * token, char ** ptr_out, uint8_t seconds);
//sys_result read_modem_usart(uint8_t seconds);
//
//
//uint8_t modem_lines[MAX_INPUT_SIZE+1];
//uint8_t modem_line_index = 0;
//
//uint8_t modem_buffer[MAX_INPUT_SIZE+1];
//uint8_t modem_buffer_index = 0;
//
//sys_result handle_result(char * token, char ** ptr_out, uint8_t seconds)
//{
//	init_modem_lines();
//	if(read_modem_usart(seconds) == SYS_AT_OK) {
//
//		char * ptr = NULL;
//
//		if((ptr = strstr(modem_lines, token))) {
//			if(ptr_out != NULL) {
//				*ptr_out = ptr;
//				//printf("sys_ok\r\n");
//				return SYS_AT_OK;
//			}
//		} else if ((ptr = strstr(modem_lines, MODEM_TOKEN_ERROR))) {
//			*ptr_out = ptr;
//			return SYS_ERR_AT_FAIL;
//		} else if((ptr = strstr(modem_lines, MODEM_TOKEN_NOCARRIER))) {
//			*ptr_out = ptr;
//			return SYS_ERR_AT_NOCARRIER;
//		}
//	}
//
//	return SYS_ERR_AT_TIMEOUT;
//}
//
//sys_result read_modem_usart(uint8_t seconds)
//{
//	uint8_t c;
//	uint8_t buffer_index = 0;
//
//	// convert to seconds
//	uint32_t timeout = ((uint32_t)sys_now()) + (seconds*1000);
//	//portTickType max_block_time_ticks = 200UL / portTICK_RATE_MS;
//
//	while(sys_now() <= timeout)
//	{
//		if (freertos_usart_serial_read_packet(modem_usart, &c, sizeof(c), portMAX_DELAY) == sizeof(c)) {
//
//			if(c == '\0')
//				continue;
//
//			modem_buffer[buffer_index] = c;
//			buffer_index++;
//
//			if(buffer_index >= MAX_INPUT_SIZE)
//				continue;
//
//			if (c == '\r' || c == '\n') {
//				strcpy(modem_buffer, modem_lines);
//				init_modem_buffer();
//				return SYS_AT_OK;
//			}
//		}
//	}
//
//	return SYS_ERR_AT_TIMEOUT;
//}
//
//void init_modem_buffer(void)
//{
//	modem_line_index = 0;
//	memset(modem_buffer, '\0', sizeof(modem_buffer));
//}
//
//void init_modem_lines(void)
//{
//	memset(modem_lines, '\0', sizeof(modem_lines));
//}

