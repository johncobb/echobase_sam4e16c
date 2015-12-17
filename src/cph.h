/*
 * cph.h
 *
 *  Created on: Dec 11, 2015
 *      Author: jcobb
 */

#ifndef SRC_CPH_CPH_H_
#define SRC_CPH_CPH_H_

#include <asf.h>

//#define MAIN_TEST

#include <conf_board.h>
#include <cph_clock.h>
#include <unit_test.h>

#define BASESTATION_ID			"20000000"
#define SOFTWARE_VER_STRING  	"Version 2.01    "


#define WAN_TAGMSG_FORMAT_ASCII		1
#define WAN_RTRMSG_FORMAT_ASCII		1

#define LOG_TAGMSG					1
#define LOG_RTRMSG					1
//#define LOG_MSG_COBSENCODED			1
//#define LOG_MSG_COBSDECODED			1


#define IP_ENDPIONT		"bs.cphandheld.com"


#endif /* SRC_CPH_CPH_H_ */
