
#ifndef _CPH_BOARD_H
#define _CPH_BOARD_H

#include "compiler.h"
#include "system_sam4e.h"
#include "exceptions.h"
#include "pio.h"
#include "pmc.h"

#warning "check ECHOBASE_REV before testing"

//#define		ECHOBASE_REV_A01		1
#define		ECHOBASE_REV_A02		1
extern uint32_t clock_millis;

uint32_t clock_time(void);

#define UNIT_TEST_YIELD		vTaskDelay(10); continue;

// special function to initialize usart after configuring pins
void board_init_modem_usart(void);
void board_init_wan_usart(void);
void board_init_printf_uart(void);

/** Board oscillator settings */
#define BOARD_FREQ_SLCK_XTAL        (32768U)
#define BOARD_FREQ_SLCK_BYPASS      (32768U)
#define BOARD_FREQ_MAINCK_XTAL      (12000000U)
#define BOARD_FREQ_MAINCK_BYPASS    (12000000U)

/** Master clock frequency */
#define BOARD_MCK                   CHIP_FREQ_CPU_MAX

/** board main clock xtal statup time */
#define BOARD_OSC_STARTUP_US   15625

/** Name of the board */
#define BOARD_NAME "CPH-BOARD"
/** Board definition */
#define cphBoard
/** Family definition (already defined) */
#define sam4e
/** Core definition */
#define cortexm4

// PRINTF/TRACE
/* ------------------------------------------------------------------------ */
/* UART0                                                                    */
/* ------------------------------------------------------------------------ */
#define PRINTF_UART		UART0
#define PRINTF_UART_ID		ID_UART0

#define PINS_UART0_PIO  PIOA
#define PINS_UART0_ID   ID_PIOA
#define PINS_UART0_TYPE PIO_PERIPH_A
#define PINS_UART0_MASK (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0)
#define PINS_UART0_ATTR PIO_DEFAULT


// CONSOLE
/* ------------------------------------------------------------------------ */
/* UART1                                                                   */
/* ------------------------------------------------------------------------ */
#define CONSOLE_UART        UART1
#define CONSOLE_UART_ID     ID_UART1

#define PINS_UART1_PIO		PIOA
#define PINS_UART1_ID		ID_PIOA
#define PINS_UART1_TYPE		PIO_PERIPH_A
#define PINS_UART1_MASK 	(PIO_PA5C_URXD1 | PIO_PA6C_UTXD1)
#define PINS_UART1_ATTR		PIO_DEFAULT


// WAN
/* ------------------------------------------------------------------------ */
/* USART0                                                                   */
/* ------------------------------------------------------------------------ */
#define WAN_USART			USART0
#define WAN_USART_ID		ID_USART0

#define PINS_USART0_PIO		PIOB
#define PINS_USART0_ID		ID_PIOB
#define PINS_USART0_TYPE	PIO_PERIPH_B
#define PINS_USART0_MASK 	(PIO_PB0C_RXD0 | PIO_PB1C_TXD0)
#define PINS_USART0_ATTR	PIO_DEFAULT


// MODEM
/* ------------------------------------------------------------------------ */
/* USART1                                                                   */
/* ------------------------------------------------------------------------ */
#define MODEM_USART			USART1
#define MODEM_USART_ID		ID_USART1

#define PINS_USART1_PIO		PIOA
#define PINS_USART1_ID		ID_PIOA
#define PINS_USART1_TYPE	PIO_PERIPH_A
#define PINS_USART1_MASK 	(PIO_PA21A_RXD1 | PIO_PA22A_TXD1)
#define PINS_USART1_ATTR	PIO_DEFAULT


/*----------------------------------------------------------------------------*/
/*	LEDS																	  */
/*----------------------------------------------------------------------------*/
#ifdef ECHOBASE_REV_A01
// MCU_LED1
#define MCU_LED1       		PIO_PD22_IDX
#define MCU_LED1_MASK  		(1 << 22)
#define MCU_LED1_PIO   		PIOD
#define MCU_LED1_ID    		ID_PIOD
#define MCU_LED1_TYPE  		PIO_OUTPUT_0
#define MCU_LED1_ATTR  		PIO_DEFAULT
#endif


