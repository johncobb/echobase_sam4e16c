/*
 * anchor.c
 *
 *  Created on: Dec 9, 2015
 *      Author: ericrudisill
 */

#include <cph.h>

/* Buffer to store received messages.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
#define MAXRXSIXZE		64
static uint8 rx_buffer[MAXRXSIXZE];

static volatile uint32 frame_len;
static volatile uint32_t int_count = 0;

#define SIGNAL_EMPTY	0x00
#define SIGNAL_RCV		0xff
#define SIGNAL_ERR		0xee
#define SIGNAL_ERR_LEN	0xaa

static volatile uint8_t trx_signal = SIGNAL_EMPTY;
static volatile uint32_t error_status_reg = 0;



static void irq_handler(uint32_t id, uint32_t mask) {
	int_count++;
	do {
		dwt_isr();
	} while (cph_deca_isr_is_detected() == 1);
}

static void irq_init(void) {

	pio_configure_pin(DW_IRQ_IDX, DW_IRQ_FLAGS);
	pio_pull_down(DW_IRQ_PIO, DW_IRQ_MASK, true);
	pio_handler_set(DW_IRQ_PIO, DW_IRQ_PIO_ID, DW_IRQ_MASK, DW_IRQ_ATTR, irq_handler);
	pio_enable_interrupt(DW_IRQ_PIO, DW_IRQ_MASK);

	pio_handler_set_priority(DW_IRQ_PIO, DW_IRQ_IRQ, 0);

	pmc_enable_periph_clk(DW_IRQ_PIO_ID);
}

static void rxcallback(const dwt_callback_data_t *rxd) {
	if (rxd->event == DWT_SIG_RX_OKAY) {
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);
		if (rxd->datalength <= MAXRXSIXZE) {
			dwt_readrxdata(rx_buffer, rxd->datalength, 0);
			trx_signal = SIGNAL_RCV;
			frame_len = rxd->datalength;
		} else {
			trx_signal = SIGNAL_ERR_LEN;
			error_status_reg = rxd->status;
		}
	}
	else {
		trx_signal = SIGNAL_ERR;
		error_status_reg = rxd->status;
	}
}

void listener_run(void) {

	uint32_t announce_coord_ts = 0;
	uint32_t elapsed = 0;
	uint32_t last_ts = 0;
	uint32_t count = 0;

	irq_init();
	pio_disable_interrupt(DW_IRQ_PIO, DW_IRQ_MASK);



	// Setup DW1000
	dwt_txconfig_t txconfig;

	// Setup DECAWAVE
	reset_DW1000();
	spi_set_rate_low();
	dwt_initialise(DWT_LOADUCODE);
	spi_set_rate_high();

	dwt_configure(&cph_config->dwt_config);

	dwt_setpanid(0x4350);
	dwt_setaddress16(0x1234);

	// Clear CLKPLL_LL
	dwt_write32bitreg(SYS_STATUS_ID, 0x02000000);

	uint32_t id = dwt_readdevid();
	printf("Device ID: %08X\r\n", id);


#if 1

	dwt_setcallbacks(0, rxcallback);
	dwt_setinterrupt(
			DWT_INT_TFRS | DWT_INT_RFCG
					| (DWT_INT_ARFE | DWT_INT_RFSL | DWT_INT_SFDT | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO /*| DWT_INT_RXPTO*/),
			1);

	pio_enable_interrupt(DW_IRQ_PIO, DW_IRQ_MASK);

	dwt_rxenable(0);

	while (1) {

		elapsed = cph_get_millis() - last_ts;
		if (elapsed > 5000) {
			printf("alive %d\r\n", count++);
			last_ts = cph_get_millis();
		}

		if (trx_signal == SIGNAL_RCV) {
			printf("[RCV] %d - ", frame_len);
			for (int i = 0; i < frame_len; i++) {
				printf("%02X ", rx_buffer[i]);
			}
			printf("\r\n");
			trx_signal = SIGNAL_EMPTY;
			dwt_rxenable(0);
		}
		else if(trx_signal == SIGNAL_ERR) {
			printf("ERROR: %08X\r\n", error_status_reg);
			trx_signal = SIGNAL_EMPTY;
			dwt_rxenable(0);
		}
		else if(trx_signal == SIGNAL_ERR_LEN) {
			printf("ERROR LENGTH: %08X\r\n", error_status_reg);
			trx_signal = SIGNAL_EMPTY;
			dwt_rxenable(0);
		}
	}

#else

	while (1) {

		/* Activate reception immediately. */
		dwt_rxenable(0);

		while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR))) {
		};

		if (status_reg & SYS_STATUS_RXFCG) {
			uint32 frame_len;

			dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_RXFCG);

			frame_len = dwt_read32bitreg(RX_FINFO_ID) & RX_FINFO_RXFL_MASK_1023;
			if (frame_len <= MAXRXSIXZE) {
				dwt_readrxdata(rx_buffer, frame_len, 0);
			} else {
				frame_len = 0;
			}

			if (frame_len > 0) {
				printf("[RCV] ");
				for (int i = 0; i < frame_len; i++) {
					printf("%02X ", rx_buffer[i]);
				}
				printf("\r\n");
			} else {
				printf("ERROR: frame_len == %d\r\n", frame_len);
			}

		} else {

			printf("ERROR: dwt_rxenable has status of %08X\r\n", status_reg);
			/* Clear RX error events in the DW1000 status register. */
			dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_CLKPLL_LL);
		}
	}
#endif

}
