/*
 * wan.h
 *
 *  Created on: Aug 19, 2015
 *      Author: jcobb
 */

#ifndef WAN_H_
#define WAN_H_

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "freertos_usart_serial.h"
#include "wan_msg.h"


#define WAN_BAUD_RATE           38400

#define COBS_MSG_LEN			64
#define COBS_BUFFER_LEN			128
#define WAN_RX_BUFFER_SIZE		128
#define TAGMSG_ASCII_SIZE		128
#define MAX_TAG_MSG_AGE			10000


#define TCP_APPMSG_BUFFER "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x"


uint8_t packet_buffer_ascii[TAGMSG_ASCII_SIZE];


extern QueueHandle_t xWanQueue;
extern QueueHandle_t xWanEncodedQueue;
extern QueueHandle_t xWanMessagesQueue;


enum comands {
	TAG = 0X01, ROUTER_STATUS = 0x08
};
enum wan_statuses {
	MSG_TIMEOUT = 0x0A, CONF_RETRIES = 0x0B, MSG_ERROR = 0x0C, INVALID_MSG = 0x0D, SYNC_CONF = 0x0E, FATAL = 0xEE
};



uint8_t wan_init(void);
uint8_t wan_config(void);
void WAN_SEND(uint8_t *cmd);
uint32_t wan_handler_async(uint32_t millis);

void wan_tagmsg_toascii(tag_msg_t *msg, uint8_t * buffer);


extern uint8_t wan_rx_buffer[WAN_RX_BUFFER_SIZE+1];

#endif /* WAN_H_ */
