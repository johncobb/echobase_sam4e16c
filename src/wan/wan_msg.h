/*
 * wan_msg.h
 *
 *  Created on: Oct 14, 2015
 *      Author: jcobb
 */

#ifndef WAN_MSG_H_
#define WAN_MSG_H_

typedef struct {
	uint8_t rssi;
	uint64_t mac;
	uint16_t batt;
	uint16_t temp;

} wan_msg_t;

typedef struct {
	uint8_t command;
} cmd_header_t;

typedef struct {
	uint8_t command;
	uint16_t pan_id;
	uint16_t short_id;
	uint8_t channel;
} cmd_config_ntw_t;

enum {
	CMD_SEND = 0x01,
	CMD_ACK_SEND = 0x02,
	CMD_CONFIG_NETWORK = 0x03,
	CMD_GET_ADDRESS = 0x04,
	CMD_IN_PROX = 0x05,
	CMD_OUT_PROX = 0x06,
	CMD_CONFIG_DONE = 0x07,
	CMD_ROUTER_STATUS = 0X08
};
#define RAMDISK_META_OFFSET		10

typedef struct __attribute__((__packed__)) {
	uint8_t messageType;		// 0x01 for tag
	uint64_t routerMac;			// full MAC or source router
	uint16_t routerShort;		// short id of MAC source router - used for two-way comm
	uint64_t tagMac;			// full MAC of ble tag
	uint16_t tagConfigSet;		// current configuration set id of tag
	uint16_t tagSerial;			// sequential serial number of tag’s report - used to group pings
	uint16_t tagStatus;			// tag status value (bit mask) within adv data
	uint8_t tagLqi;				// link quality indicator of tag-to-router signal
	uint8_t tagRssi;			// signal strength of tag-to-router signal
	uint32_t tagBattery;		// raw ADC value of battery read - currently not reliable but kept for future uses
	uint32_t tagTemperature;	// raw ADC value of temp sensor on tag - reserved for future use
//	uint32_t lastSent;			// RAMDISK META DATA
//	uint16_t temp;				// RAMDISK META DATA
//	void *next;					// RAMDISK META DATA

} tag_msg_t;




typedef struct {
	tag_msg_t *tag;
	uint32_t lastSent;
	uint16_t temp;
	void *next;
}ramdisk_record_t;







#endif /* WAN_MSG_H_ */
