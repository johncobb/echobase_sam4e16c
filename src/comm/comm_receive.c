/*
 * comm_receive.c
 *
 *  Created on: Dec 23, 2015
 *      Author: jcobb
 */


#include <string.h>
#include "socket.h"
#include "modem_defs.h"
#include "modem.h"
#include "telit_modem_api.h"
#include "comm.h"
#include "comm_receive.h"
#include "http_handler.h"

typedef enum
{
	COMM_RECEIVE_RX = 0,
	COMM_RECEIVE_RX_ABORT
}comm_receive_state_t;

typedef enum
{
	COMM_RECEIVE_INVOKE = 0,
	COMM_RECEIVE_WAITREPLY = 1
}comm_receive_sub_state_t;



sys_result  comm_receive(modem_socket_t * socket)
{
	sys_result result;

	if(socket->state_handle.state == COMM_RECEIVE_RX) {

		if(socket->state_handle.substate == COMM_RECEIVE_INVOKE) {
			printf("socket(%d) receive...\r\n", socket->socket_id);

			// prepare the socket receive buffer
			memset(socket->rx_buffer, '\0', SOCKET_BUFFER_LEN+1);
			socket->bytes_received = 0;

			socket_entersubstate(socket, COMM_RECEIVE_WAITREPLY);

			socket_settimeout(socket, DEFAULT_COMM_SOCKETRECEIVE_TIMEOUT);

			result = SYS_OK;
		} else if(socket->state_handle.substate == COMM_RECEIVE_WAITREPLY) {

			// wait up to n seconds.
			if(socket_timeout(socket)) {
				socket->socket_error = SCK_ERR_TIMEOUT;

				printf("socket(%d) receive timeout\r\n", socket->socket_id);

				// TODO: review for proper transition after receive
				comm_enterstate(socket, COMM_IDLE);
				socket_exitstate(socket);
				xSemaphoreGive(tcp_receive_signal);
				result = SYS_OK;

			}

			// if we receive byts we need to copy the data and signal the
			// function to return
			if(socket->bytes_received > 0) {
//				printf("bytes_received: %lu, %s\r\n", socket->bytes_received, socket->rx_buffer);
//				socket->handle_data(socket->rx_buffer, socket->bytes_received);
				socket->event_handler->on_datareceive(socket->rx_buffer, socket->bytes_received);
				memset(socket->rx_buffer, '\0', SOCKET_BUFFER_LEN+1);
				xSemaphoreGive(tcp_receive_signal);
			}
			result = SYS_OK;
		}
	}

	return result;
}
