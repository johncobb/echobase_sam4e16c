/*
 * cph_clock.h
 *
 *  Created on: Dec 1, 2015
 *      Author: jcobb
 */

#ifndef SRC_CPH_CPH_CLOCK_H_
#define SRC_CPH_CPH_CLOCK_H_

#include <stdint.h>
#include <cph.h>

extern volatile uint32_t g_cph_millis;
extern volatile uint32_t g_cph_timeout;



#define cph_get_millis()	g_cph_millis
#define cph_timeout() (g_cph_millis > g_cph_timeout)


void cph_clock_init(void);
void cph_clock_millis_delay(uint32_t millis);
void cph_set_timeout(uint32_t timeout);

#endif /* SRC_CPH_CPH_CLOCK_H_ */
