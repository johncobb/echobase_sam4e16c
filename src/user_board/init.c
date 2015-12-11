/**
 * \file
 *
 * \brief Arduino Due/X board init.
 *
 * Copyright (c) 2011 - 2014 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */
 /**
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#include "compiler.h"

#include "board.h"
#include "conf_board.h"

void board_init(void)
{
#ifndef CONF_BOARD_KEEP_WATCHDOG_AT_INIT
	/* Disable the watchdog */
	WDT->WDT_MR = WDT_MR_WDDIS;
#endif


#ifdef CONF_BOARD_MODEM_CONTROL
	pio_configure(MDM_ONOFF_PIO, MDM_ONOFF_TYPE, MDM_ONOFF_MASK, MDM_ONOFF_ATTR);
	pio_configure(MDM_ENABLE_PIO, MDM_ENABLE_TYPE, MDM_ENABLE_MASK, MDM_ENABLE_ATTR);
	pio_configure(MDM_RESET_PIO, MDM_RESET_TYPE, MDM_RESET_MASK, MDM_RESET_ATTR);
	pio_configure(MDM_POWMON_PIO, MDM_POWMON_TYPE, MDM_POWMON_MASK, MDM_POWMON_ATTR);
	// enable the peripheral pins
	pmc_enable_periph_clk(ID_PIOD);

#endif

#ifdef CONF_BOARD_WAN_CONTROL
	pio_configure(WAN_INT1_PIO, WAN_INT1_TYPE, WAN_INT1_MASK, WAN_INT1_ATTR);
	pio_configure(WAN_INT2_PIO, WAN_INT2_TYPE, WAN_INT2_MASK, WAN_INT2_ATTR);
	pio_configure(WAN_INT3_PIO, WAN_INT3_TYPE, WAN_INT3_MASK, WAN_INT3_ATTR);
	// enable the wan peripheral clock
	pmc_enable_periph_clk(ID_PIOB);
#endif


#ifdef CONF_BOARD_LEDS
	pio_configure(MCU_LED1_PIO, MCU_LED1_TYPE, MCU_LED1_MASK, MCU_LED1_ATTR);
	pio_configure(MCU_LED2_PIO, MCU_LED2_TYPE, MCU_LED2_MASK, MCU_LED2_ATTR);
#endif

//#ifdef CONF_BOARD_UART_CONSOLE
//	pio_configure(PINS_UART1_PIO, PINS_UART1_TYPE, PINS_UART1_MASK, PINS_UART1_ATTR);
//#endif

}


void board_init_modem_usart(void)
{
	// todo: research why this needs to be called in FreeRTOS modem compared
	// to a single threaded task
#ifdef CONF_MODEM_USART
	pio_configure(PINS_USART1_PIO, PINS_USART1_TYPE, PINS_USART1_MASK, PINS_USART1_ATTR);
#endif
}

void board_init_wan_usart(void)
{
#ifdef CONF_WAN_USART
	pio_configure(PINS_USART0_PIO, PINS_USART0_TYPE, PINS_USART0_MASK, PINS_USART0_ATTR);
#endif
}

void board_init_printf_uart(void)
{
#ifdef CONF_PRINTF_UART
	pio_configure(PINS_UART0_PIO, PINS_UART0_TYPE, PINS_UART0_MASK, PINS_UART0_ATTR);
#endif
}

