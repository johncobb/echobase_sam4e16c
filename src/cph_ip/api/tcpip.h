/*
 * tcpip.h
 *
 *  Created on: Jul 21, 2015
 *      Author: jcobb
 */

#ifndef TCPIP_H_
#define TCPIP_H_

#include "socket.h"

#define DEFAULT_TCP_BUFFERSIZE		256

#define DEFAULT_TCIP_CONNECTTIMEOUT	20000
#define DEFAULT_TCIP_SENDTIMEOUT	1000
#define DEFAULT_TCIP_RECEIVETIMEOUT	1000

typedef enum
{
	SYS_TCP_OK = 0,
	SYS_ERR_TCP_FAIL,
	SYS_ERR_TCP_FAIL_REQUESTENQUEUE,
	SYS_ERR_TCP_TIMEOUT
}tcp_result;


extern volatile bool tcp_isconnected;

uint8_t cph_tcp_buffer[DEFAULT_TCP_BUFFERSIZE];

typedef tcp_result (*tcp_func_t)(uint8_t *data, uint32_t len);
typedef void (*tcp_data_callback_func_t)(uint8_t *data, uint32_t len);


typedef void (*tcp_connect_func_t)(void);
typedef void (*tcp_ondisconnect_func_t)(void);
typedef void (*tcp_onreceive_func_t)(uint8_t *data, uint32_t len);

typedef struct
{
	tcp_connect_func_t on_connect;
	tcp_ondisconnect_func_t on_disconnect;
	tcp_onreceive_func_t on_datareceive;

}tcp_event_handler_t;

tcp_event_handler_t tcp_event_handler;
tcp_event_handler_t tcp_event_handler2;



sys_result cph_tcp_receivecb(uint8_t *data, uint32_t len);


tcp_result tcp_init(tcp_func_t handle_data);

void cph_tcp_init(socket_connection_t *cnx, uint8_t *endpoint, uint32_t timeout);
void cph_tcp_init2(socket_connection_t *cnx, uint8_t *endpoint, uint32_t timeout, socket_event_handler_t *handler);

tcp_result tcp_input(void);
tcp_result tcp_output(void);

tcp_result cph_tcp_connect(socket_connection_t *cnx);
tcp_result cph_tcp_send(socket_connection_t *cnx, uint8_t *packet);
tcp_result cph_tcp_receive(socket_connection_t *cnx, uint8_t *data);
tcp_result cph_tcp_suspend(socket_connection_t *cnx);
tcp_result cph_tcp_resume(socket_connection_t *cnx);
tcp_result cph_tcp_close();
tcp_result cph_tcp_listen(uint8_t *endpoint, uint32_t len);


void cph_tcp_onconnect(void);
void cph_tcp_ondisconnect(void);
void cph_tcp_ondatareceive(uint8_t *data, uint32_t len);


#endif /* TCPIP_H_ */
