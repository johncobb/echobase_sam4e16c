/*
 * test_ramdisk.c
 *
 *  Created on: Dec 11, 2015
 *      Author: jcobb
 */

#include <stdbool.h>
#include <cph.h>
#include "cph_board.h"
#include "ramdisk.h"


void test_ramdisk(void)
{
	uint8_t user_input = '2';
	struct record_t test;
	struct record_t local_copy;
	struct record_t test_t;

	srand(cph_get_millis());

	test.wan_msg.tagMac = 1;
	test.wan_msg.tagBattery = 2;
	test.wan_msg.tagRssi = 3;
	struct record_t test_2;
	local_copy = test;

	ramdisk_init();

	while(true) {

		test_2.wan_msg.tagMac = rand();
		test_2.wan_msg.tagRssi = rand() % 65535;
		test_2.wan_msg.tagBattery = rand() % 65535;
		test_2.next = NULL;

		if(user_input == '2')
			ramdisk_write(test_2);

		else if(user_input =='1')
			ramdisk_write(test);

		else if(user_input == 'f')
			local_copy = *ramdisk_next(ramdisk_find(test.wan_msg.tagMac));

		else if(user_input == 'p')
			print_records();

		else if(user_input == 'e')
			ramdisk_erase(local_copy);

		else if(user_input == 'r')
			local_copy = test;

		else
			user_input = 'q';

//		scanf(" %c", &user_input);

		if(user_input == 'q')
			break;

	}

}
