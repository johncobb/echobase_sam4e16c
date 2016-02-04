/*
 * cph_utils.c
 *
 *  Created on: Dec 14, 2015
 *      Author: ericrudisill
 */

#include <cph.h>

uint16_t cph_utils_get_shortid_candidate(void) {
    uint32_t uid[4];
	uint32_t seed = 0;

    flash_read_unique_id(uid, 4);

    seed ^= uid[0];
    seed ^= uid[1];
    seed ^= uid[2];
    seed ^= uid[3];
//    TRACE("seed: %08X\r\n", seed);
    uint16_t shortid = 0;
	srand(seed);
	uint8_t select = (uint8_t)(seed & 0xFF);
	for (int i=0;i<select;i++) {
		shortid += rand() % 0xFFFF;
	}
//    TRACE("shortid: %04X\r\n", shortid);

    return shortid;
}
