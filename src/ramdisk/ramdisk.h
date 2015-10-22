/*
 * ramdisk.h
 *
 *  Created on: Nov 26, 2014
 *      Author: Will
 */

#ifndef RAMDISK_RAMDISK_H_
#define RAMDISK_RAMDISK_H_

#include <stdint.h>
#include "wan_msg.h"

#define SIZE_OF 25

void ramdisk_init();
uint8_t ramdisk_write(tag_msg_t to_write);
uint8_t ramdisk_erase(tag_msg_t to_remove);
uint8_t ramdisk_find(uint64_t target, tag_msg_t * result);
uint8_t ramdisk_next(tag_msg_t * target, tag_msg_t * result);


#endif /* RAMDISK_RAMDISK_H_ */
