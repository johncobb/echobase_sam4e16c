/*
 * cph_clock.c
 *
 *  Created on: Dec 1, 2015
 *      Author: jcobb
 */


#include "cph_clock.h"

volatile uint32_t g_cph_millis = 0;
volatile uint32_t g_cph_timeout = 0;


void cph_clock_init(void)
{
	sysclk_init();
	// todo: not needed when using FreeRTOS
//	SysTick_Config(sysclk_get_cpu_hz() / 1000);

}

// todo: not needed when using FreeRTOS
// handled in vApplicationTickHook()
//void SysTick_Handler(void) {
//	g_cph_millis++;
//}

void cph_clock_millis_delay(uint32_t millis) {
	uint32_t current;
	current = g_cph_millis;
	while ((g_cph_millis - current) < millis)
		;
}

void cph_set_timeout(uint32_t timeout)
{
	g_cph_timeout = cph_get_millis() + timeout;
}



