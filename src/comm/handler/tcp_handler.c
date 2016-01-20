/*
 * tcp_handler.c
 *
 *  Created on: Jan 19, 2016
 *      Author: jcobb
 */

#include <stdio.h>
#include <string.h>

#include "comm.h"
#include "telit_modem_api.h"
#include "modem_defs.h"
#include "modem.h"
#include "tcp_handler.h"

sys_result tcp_validate_connection(char * data);

uint16_t tcp_buffer_index = 0;
uint8_t tcp_buffer[TCP_BUFFER_LEN+1] = {0};

BaseType_t result;

sys_result tcp_handle_data(uint8_t *data, uint32_t len)
{
	sys_result result = tcp_validate_connection(data);

	comm_frame_t frame;

	memset(frame.buffer, '\0', FRAME_BUFFER_LEN+1);

	frame.type = FRAME_TCP;
	frame.len = len;

	memcpy(frame.buffer, data, len);

	result = xQueueSendToBack( xCommQueue, &frame, (TickType_t)0);

	if(result == pdTRUE) {
		//printf("message queued.\r\n");
	} else {
		printf("failed to enqueue message\r\n");
	}

	return result;

}


sys_result tcp_validate_connection(char * data)
{
	char * ptr = NULL;

	sys_result result = SYS_OK;

	if((ptr = strstr(data, MODEM_TOKEN_NOCARRIER))) {
		printf("SYS_ERR_AT_NOCARRIER\r\n");
		result = SYS_ERR_AT_NOCARRIER;
	}

	return result;
}
