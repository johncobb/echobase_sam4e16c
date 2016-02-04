/*
 * anchor.c
 *
 *  Created on: Dec 9, 2015
 *      Author: ericrudisill
 */

#include <stdio.h>
#include <string.h>

#include <cph.h>
#include <cph_deca_port.h>
#include <deca_device_api.h>
#include <deca_regs.h>
#include <deca_sleep.h>


static cph_deca_msg_range_response_t tx_range_response_t = {
		MAC_FC,			// mac.ctl - data frame, frame pending, pan id comp, short dest, short source
		0,				// mac.seq
		MAC_PAN_ID,		// mac.panid
		MAC_TAG_ID,		// mac.dest
		MAC_ANCHOR_ID,	// mac.source
		FUNC_RANGE_RESP,		// functionCode
		0x00000000,		// pollRxTs
		0x00000000,		// respTxTs
		0x0000			// mac_cs
		};

static cph_deca_msg_discover_reply_t tx_discover_reply = {
		MAC_FC,			// mac.ctl - data frame, frame pending, pan id comp, short dest, short source
		0,				// mac.seq
		MAC_PAN_ID,		// mac.panid
		MAC_TAG_ID,		// mac.dest
		MAC_ANCHOR_ID,	// mac.source
		FUNC_DISC_REPLY,	// functionCode
		0x0000,			// coordid
		0x0000			// mac_cs
		};

static cph_deca_msg_coord_announce_t tx_coord_announce = {
		MAC_FC,			// mac.ctl - data frame, frame pending, pan id comp, short dest, short source
		0,				// mac.seq
		MAC_PAN_ID,		// mac.panid
		0xFFFF,			// mac.dest
		MAC_ANCHOR_ID,	// mac.source
		FUNC_COORD_ANNO,	// functionCode
		0x0000			// mac_cs
		};

/* Buffer to store received messages.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
static uint8 rx_buffer[CPH_MAX_MSG_SIZE];

/* Hold copy of status register state here for reference, so reader can examine it at a breakpoint. */
static uint32 status_reg = 0;

/* Timestamps of frames transmission/reception.
 * As they are 40-bit wide, we need to define a 64-bit int type to handle them. */
typedef unsigned long long uint64;
static uint64 poll_rx_ts;
static uint64 resp_tx_ts;

/* List of paired tags */
static cph_deca_pair_info_t paired_tags[MAX_TAGS];

/* Declaration of static functions. */
static uint64 get_rx_timestamp_u64(void);

static bool can_respond_to_discover(uint16_t shortid) {
	for (int i = 0; i < MAX_TAGS; i++) {
		if (paired_tags[i].shortid == shortid) {
			uint32_t elapsed = g_cph_millis - paired_tags[i].paired_ts;
			if (elapsed < PAIR_LIFETIME) {
				// Already paired - return no
				return false;
			} else {
				// Paired, but timed out - return yes
				return true;
			}
		}
	}
	// No pair found - return yes
	return true;
}

static bool update_paired_tags(uint16_t shortid) {
	int first_empty = -1;
	int i = 0;

	for (i = 0; i < MAX_TAGS; i++) {
		if (paired_tags[i].shortid == 0 && first_empty == -1)
			first_empty = i;
		if (paired_tags[i].shortid == shortid) {
			// Found tag, update timestamp and return success
			paired_tags[i].paired_ts = g_cph_millis;
			return true;
		}
	}

	// Not found, so try and add it
	if (first_empty == -1) {
		// No empty slot, return fail
		return false;
	}

	paired_tags[first_empty].shortid = shortid;
	paired_tags[first_empty].paired_ts = g_cph_millis;
	return true;

}

static void announce_coord(int repeat) {

	TRACE("announcing coordinator %04X", cph_coordid);
	if (cph_coordid == cph_config->shortid) {
		TRACE(" (me)");
	}
	TRACE("\r\n");

	// Write announce message to frame buffer
	tx_coord_announce.coordid = cph_coordid;
	cph_deca_load_frame(&tx_coord_announce.header, sizeof(tx_coord_announce));

	// Burst out our announcement
	for (int i = 0; i < repeat; i++) {
		cph_deca_send_immediate();
		deca_sleep(10);
	}
}

