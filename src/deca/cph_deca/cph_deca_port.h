/*
 * cph_deca_port.h
 *
 *  Created on: Nov 19, 2015
 *      Author: ericrudisill
 */

#ifndef SRC_CPH_CPH_DECA_PORT_H_
#define SRC_CPH_CPH_DECA_PORT_H_

#include <cph.h>
#include <deca_device_api.h>
#include "cph_clock.h"




void cph_deca_spi_init(void);
void cph_deca_spi_set_baud(uint32_t baud);
int cph_deca_spi_write(uint16_t headerLength, const uint8_t *headerBuffer, uint32_t bodylength, const uint8_t *bodyBuffer);
int cph_deca_spi_read(uint16_t headerLength, const uint8_t *headerBuffer, uint32_t readlength, uint8_t *readBuffer);

void cph_deca_init_gpio(void);

void cph_deca_isr_disable(void);
void cph_deca_isr_enable(void);
bool cph_deca_isr_is_detected(void) ;
decaIrqStatus_t cph_deca_isr_mutex_on(void);
void cph_deca_isr_mutex_off(decaIrqStatus_t s);
void cph_deca_isr(void);

void cph_deca_reset(void);


// Redirect DW API calls to our functions
#define writetospi				cph_deca_spi_write
#define readfromspi				cph_deca_spi_read
#define reset_DW1000			cph_deca_reset
#define portGetTickCount		cph_get_millis
#define decamutexoff			cph_deca_isr_mutex_off
#define decamutexon				cph_deca_isr_mutex_on

#define spi_set_rate_low()		cph_deca_spi_set_baud(1000000)
#define spi_set_rate_high()		cph_deca_spi_set_baud(8000000)


#define cph_millis_delay		cph_clock_millis_delay


#define port_CheckEXT_IRQ()			pio_get_pin_value(DW_IRQ_IDX)
#define port_EnableEXT_IRQ()		pio_enable_interrupt(DW_IRQ_PIO, DW_IRQ_MASK)
#define port_DisableEXT_IRQ()		pio_disable_interrupt(DW_IRQ_PIO, DW_IRQ_MASK)


#endif /* SRC_CPH_CPH_DECA_PORT_H_ */
