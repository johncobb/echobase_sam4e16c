/*
 * sender.c
 *
 *  Created on: Dec 9, 2015
 *      Author: ericrudisill
 */

#include <cph.h>

static cph_deca_msg_discover_announce_t tx_discover_msg = {
MAC_FC,			// mac.ctl - data frame, frame pending, pan id comp, short dest, short source
		0,				// mac.seq
		MAC_PAN_ID,		// mac.panid
		0xFFFF,			// mac.dest
		MAC_TAG_ID,		// mac.source
		FUNC_DISC_ANNO,	// functionCode
		0x0000			// mac_cs
		};


void sender_run(void) {

	uint32_t announce_coord_ts = 0;
	uint32_t elapsed = 0;
	uint32_t count = 0;

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

	uint8_t seq = 0;
	uint32_t status_reg;

	while (1) {

		printf("sending %d\r\n", count++);

		// Write message to frame buffer
		tx_discover_msg.header.seq = seq++;
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);
		dwt_writetxdata(sizeof(tx_discover_msg), (uint8_t*) &tx_discover_msg, 0);
		dwt_writetxfctrl(sizeof(tx_discover_msg), 0);

		dwt_starttx(DWT_START_TX_IMMEDIATE);

		while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_TXFRS)) {
		};
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_TXFRS);

		cph_millis_delay(cph_config->sender_period);
	}
}