void anchor_run(void) {
	uint32_t announce_coord_ts = 0;
	uint32_t elapsed = 0;

	// Setup DW1000
	cph_deca_init_device();
	cph_deca_init_network(cph_config->panid, cph_config->shortid);

	// TESTTESTTESTTESTTEST
//	uint32_t reg = dwt_read32bitreg(SYS_CFG_ID);
//	reg |= SYS_CFG_DIS_PHE;
//	dwt_write32bitreg(SYS_CFG_ID, reg);


	// Init list of paired tags
	memset(paired_tags, 0, sizeof(cph_deca_pair_info_t) * MAX_TAGS);

	// Set our shortid in common messages
	tx_range_response_t.header.source = cph_config->shortid;
	tx_discover_reply.header.source = cph_config->shortid;
	tx_coord_announce.header.source = cph_config->shortid;

	// Announce ourselves if we're the coordinator
	if (cph_config->mode & CPH_MODE_COORD) {
		cph_coordid = cph_config->shortid;
//		cph_config->mode |= CPH_MODE_COORD;
		announce_coord(COORD_ANNOUNCE_START_BURST);
	}

	/* Loop forever responding to ranging requests. */
	while (1) {

		if (cph_coordid) {
			elapsed = cph_get_millis() - announce_coord_ts;
			if (elapsed > COORD_ANNOUNCE_INTERVAL) {
				announce_coord(1);
				announce_coord_ts = cph_get_millis();
			}
		}

//		status_reg = dwt_read32bitreg(SYS_STATUS_ID);
//		printf("status: %08X\r\n", status_reg);

		/* Activate reception immediately. */
		dwt_rxenable(0);

		status_reg = cph_deca_wait_for_rx_finished();

		if (status_reg & SYS_STATUS_RXFCG) {
			uint32 frame_len;
			cph_deca_msg_header_t * rx_header;

			rx_header = cph_deca_read_frame(rx_buffer, &frame_len);

			// Look for Poll message
			if (rx_header->functionCode == FUNC_RANGE_REQU) {
				uint32 resp_tx_time;

				/* Retrieve poll reception timestamp. */
				poll_rx_ts = get_rx_timestamp_u64();

				/* Compute final message transmission time. See NOTE 7 below. */
				resp_tx_time = (poll_rx_ts + (POLL_RX_TO_RESP_TX_DLY_UUS * UUS_TO_DWT_TIME)) >> 8;
				dwt_setdelayedtrxtime(resp_tx_time);

				/* Response TX timestamp is the transmission time we programmed plus the antenna delay. */
				resp_tx_ts = (((uint64) (resp_tx_time & 0xFFFFFFFE)) << 8) + TX_ANT_DLY;

				/* Write all timestamps in the final message. See NOTE 8 below. */
				tx_range_response_t.requestRxTs = poll_rx_ts;
				tx_range_response_t.responseTxTs = resp_tx_ts;

				/* Send the response message */
				tx_range_response_t.header.dest = rx_header->source;
				cph_deca_load_frame(&tx_range_response_t.header, sizeof(tx_range_response_t));
				cph_deca_send_delayed();

			} else if (rx_header->functionCode == FUNC_DISC_ANNO) {

				if (can_respond_to_discover(rx_header->source)) {
					/* Write and send the announce message. */
					tx_discover_reply.coordid = cph_coordid;
					tx_discover_reply.header.dest = rx_header->source;
					cph_deca_load_frame(&tx_discover_reply.header, sizeof(tx_discover_reply));
					cph_deca_send_immediate();
				} else {
					TRACE("ignoring pair with %04X\r\n", (rx_header->source));
				}

			} else if (rx_header->functionCode == FUNC_PAIR_RESP) {
				//TODO: Record the pairing details and check them when receiving a discover request
				//      For now, nothing to do
				if (update_paired_tags(rx_header->source)) {
					TRACE("paired with %04X\r\n", (rx_header->source));
				} else {
					TRACE("failed to pair with %04X\r\n", (rx_header->source));
				}

			} else if (rx_header->functionCode == FUNC_COORD_ANNO) {
				uint16_t id = ((cph_deca_msg_coord_announce_t*) rx_buffer)->coordid;
				if (id != cph_coordid) {
					cph_coordid = id;
					if (cph_coordid == cph_config->shortid) {
						TRACE("becoming coord\r\n");
						cph_config->mode |= CPH_MODE_COORD;
					} else {
						if (cph_config->mode & CPH_MODE_COORD) {
							TRACE("giving coord to %04X\r\n", cph_coordid);
						} else {
							TRACE("recognizing coord as %04X\r\n", cph_coordid);
						}
						cph_config->mode &= (~CPH_MODE_COORD);
					}
				}

			} else if (rx_header->functionCode == FUNC_RANGE_REPO) {
				cph_deca_msg_range_report_t * results = ((cph_deca_msg_range_report_t*) rx_buffer);
				TRACE("* %04X", rx_header->source);
				for (int i = 0; i < results->numranges; i++) {
					TRACE(" %04X:%3.2f", results->ranges[i].shortid, results->ranges[i].range);
				}
				TRACE("\r\n");

			} else {
				TRACE("ERROR: unknown function code - data: ");
				for (int i = 0; i < frame_len; i++)
					TRACE("%02X ", rx_buffer[i]);
				TRACE("\r\n");
			}
		} else {

			// Ignore frame rejections and timeouts
			uint32_t test = status_reg;// & (~(SYS_STATUS_AFFREJ | SYS_STATUS_RXRFTO));
			//if (test & SYS_STATUS_ALL_RX_ERR)
			{
				TRACE("ERROR: dwt_rxenable has status of %08X\r\n", status_reg);
				/* Clear RX error events in the DW1000 status register. */
				dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR | SYS_STATUS_CLKPLL_LL);

			}
		}
	}
}

/*! ------------------------------------------------------------------------------------------------------------------
 * @fn get_rx_timestamp_u64()
 *
 * @brief Get the RX time-stamp in a 64-bit variable.
 *        /!\ This function assumes that length of time-stamps is 40 bits, for both TX and RX!
 *
 * @param  none
 *
 * @return  64-bit value of the read time-stamp.
 */
static uint64 get_rx_timestamp_u64(void) {
	uint8 ts_tab[5];
	uint64 ts = 0;
	int i;
	dwt_readrxtimestamp(ts_tab);
	for (i = 4; i >= 0; i--) {
		ts <<= 8;
		ts |= ts_tab[i];
	}
	return ts;
}
