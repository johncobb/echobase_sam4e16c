/*
 * cph_deca_port.c
 *
 *  Created on: Nov 19, 2015
 *      Author: ericrudisill
 */
#include "cph_clock.h"
#include "cph_deca_port.h"



//void cph_deca_spi_init(void) {
//
//	spi_set_master_mode(DW_SPI);
//
//	pio_configure_pin(DW_MISO_PIO_IDX, DW_MISO_PERIPH);	// MISO
//	pio_configure_pin(DW_MOSI_PIO_IDX, DW_MOSI_PERIPH);	// MOSI
//	pio_configure_pin(DW_SPCK_PIO_IDX, DW_SPCK_PERIPH);	// SPCK
//	pio_configure_pin(DW_CSn_PIO_IDX, DW_CSn_PIO_PERIPH);
//	pmc_enable_periph_clk(ID_SPI);
//
//	spi_disable(DW_SPI);
//	spi_set_clock_polarity(DW_SPI, DW_CHIP_SELECT, DW_CLOCK_POLARITY);
//	spi_set_clock_phase(DW_SPI, DW_CHIP_SELECT, DW_CLOCK_PHASE);
//	spi_set_baudrate_div(DW_SPI, DW_CHIP_SELECT, (sysclk_get_cpu_hz() / DW_SPI_BAUD_SLOW));
//	spi_set_transfer_delay(DW_SPI, DW_CHIP_SELECT, DW_DELAY_BEFORE,		DW_DELAY_BETWEEN);
//	spi_set_delay_between_chip_select(DW_SPI, DW_DELAY_BETWEEN_CS);
//
//	spi_set_fixed_peripheral_select(DW_SPI);
//	spi_configure_cs_behavior(DW_SPI, DW_CHIP_SELECT, SPI_CS_KEEP_LOW);
//	spi_set_peripheral_chip_select_value(DW_SPI, (~(1U << DW_CHIP_SELECT)));
//
//	spi_enable(DW_SPI);
//}

void cph_deca_spi_init(void)
{

	freertos_spi_if spi_if;

	const freertos_peripheral_options_t driver_options = {
	/* No receive buffer pointer needed for SPI */
	NULL,

	/* No receive buffer size needed for SPI */
	0,

	/* Cortex-M4 priority */
	configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY,

	/* Operation mode - MASTER */
	SPI_MASTER,

	/* Blocking (not async) */
	(USE_TX_ACCESS_MUTEX | USE_RX_ACCESS_MUTEX | WAIT_TX_COMPLETE | WAIT_RX_COMPLETE) };

	spi_if = freertos_spi_master_init(DW_SPI, &driver_options);

	if (spi_if != NULL) {
		pio_configure_pin(DW_MISO_PIO_IDX, DW_MISO_PERIPH);	// MISO
		pio_configure_pin(DW_MOSI_PIO_IDX, DW_MOSI_PERIPH);	// MOSI
		pio_configure_pin(DW_SPCK_PIO_IDX, DW_SPCK_PERIPH);	// SPCK
		pio_configure_pin(DW_CSn_PIO_IDX, DW_CSn_PIO_PERIPH);
		pmc_enable_periph_clk(ID_SPI);

		spi_disable(DW_SPI);
		spi_set_clock_polarity(DW_SPI, DW_CHIP_SELECT, DW_CLOCK_POLARITY);
		spi_set_clock_phase(DW_SPI, DW_CHIP_SELECT, DW_CLOCK_PHASE);
		spi_set_baudrate_div(DW_SPI, DW_CHIP_SELECT, (sysclk_get_cpu_hz() / DW_SPI_BAUD_SLOW));
		spi_set_transfer_delay(DW_SPI, DW_CHIP_SELECT, DW_DELAY_BEFORE,		DW_DELAY_BETWEEN);
//		spi_configure_cs_behavior(DW_SPI, DW_CHIP_SELECT, SPI_CS_KEEP_LOW);
		spi_configure_cs_behavior(DW_SPI, DW_CHIP_SELECT, SPI_CS_RISE_NO_TX);
		spi_set_peripheral_chip_select_value(DW_SPI, DW_CHIP_SELECT_VALUE);

		spi_enable(DW_SPI);
	}

//	return spi_if;


}


//void cph_deca_spi_set_baud(uint32_t baud) {
//	spi_disable(DW_SPI);
//	spi_set_baudrate_div(DW_SPI, DW_CHIP_SELECT, (sysclk_get_cpu_hz() / baud));
//	spi_enable(DW_SPI);
//}

void cph_deca_spi_set_baud(uint32_t baud) {
	spi_disable(DW_SPI);
	spi_set_baudrate_div(DW_SPI, DW_CHIP_SELECT, (sysclk_get_cpu_hz() / baud));
	spi_enable(DW_SPI);
}


