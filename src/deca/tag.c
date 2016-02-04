/*
 *
 * tag.c
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

#define ANCHOR_ID		0x616A

/* Frames used in the ranging process.  */
static cph_deca_msg_range_request_t tx_poll_msg = {
MAC_FC,			// mac.ctl - data frame, frame pending, pan id comp, short dest, short source
		0,				// mac.seq
		MAC_PAN_ID,		// mac.panid
		MAC_ANCHOR_ID,	// mac.dest  	'A' 'W'
		MAC_TAG_ID,		// mac.source	'E' 'V'
		FUNC_RANGE_REQU,		// functionCode
		0x0000			// mac_cs
		};

static cph_deca_msg_discover_announce_t tx_discover_msg = {
MAC_FC,			// mac.ctl - data frame, frame pending, pan id comp, short dest, short source
		0,				// mac.seq
		MAC_PAN_ID,		// mac.panid
		0xFFFF,			// mac.dest
		MAC_TAG_ID,		// mac.source
		FUNC_DISC_ANNO,	// functionCode
		0x0000			// mac_cs
		};

static cph_deca_msg_pair_response_t tx_pair_msg = {
MAC_FC,			// mac.ctl - data frame, frame pending, pan id comp, short dest, short source
		0,				// mac.seq
		MAC_PAN_ID,		// mac.panid
		MAC_ANCHOR_ID,	// mac.dest
		MAC_TAG_ID,		// mac.source
		FUNC_PAIR_RESP,		// functionCode
		0x0000			// mac_cs
		};


static cph_deca_msg_range_report_t tx_range_results_msg = {
MAC_FC,			// mac.ctl - data frame, frame pending, pan id comp, short dest, short source
		0,				// mac.seq
		MAC_PAN_ID,		// mac.panid
		MAC_ANCHOR_ID,	// mac.dest
		MAC_TAG_ID,		// mac.source
		FUNC_RANGE_REPO,		// functionCode
		0,				// num ranges
		0,		// results
		0x0000			// mac_cs
		};


typedef unsigned long long uint64;

// Discovered anchors and their ranges
static cph_deca_anchor_range_t anchors[ANCHORS_MIN];
static unsigned int anchors_status;

/* Buffer to store received response message.
 * Its size is adjusted to longest frame that this example code is supposed to handle. */
static uint8 rx_buffer[CPH_MAX_MSG_SIZE];

/* Hold copy of status register state here for reference, so reader can examine it at a breakpoint. */
static uint32 status_reg = 0;

/* Hold copies of computed time of flight and distance here for reference, so reader can examine it at a breakpoint. */
static double tof;

static int range(cph_deca_anchor_range_t * range) {
	int result = CPH_OK;

	dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
	dwt_setrxtimeout(RESP_RX_TIMEOUT_UUS);

	// Setup POLL frame to request to range with anchor
	tx_poll_msg.header.dest = range->shortid;

	cph_deca_load_frame((cph_deca_msg_header_t*)&tx_poll_msg, sizeof(tx_poll_msg));
	status_reg = cph_deca_send_response_expected();

	if (status_reg & SYS_STATUS_RXFCG) {
		uint32 frame_len;
		cph_deca_msg_header_t * rx_header;

		// A frame has been received, read it into the local buffer.
		rx_header = cph_deca_read_frame(rx_buffer, &frame_len);
		if (rx_header) {
			// If valid response, calculate distance
			if (rx_header->functionCode == 0xE1) {
				uint32 poll_tx_ts, resp_rx_ts, poll_rx_ts, resp_tx_ts;
				int32 rtd_init, rtd_resp;

				// Retrieve poll transmission and response reception timestamps.
				poll_tx_ts = dwt_readtxtimestamplo32();
				resp_rx_ts = dwt_readrxtimestamplo32();

				// Get timestamps embedded in response message.
				poll_rx_ts = ((cph_deca_msg_range_response_t*) (rx_header))->requestRxTs;
				resp_tx_ts = ((cph_deca_msg_range_response_t*) (rx_header))->responseTxTs;

				// Compute time of flight and distance.
				rtd_init = resp_rx_ts - poll_tx_ts;
				rtd_resp = resp_tx_ts - poll_rx_ts;

				tof = ((rtd_init - rtd_resp) / 2.0) * DWT_TIME_UNITS;
				range->range = tof * SPEED_OF_LIGHT;

				range->range_avg -= (range->range_avg / RANGE_SAMPLES_AVG);
				range->range_avg += (range->range / RANGE_SAMPLES_AVG);
			} else {
				result = CPH_BAD_FRAME;
			}
		} else {
			result = CPH_BAD_LENGTH;
		}
	} else {
		// Clear RX error events in the DW1000 status register.
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
		result = CPH_ERROR;
	}

	return result;
}

