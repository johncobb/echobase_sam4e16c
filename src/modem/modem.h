/*
 * modem.h
 *
 *  Created on: Jun 16, 2015
 *      Author: jcobb
 */

#ifndef MODEM_H_
#define MODEM_H_
#include "FreeRTOS.h"
#include "semphr.h"
#include "freertos_usart_serial.h"

/* The size of the buffer provided to the USART driver for storage of received
 * bytes. */
#define MODEM_RX_BUFFER_SIZE	(128)
/* Baud rate to use. */
#define DIALER_BAUD_RATE		115200
/* The USART instance used for input and output. */
extern freertos_usart_if modem_usart;


typedef void (*modem_connect_func_t)(void);
typedef void (*modem_ondisconnect_func_t)(void);

typedef struct
{
	modem_connect_func_t on_connect;
	modem_ondisconnect_func_t on_disconnect;

}modem_event_handler_t;

typedef enum
{
	SOCKET_TCP = 0,
	SOCKET_UDP = 1
}modem_socket_type;

typedef enum
{
	SOCKET_PROT_TCP = 0,
	SOCKET_PROT_HTTP = 1,
	SOCKET_PROT_UDP = 2
}modem_socket_protocol;

typedef enum
{
	SYS_OK = 0,
	SYS_AT_OK,
	SYS_ERR_AT_TIMEOUT,
	SYS_ERR_AT_NOCARRIER,
	SYS_ERR_AT_FAIL,
	SYS_ERR_FAIL_OTHERS,
	SYS_ERR_FAIL,
	SYS_CONFIG_OK,
	SYS_NOTFOUND
}sys_result;


typedef struct {
	uint8_t type;

} dialer_cmd_t;

// buffer for maintaining
// last ten bytes received
typedef struct
{
	uint8_t index;
	uint8_t maxlen;
	uint8_t *temp;
	uint8_t *buffer;
} window_buffer_t;


extern uint32_t bytes_received;
xSemaphoreHandle config_signal;

sys_result modem_init(void);
sys_result modem_config_handler(void);
uint32_t modem_handler_async(uint32_t millis);
//sys_result handle_modem_events(modem_socket_t * socket);
sys_result handle_modem_events(uint8_t *data, int len);
sys_result handle_modem_events2(window_buffer_t *wb, uint8_t *data, int len, uint8_t *token);

void reset_rx_buffer(void);


sys_result modem_config(uint8_t config_index);
uint32_t read_modem(void);
sys_result handle_result(char * token, char ** ptr_out);
sys_result handle_result_ex(uint8_t * rx_buffer, char * token, char ** ptr_out);
uint32_t handle_stream(uint8_t *data, uint32_t len, uint32_t millis);
uint32_t modem_copy_buffer(uint8_t *data);


void SEND_AT(uint8_t *cmd);
void SEND_RAW(uint8_t *cmd);

#endif /* MODEM_H_ */
