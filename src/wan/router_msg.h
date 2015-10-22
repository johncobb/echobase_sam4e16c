/*
 * router_msg.h
 *
 *  Created on: Oct 14, 2015
 *      Author: jcobb
 */

#ifndef ROUTER_MSG_H_
#define ROUTER_MSG_H_

typedef struct router_msg_t {
	uint8_t messageType;		// 0x08 for router
	uint64_t routerMac;			// full MAC or source router
	uint16_t routerShort;		// short id of MAC source router - used for two-way comm
	uint8_t routerReset;		// recent reset reason
	uint8_t resetTask;			// task that caused the reset
	uint16_t routerSerial;		// sequential serial number of router msg
	uint8_t routerConfigSet;	// current configuration set id of router
	uint32_t routerMsgCount;	// current tag messages received count since reset
	uint32_t routerUptime;		// current ms since reset
	uint32_t routerBattery;		// raw ADC value of battery read - currently not reliable but kept for future uses
	uint32_t routerTemperature;	// raw ADC value of temp sensor on tag - reserved for future use

} router_msg_t;

typedef struct {
	uint8_t command;
//uint8_t message_length;
} cmd_router_status_header_t;

#endif /* ROUTER_MSG_H_ */