//int cph_deca_spi_write(uint16_t headerLength, const uint8_t *headerBuffer, uint32_t bodyLength, const uint8_t *bodyBuffer) {
//	status_code_t result = STATUS_OK;
//
//	//We don't care about the values stored here
//	uint8_t pcs = DW_CHIP_SELECT;
//	uint16_t data;
//
//
//	for(int i = 0; i < headerLength; i++)
//	{
//		spi_write(DW_SPI, headerBuffer[i], 0, 0);
//		spi_read(DW_SPI, &data, &pcs);
//	}
//	for(int i = 0; i < bodyLength; i++)
//	{
//		if (i == (bodyLength - 1))
//			spi_set_lastxfer(DW_SPI);
//		spi_write(DW_SPI, bodyBuffer[i], 0, 0);
//		spi_read(DW_SPI, &data, &pcs);
//	}
//
//	while (spi_is_tx_empty(DW_SPI) == 0) ;
//
//	return result;
//}

int cph_deca_spi_write(uint16_t headerLength, const uint8_t *headerBuffer, uint32_t bodyLength, const uint8_t *bodyBuffer)
{

//	const portTickType max_block_time_ticks = 200UL / portTICK_RATE_MS;
	const portTickType max_block_time_ticks = 0;

	uint16_t data;

	/* The blocking API is being used.  Other tasks will execute while the
	SPI write is in progress. */

	for (int i=0; i<headerLength; i++) {
		freertos_spi_write_packet(DW_SPI, headerBuffer[i], 1, max_block_time_ticks);
		freertos_spi_read_packet(DW_SPI, &data, 1, max_block_time_ticks);
	}


	for (int i=0; i<bodyLength; i++) {
//		if (i == (bodyLength - 1))
//			spi_set_lastxfer(DW_SPI);

		freertos_spi_write_packet(DW_SPI, bodyBuffer[i], 1, max_block_time_ticks);
		freertos_spi_read_packet(DW_SPI, &data, 1, max_block_time_ticks);
	}



}

int cph_deca_spi_read(uint16_t headerLength, const uint8_t *headerBuffer, uint32_t readlength, uint8_t *readBuffer) {
	status_code_t result = STATUS_OK;

	uint8_t pcs = DW_CHIP_SELECT;
	uint16_t data;

	for(int i = 0; i < headerLength; i++)
	{
		spi_write(DW_SPI, headerBuffer[i], 0, 0);
		spi_read(DW_SPI, &data, &pcs);
	}
	for(int i = 0; i < readlength; i++)
	{
		if (i == (readlength - 1))
			spi_set_lastxfer(DW_SPI);
		spi_write(DW_SPI, 0xFF, 0, 0);
		spi_read(DW_SPI, &data, &pcs);
		readBuffer[i] = data & 0xFF;
	}

	while (spi_is_tx_empty(DW_SPI) == 0) ;

	return result;
}



void cph_deca_init_gpio(void) {
//	pio_configure_pin(DW_IRQ_IDX, DW_IRQ_FLAGS);
//	pio_pull_down(DW_IRQ_PIO, DW_IRQ_MASK, true);
//	pio_handler_set(DW_IRQ_PIO, DW_IRQ_PIO_ID, DW_IRQ_MASK, DW_IRQ_ATTR, process_deca_irq);
//	pio_enable_interrupt(DW_IRQ_PIO, DW_IRQ_MASK);
//
//	pmc_enable_periph_clk(DW_IRQ_PIO_ID);
}

void cph_deca_isr_disable(void) {
	pio_disable_interrupt(DW_IRQ_PIO, DW_IRQ_MASK);
}

void cph_deca_isr_enable(void) {
	pio_enable_interrupt(DW_IRQ_PIO, DW_IRQ_MASK);
}
bool cph_deca_isr_is_detected(void) {
	return pio_get_pin_value(DW_IRQ_IDX);
}

decaIrqStatus_t cph_deca_isr_mutex_on(void) {
//	TRACE("cph_deca_isr_mutex_on\r\n");
	cph_deca_isr_disable();
	return 0x00;
}

void cph_deca_isr_mutex_off(decaIrqStatus_t s) {
//	TRACE("cph_deca_isr_mutex_off\r\n");
	cph_deca_isr_enable();
}

void cph_deca_isr(void) {
	do {
		dwt_isr();
	} while (cph_deca_isr_is_detected() == 1);
}


//void cph_deca_reset(void) {
//	// low gate - turn on MOSFET and drive to ground
//	pio_set_pin_low(DW_RSTSWn_PIO_IDX);
//
//	// sleep here??
//
//	// high gate - turn off MOSFET and open drain
//	pio_set_pin_high(DW_RSTSWn_PIO_IDX);
//
//	// Why sleep here?  Left over from original code.
//	cph_millis_delay(1);
//}

void cph_deca_reset(void) {
	// low gate - turn on MOSFET and drive to ground
	pio_set_pin_low(DW_RSTn_PIO_IDX);

	// sleep here??

	// high gate - turn off MOSFET and open drain
	pio_set_pin_high(DW_RSTn_PIO_IDX);

	// Why sleep here?  Left over from original code.
	cph_millis_delay(1);
}
