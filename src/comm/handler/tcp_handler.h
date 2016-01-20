/*
 * tcp_handler.h
 *
 *  Created on: Jan 19, 2016
 *      Author: jcobb
 */

#ifndef SRC_COMM_HANDLER_TCP_HANDLER_H_
#define SRC_COMM_HANDLER_TCP_HANDLER_H_

#include "modem.h"

#define TCP_BUFFER_LEN		1024
extern uint8_t tcp_buffer[];

sys_result tcp_handle_data(uint8_t *data, uint32_t len);

#endif /* SRC_COMM_HANDLER_TCP_HANDLER_H_ */