#ifdef ECHOBASE_REV_A02
// MCU_LED1
#define MCU_LED1       		PIO_PA0_IDX
#define MCU_LED1_MASK  		(1 << 0)
#define MCU_LED1_PIO   		PIOA
#define MCU_LED1_ID    		ID_PIOA
#define MCU_LED1_TYPE  		PIO_OUTPUT_0
#define MCU_LED1_ATTR  		PIO_DEFAULT
#endif


#ifdef ECHOBASE_REV_A01
// MCU_LED2
#define MCU_LED2       		PIO_PD23_IDX
#define MCU_LED2_MASK  		(1 << 23)
#define MCU_LED2_PIO   		PIOD
#define MCU_LED2_ID    		ID_PIOD
#define MCU_LED2_TYPE  		PIO_OUTPUT_0
#define MCU_LED2_ATTR  		PIO_DEFAULT
#endif

#ifdef ECHOBASE_REV_A02
// MCU_LED2
#define MCU_LED2       		PIO_PA1_IDX
#define MCU_LED2_MASK  		(1 << 1)
#define MCU_LED2_PIO   		PIOA
#define MCU_LED2_ID    		ID_PIOA
#define MCU_LED2_TYPE  		PIO_OUTPUT_0
#define MCU_LED2_ATTR  		PIO_DEFAULT
#endif

/*----------------------------------------------------------------------------*/
/*	WAN INTs																	  */
/*----------------------------------------------------------------------------*/

// WAN_INT1
#define WAN_INT1_IDX       	PIO_PD20_IDX
#define WAN_INT1_MASK  		(1 << 20)
#define WAN_INT1_PIO   		PIOD
#define WAN_INT1_ID    		ID_PIOD
#define WAN_INT1_TYPE  		PIO_TYPE_PIO_INPUT
#define WAN_INT1_ATTR  		PIO_DEFAULT

// WAN_INT2
#define WAN_INT2_IDX       	PIO_PD21_IDX
#define WAN_INT2_MASK  		(1 << 21)
#define WAN_INT2_PIO   		PIOD
#define WAN_INT2_ID    		ID_PIOD
#define WAN_INT2_TYPE  		PIO_TYPE_PIO_INPUT
#define WAN_INT2_ATTR  		PIO_DEFAULT

// WAN_INT3
#define WAN_INT3_IDX       	PIO_PD22_IDX
#define WAN_INT3_MASK  		(1 << 22)
#define WAN_INT3_PIO   		PIOD
#define WAN_INT3_ID    		ID_PIOD
#define WAN_INT3_TYPE  		PIO_TYPE_PIO_INPUT
#define WAN_INT3_ATTR  		PIO_DEFAULT

// TODO: MODEM_ONOFF MISSING FROM SCHEMATIC (NEEDS JUMPER)
/*! MODEM ON/OFF.*/
#define MDM_ONOFF_IDX       	PIO_PD28_IDX
#define MDM_ONOFF_MASK  		(1 << 28)
#define MDM_ONOFF_PIO   		PIOD
#define MDM_ONOFF_ID    		ID_PIOD
#define MDM_ONOFF_TYPE  		PIO_OUTPUT_0
#define MDM_ONOFF_ATTR  		PIO_DEFAULT

/*! MODEM ENABLE.*/
#define MDM_ENABLE_IDX       	PIO_PD26_IDX
#define MDM_ENABLE_MASK  		(1 << 26)
#define MDM_ENABLE_PIO   		PIOD
#define MDM_ENABLE_ID    		ID_PIOD
#define MDM_ENABLE_TYPE  		PIO_OUTPUT_0
#define MDM_ENABLE_ATTR  		PIO_DEFAULT
//#define MDM_ENABLE_ATTR  		PIO_PULLUP

/*! MODEM RESET.*/
#define MDM_RESET_IDX       	PIO_PD25_IDX
#define MDM_RESET_MASK  		(1 << 25)
#define MDM_RESET_PIO   		PIOD
#define MDM_RESET_ID    		ID_PIOD
#define MDM_RESET_TYPE  		PIO_OUTPUT_0
#define MDM_RESET_ATTR  		PIO_DEFAULT

/*! MODEM POWMON.*/
#define MDM_POWMON_IDX       	PIO_PD24_IDX
#define MDM_POWMON_MASK  		(1 << 24)
#define MDM_POWMON_PIO   		PIOD
#define MDM_POWMON_ID    		ID_PIOD
#define MDM_POWMON_TYPE  		PIO_INPUT
#define MDM_POWMON_ATTR			PIO_PULLUP


#endif  // _CPH_BOARD_H
