/*
 * tcpip.c
 *
 *  Created on: Jul 21, 2015
 *      Author: jcobb
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "socket.h"
#include "comm.h"
#include "tcpip.h"

xSemaphoreHandle tcp_connect_signal = 0;
xSemaphoreHandle tcp_send_signal = 0;
xSemaphoreHandle tcp_receive_signal = 0;
xSemaphoreHandle tcp_suspend_signal = 0;
xSemaphoreHandle tcp_close_signal = 0;

uint16_t cph_tcp_bufferindex = 0;
uint16_t cph_tcp_bufferlen = 0;
uint8_t cph_tcp_buffer[DEFAULT_TCP_BUFFERSIZE] = {0};

bool tcp_isconnected = false;

void cph_tcp_newrequest(socket_connection_t *cnx, comm_request_t *request);
void connection_settimeout(socket_connection_t * cnx, uint32_t millis);
bool connection_timeout(socket_connection_t * cnx);


void cph_tcp_onconnect(void)
{
	tcp_event_handler.on_connect();
//	printf("cph_tcp_onconnect\r\n");
	tcp_isconnected = true;
}

void cph_tcp_ondisconnect(void)
{
	tcp_isconnected = false;
	tcp_event_handler.on_disconnect();
//	printf("cph_tcp_ondisconnect\r\n");
}

void cph_tcp_ondatareceive(uint8_t *data, uint32_t len)
{
	tcp_event_handler.on_datareceive(data, len);
//	printf("cph_tcp_ondatareceive: bytes=%d data=%s\r\n", len, data);
}

static socket_event_handler_t tcp_socketevent_handler = {cph_tcp_onconnect, cph_tcp_ondisconnect, cph_tcp_ondatareceive};

void cph_tcp_init(socket_connection_t *cnx, uint8_t *endpoint, uint32_t timeout)
{
	socket_newconnection(cnx, endpoint, timeout, &tcp_socketevent_handler);
}

void connection_settimeout(socket_connection_t * cnx, uint32_t millis)
{
	cnx->max_wait_millis = millis / portTICK_RATE_MS;

	/* Remember the time on entry. */
	vTaskSetTimeOutState(&cnx->timeout);

}

bool connection_timeout(socket_connection_t * cnx)
{
	bool timeout = false;

	if (xTaskCheckForTimeOut(&cnx->timeout, &cnx->max_wait_millis) == pdTRUE)
	{
//		printf("idle_timeout.\r\n");
		timeout = true;
	}

	return timeout;
}

void cph_tcp_newrequest(socket_connection_t *cnx, comm_request_t *request)
{
	memset(request->endpoint, '\0', SOCKET_IPENDPOINT_LEN+1);
	memcpy(request->endpoint, cnx->endpoint, SOCKET_IPENDPOINT_LEN);
	request->type = REQUEST_CONNECT;
}

tcp_result cph_tcp_connect(socket_connection_t *cnx)
{
	// TODO: SEMAPHORE TAKE GIVE TASK NOTIFY
	tcp_result result;
	comm_request_t request;

	printf("cph_tcp_newrequest.\r\n");
	cph_tcp_newrequest(cnx, &request);

	// setup our default connection timeout
	printf("connection_settimeout\r\n");
	connection_settimeout(cnx, DEFAULT_TCIP_CONNECTTIMEOUT);

	if(xQueueSendToBack( xCommQueueRequest, &request, (TickType_t)0) == pdTRUE) {

		while(true) {

			if(connection_timeout(cnx)) {
				result = SYS_ERR_TCP_TIMEOUT;
				printf("socket connection timeout.\r\n");
				break;
			}

			if(cnx->socket->socket_error > 0) {
				printf("sck err: %d stat: %d\r\n", cnx->socket->socket_error, cnx->socket->socket_status);
				result = SYS_ERR_TCP_FAIL;
				break;
			}

			if(cnx->socket->socket_status == SCK_OPENED) {
				printf("SCK_OPENED\r\n");
				result = SYS_TCP_OK;
				break;
			}

			vTaskDelay(100);
		}
	} else {
		printf("failed to enqueue comm request.\r\n");
	}

	if(xSemaphoreTake(tcp_connect_signal, portMAX_DELAY)) {
		//printf("tcp_connect: connected.\r\n");
	}

	return result;
}