static int discover(int idx) {
	int result = CPH_OK;

	dwt_setrxaftertxdelay(0);
	dwt_setrxtimeout(600);

	// Broadcast anchor discovery request
	cph_deca_load_frame((cph_deca_msg_header_t*)&tx_discover_msg, sizeof(tx_discover_msg));
	status_reg = cph_deca_send_response_expected();

	if (status_reg & SYS_STATUS_RXFCG) {
		uint32 frame_len;
		cph_deca_msg_header_t * rx_header;

		// A frame has been received, read it into the local buffer.
		rx_header = cph_deca_read_frame(rx_buffer, &frame_len);
		if (rx_header) {
			// If valid response, send the reply
			if (rx_header->functionCode == FUNC_DISC_REPLY) {

				uint16_t shortid = rx_header->source;

				// Now send the pair response back
				tx_pair_msg.header.dest = shortid;
				cph_deca_load_frame((cph_deca_msg_header_t*)&tx_pair_msg, sizeof(tx_pair_msg));
				cph_deca_send_immediate();

                // Grab the coordinator id
                if (((cph_deca_msg_discover_reply_t*) rx_buffer)->coordid != cph_coordid) {
                	cph_coordid = ((cph_deca_msg_discover_reply_t*) rx_buffer)->coordid;
                	printf("coordinator discovered at %04X\r\n", cph_coordid);
                }

                // Check for duplicate
				for (int i=0;i<ANCHORS_MIN;i++) {
					if (anchors[i].shortid == shortid) {
		                printf("shortid %04X already exists in anchors[%d]\r\n", shortid, i);
		                result = CPH_DUPLICATE;
		                break;
					}
				}

				// Not a duplicate so store the shortid
				if (result == CPH_OK) {
					anchors[idx].shortid = shortid;
					anchors[idx].range = 0;
				}

			} else {
				result = CPH_BAD_FRAME;
			}
		} else {
			result = CPH_BAD_LENGTH;
		}
	} else {

		//status_reg = dwt_read32bitreg(SYS_STATUS_ID);
		uint32_t rf_status = dwt_read32bitoffsetreg(RF_CONF_ID, RF_STATUS_OFFSET);
		printf("discover: status_reg %08X\trf_status %08X\r\n", status_reg, rf_status);

		// Clear RX error events in the DW1000 status register.
		dwt_write32bitreg(SYS_STATUS_ID, SYS_STATUS_ALL_RX_ERR);
		result = CPH_ERROR;
	}

	return result;
}

void init_anchors(void) {
	// Init anchors table
	for (int i = 0; i < ANCHORS_MIN; i++) {
		anchors[i].shortid = 0;
		anchors[i].range = 0;
	}
	anchors_status = ANCHORS_MASK;
}

void refresh_anchors(void) {

	init_anchors();

	// Discover anchors
	uint32_t anchor_refresh_ts = 0;
	while (anchors_status) {
		printf("Discovering anchors .. anchors_status:%02X\r\n", anchors_status);

		// Check for refresh of anchors
		uint32_t elapsed = cph_get_millis() - anchor_refresh_ts;
		if (elapsed > ANCHORS_REFRESH_INTERVAL) {
			printf("Anchors discovery timeout.  anchors_status:%02X\r\n", anchors_status);
			init_anchors();
			anchor_refresh_ts = cph_get_millis();
		}

		for (int i=0;i<ANCHORS_MIN;i++) {
			if (anchors_status & (1 << i)) {
				int result = discover(i);
				if (result == CPH_OK) {
					anchors_status &= (~(1 << i));
					printf("anchor[%d] %04X\r\n", i, anchors[i].shortid);
				}
				deca_sleep(RNG_DELAY_MS);
			}
		}
		deca_sleep(POLL_DELAY_MS);
	}

	printf("Anchors discovered. Moving to poll.  anchors_status:%02X\r\n", anchors_status);
}

static void send_ranges(int tries) {
	printf("%d\t%04X\t", tries,cph_coordid);
	for (int i = 0; i < ANCHORS_MIN; i++) {
		printf("%04X: %3.2f m (%3.2f m)\t", anchors[i].shortid, anchors[i].range, anchors[i].range_avg);
	}
	printf("\r\n");

	if (cph_coordid) {
		// Now send the results
		tx_range_results_msg.header.dest = cph_coordid;
		tx_range_results_msg.numranges = ANCHORS_MIN;
		memcpy(&tx_range_results_msg.ranges[0], &anchors[0], sizeof(cph_deca_anchor_range_t) * ANCHORS_MIN);

		cph_deca_load_frame((cph_deca_msg_header_t*)&tx_range_results_msg, sizeof(tx_range_results_msg));
		cph_deca_send_immediate();
	}
}


void tag_run(void) {

	// Setup DECAWAVE
	cph_deca_init_device();
	cph_deca_init_network(cph_config->panid, cph_config->shortid);

	// Set our short id in common messages
	tx_poll_msg.header.source = cph_config->shortid;
	tx_discover_msg.header.source = cph_config->shortid;
	tx_pair_msg.header.source = cph_config->shortid;
	tx_range_results_msg.header.source = cph_config->shortid;

	// First, discover anchors
	uint32_t anchor_refresh_ts = 0;
	refresh_anchors();
	anchor_refresh_ts = cph_get_millis();

	// Poll loop
	while (1) {

		int ranges_countdown = MAX_RANGES_BEFORE_POLL_TIMEOUT;
		anchors_status = ANCHORS_MASK;

		while (anchors_status && (--ranges_countdown)) {

			// Check for refresh of anchors
			uint32_t elapsed = cph_get_millis() - anchor_refresh_ts;
			if (elapsed > ANCHORS_REFRESH_INTERVAL) {
				printf("Anchors refresh timeout.  anchors_status:%02X\r\n", anchors_status);
				refresh_anchors();
				anchor_refresh_ts = cph_get_millis();
				// Since we refreshed the anchors, need to range again for ALL anchors during this poll
				anchors_status = ANCHORS_MASK;
			}

			// Range each anchor once during this poll
			for (int i = 0; i < ANCHORS_MIN; i++) {
				if (anchors_status & (1 << i)) {
					anchors[i].range = 0;
					int result = range(&anchors[i]);

					if (result == CPH_OK) {
						anchors_status &= (~(1 << i));
					}

					deca_sleep(RNG_DELAY_MS);
				}
			}

			deca_sleep(RNG_DELAY_MS);
		}

		if (ranges_countdown) {
			send_ranges(MAX_RANGES_BEFORE_POLL_TIMEOUT - ranges_countdown);
		}
		else {
			printf("ranges_countdown expired!\r\n");
		}

		// Execute a delay between ranging exchanges.
		deca_sleep(POLL_DELAY_MS);
	}
}
