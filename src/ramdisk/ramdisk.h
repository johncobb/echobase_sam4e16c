/*
 * ramdisk.h
 *
 *  Created on: Dec 10, 2015
 *      Author: jcobb
 */

#ifndef SRC_CPH_RAMDISK_H_
#define SRC_CPH_RAMDISK_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include "wan_msg.h"

#define RAMDISK_SIZE 20

struct record_t {
	tag_msg_t wan_msg;
	struct record_t *next;
};


void print_records();
void ramdisk_init();
int ramdisk_write(struct record_t to_write);
int ramdisk_erase(struct record_t to_remove);
struct record_t *ramdisk_find(uint64_t target);
struct record_t *ramdisk_next(struct record_t *target);

#endif /* SRC_CPH_RAMDISK_H_ */