tcp_result cph_tcp_send(socket_connection_t *cnx, uint8_t *packet)
{
	// TODO: SEMAPHORE TAKE GIVE TASK NOTIFY
	tcp_result result;

	comm_request_t request;

	request.type = REQUEST_SEND;

	connection_settimeout(cnx, DEFAULT_TCIP_SENDTIMEOUT);

	// copy the data packet into the socket's tx_buffer
	memcpy(cnx->socket->tx_buffer, packet, SOCKET_BUFFER_LEN);

	if(xQueueSendToBack( xCommQueueRequest, &request, (TickType_t)0) == pdTRUE) {
		result = SYS_TCP_OK;
	} else {
		result = SYS_ERR_TCP_FAIL_REQUESTENQUEUE;
	}

	if(xSemaphoreTake(tcp_send_signal, portMAX_DELAY)) {
		//printf("cph_tcp_send: connected.\r\n");
	}

	return result;
}

// synchronous cph_tcp_receive message
// this function returns whatever data is received from the port
// and copies into the data buffer passed in
tcp_result cph_tcp_receive(socket_connection_t *cnx, uint8_t *data)
{
	// TODO: SEMAPHORE TAKE GIVE TASK NOTIFY
	tcp_result result;

	comm_request_t request;

	request.type = REQUEST_RECEIVE;

	connection_settimeout(cnx, DEFAULT_TCIP_RECEIVETIMEOUT);

	if(xQueueSendToBack( xCommQueueRequest, &request, (TickType_t)0) == pdTRUE) {
		result = SYS_TCP_OK;
	} else {
		result = SYS_ERR_TCP_FAIL_REQUESTENQUEUE;
	}

	if(xSemaphoreTake(tcp_receive_signal, portMAX_DELAY)) {

		if(cnx->socket->bytes_received > 0) {

			printf("cph_tcp_receive: bytes %d\r\n", cnx->socket->bytes_received);
			memcpy(data, cnx->socket->rx_buffer, cnx->socket->bytes_received);
		}
	}

	return result;
}

tcp_result cph_tcp_suspend(socket_connection_t *cnx)
{
	// TODO: SEMAPHORE TAKE GIVE TASK NOTIFY
	tcp_result result;

	comm_request_t request;

	request.type = REQUEST_SUSPEND;

	if(xQueueSendToBack( xCommQueueRequest, &request, (TickType_t)0) == pdTRUE) {
		while(true) {

			if(cnx->socket->socket_error > 0) {
				result = SYS_ERR_TCP_FAIL;
				break;
			}

			if(cnx->socket->socket_status == SCK_SUSPENDED) {
				result = SYS_TCP_OK;
				break;
			}
		}
	}

	if(xSemaphoreTake(tcp_suspend_signal, portMAX_DELAY)) {
		//printf("tcp_connect: connected.\r\n");
	}

	socket_free();
	return result;
}

tcp_result cph_tcp_resume(socket_connection_t *cnx)
{
	// TODO: SEMAPHORE TAKE GIVE TASK NOTIFY
	tcp_result result;

	comm_request_t request;

	request.type = REQUEST_CONNECT;

	if(xQueueSendToBack( xCommQueueRequest, &request, (TickType_t)0) == pdTRUE) {
		while(true) {

			if(cnx->socket->socket_error > 0) {
				result = SYS_ERR_TCP_FAIL;
				break;
			}

			if(cnx->socket->socket_status == SCK_OPENED) {
				result = SYS_TCP_OK;
				break;
			}
		}
	}

	if(xSemaphoreTake(tcp_suspend_signal, portMAX_DELAY)) {
		//printf("tcp_connect: connected.\r\n");
	}

	return SYS_TCP_OK;
}

tcp_result cph_tcp_close()
{
	// TODO: SEMAPHORE TAKE GIVE TASK NOTIFY
	BaseType_t result;

	comm_request_t request;

	request.type = REQUEST_CLOSE;

	result = xQueueSendToBack( xCommQueueRequest, &request, (TickType_t)0);

	if(result == pdTRUE) {
		//printf("request enqueued.\r\n");
	}

	if(xSemaphoreTake(tcp_close_signal, portMAX_DELAY)) {
		//printf("tcp_connect: connected.\r\n");
	}

	socket_free();
	return SYS_TCP_OK;
}

