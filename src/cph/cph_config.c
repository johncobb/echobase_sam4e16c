/*
 * cph_config.c
 *
 *  Created on: Dec 14, 2015
 *      Author: ericrudisill
 */

#include <string.h>
#include <cph.h>

static uint32_t config_buffer[IFLASH_PAGE_SIZE];

#define CONFIG_PAGE_ADDRESS	(IFLASH_ADDR + IFLASH_SIZE - IFLASH_PAGE_SIZE)

void * cph_config_init(void) {
	flash_init(FLASH_ACCESS_MODE_128, 6);
	return cph_config_read();
}

void * cph_config_read(void) {
	memcpy(config_buffer, (uint32_t*)(CONFIG_PAGE_ADDRESS), IFLASH_PAGE_SIZE);
	return (void*)config_buffer;

}

void cph_config_write(void) {
	flash_unlock(CONFIG_PAGE_ADDRESS, CONFIG_PAGE_ADDRESS + IFLASH_PAGE_SIZE - 1, 0, 0);
	flash_erase_sector(CONFIG_PAGE_ADDRESS);
	flash_write(CONFIG_PAGE_ADDRESS, config_buffer, IFLASH_PAGE_SIZE, 0);
	flash_lock(CONFIG_PAGE_ADDRESS, CONFIG_PAGE_ADDRESS + IFLASH_PAGE_SIZE - 1, 0, 0);
}

