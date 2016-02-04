/*
 * cph.h
 *
 *  Created on: Dec 11, 2015
 *      Author: jcobb
 */

#ifndef SRC_CPH_CPH_H_
#define SRC_CPH_CPH_H_

#include "asf.h"

//#define MAIN_TEST

#include "conf_board.h"
#include "cph_clock.h"
#include "unit_test.h"

// decawave includes
#include "cph_config.h"
#include <cph_deca.h>
#include <globals.h>
#include <configure.h>

#define BASESTATION_ID			"20000000"
#define SOFTWARE_VER_STRING  	"Version 2.01    "


#define WAN_TAGMSG_FORMAT_ASCII		1
#define WAN_RTRMSG_FORMAT_ASCII		1

#define LOG_TAGMSG					1
#define LOG_ANCHORMSG					1
//#define LOG_MSG_COBSENCODED			1
//#define LOG_MSG_COBSDECODED			1


#define IP_ENDPIONT					"bs.cphandheld.com"
#define IP_ENDPIONT_PRINTTASK		"google.com"


// decawave defines
#define FW_MAJOR				0x01
#define FW_MINOR				0x01

//#define ANCHOR
//#define TAG
//#define SENDER
#define LISTENER

#define TRACE(...)				printf(__VA_ARGS__)


typedef struct PACKED {
	uint8_t magic[4];
	uint8_t hw_major;
	uint8_t hw_minor;
	uint8_t fw_major;
	uint8_t fw_minor;
	uint16_t panid;
	uint16_t shortid;
	dwt_config_t dwt_config;
	uint8_t mode;
	uint32_t sender_period;
} cph_config_t;

extern cph_config_t * cph_config;

extern int cph_mode;

extern uint16_t cph_coordid;





#endif
