/*
 * wan_proc.h
 *
 *  Created on: Oct 27, 2015
 *      Author: jcobb
 */

#ifndef WAN_PROC_H_
#define WAN_PROC_H_



void parse_tag_msg(uint8_t *data, tag_msg_t *tag);
void parse_anchor_msg(uint8_t *data, anchor_msg_t *msg);
static void wan_proc_task(void *pvParameters);



#endif /* WAN_PROC_H_ */
