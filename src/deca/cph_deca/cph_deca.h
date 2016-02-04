/*
 * cph_deca.h
 *
 *  Created on: Jan 11, 2016
 *      Author: ericrudisill
 */

#ifndef SRC_CPH_DECA_CPH_DECA_H_
#define SRC_CPH_DECA_CPH_DECA_H_

#include <cph_deca_port.h>
#include <deca_regs.h>
#include <deca_device_api.h>

// 								 'C''P'
#define MAC_PAN_ID				0x4350
#define MAC_ANCHOR_ID			0x4157
#define MAC_TAG_ID				0x4556
#define MAC_FC					0x8841
#define MAC_FC_ACK				0x8861
#define MAC_SHORT				0x1234


void anchor_run(void);
void listener_run(void);
void tag_run(void);
void sender_run(void);


/* Inter-ranging delay period, in milliseconds. */
#define RNG_DELAY_MS 	5

/* Inter-poll delay period, in milliseconds */
#define POLL_DELAY_MS 	200

/* Default antenna delay values for 64 MHz PRF. See NOTE 2 below. */
//#define TX_ANT_DLY 16436
//#define RX_ANT_DLY 16436

////////////ADJUSTED
//#define TX_ANT_DLY 16486
//#define RX_ANT_DLY 16486

#define TX_ANT_DLY 0
#define RX_ANT_DLY (16486 * 2)


/* UWB microsecond (uus) to device time unit (dtu, around 15.65 ps) conversion factor.
 * 1 uus = 512 / 499.2 µs and 1 µs = 499.2 * 128 dtu. */
#define UUS_TO_DWT_TIME 65536


///* Delay between frames, in UWB microseconds.  For TAG */
//#define POLL_TX_TO_RESP_RX_DLY_UUS 330
///* Receive response timeout. See NOTE 5 below. */
//#define RESP_RX_TIMEOUT_UUS 370

/* Delay between frames, in UWB microseconds.  For TAG */
#define POLL_TX_TO_RESP_RX_DLY_UUS 100
/* Receive response timeout. See NOTE 5 below. */
#define RESP_RX_TIMEOUT_UUS 900



/* Delay between frames, in UWB microseconds.  For ANCHOR */
//#define POLL_RX_TO_RESP_TX_DLY_UUS 660
#define POLL_RX_TO_RESP_TX_DLY_UUS 550


/* Speed of light in air, in metres per second. */
#define SPEED_OF_LIGHT 299702547


// Min Number of anchors to range with - if this changes, so should ANCHORS_MASK
//#define ANCHORS_MIN		3
#define ANCHORS_MIN		1

// Used for tracking status of anchor ids (by bitmask) during discovery and poll
//#define ANCHORS_MASK	0x07
#define ANCHORS_MASK	0x01

// Anchor refresh interval
#define ANCHORS_REFRESH_INTERVAL	10000

// Coord announce startup burst repeat count
#define COORD_ANNOUNCE_START_BURST	10

// Coord announce period in ms
#define COORD_ANNOUNCE_INTERVAL		7000

// Max ranges before poll timeout - keeps from blasting radio when an anchor is not responding
#define MAX_RANGES_BEFORE_POLL_TIMEOUT	5

// Max number of tags to pair with
#define MAX_TAGS		32

// Lifetime of tag pairing
#define PAIR_LIFETIME	5000

// Number of samples to average over
#define RANGE_SAMPLES_AVG	8.0

// Delay to start listening after discover
#define DISCOVER_TX_TO_ANNOUNCE_RX_DELAY_UUS	400
#define DISCOVER_RX_TO_ANNOUNCE_TX_DELAY_UUS	460

enum {
	CPH_OK = 0,
	CPH_ERROR,
	CPH_BAD_FRAME,
	CPH_BAD_LENGTH,
	CPH_DUPLICATE
};

enum {
	CPH_MODE_ANCHOR = 0x01,
	CPH_MODE_TAG = 0x02,
	CPH_MODE_LISTENER = 0x03,
	CPH_MODE_SENDER = 0x04,
	CPH_MODE_COORD = 0x81
};

#define	FUNC_RANGE_REQU				0xE0
#define FUNC_RANGE_RESP				0xE1
#define FUNC_DISC_ANNO				0xE2
#define FUNC_DISC_REPLY				0xE3
#define FUNC_PAIR_RESP				0xE4
#define FUNC_COORD_ANNO				0xE5
#define FUNC_RANGE_REPO				0xE6

#define CPH_MAX_MSG_SIZE		128

#define PACKED	__attribute__((packed))

typedef struct PACKED {
	uint16_t ctl;
	uint8_t seq;
	uint16_t panid;
	uint16_t dest;
	uint16_t source;
	uint8_t functionCode;
} cph_deca_msg_header_t;

typedef struct PACKED {
	cph_deca_msg_header_t header;
	uint16_t mac_cs;
} cph_deca_msg_range_request_t;

typedef struct PACKED {
	cph_deca_msg_header_t header;
	uint32_t requestRxTs;
	uint32_t responseTxTs;
	uint16_t mac_cs;
} cph_deca_msg_range_response_t;

typedef struct PACKED {
	cph_deca_msg_header_t header;
	uint16_t mac_cs;
} cph_deca_msg_discover_announce_t;

typedef struct PACKED {
	cph_deca_msg_header_t header;
	uint16_t coordid;
	uint16_t mac_cs;
} cph_deca_msg_discover_reply_t;

typedef struct PACKED {
	cph_deca_msg_header_t header;
	uint16_t mac_cs;
} cph_deca_msg_pair_response_t;

typedef struct PACKED {
	cph_deca_msg_header_t header;
	uint16_t coordid;
	uint16_t mac_cs;
} cph_deca_msg_coord_announce_t;

typedef struct PACKED {
	uint16_t shortid;
	double range;
	double range_avg;
} cph_deca_anchor_range_t;

typedef struct PACKED {
	cph_deca_msg_header_t header;
	uint8_t numranges;
	cph_deca_anchor_range_t ranges[ANCHORS_MIN];		//TODO: Make this dynamic
	uint16_t mac_fs;
} cph_deca_msg_range_report_t;

typedef struct PACKED {
	uint16_t shortid;
	uint32_t paired_ts;
} cph_deca_pair_info_t;



inline uint32_t cph_deca_wait_for_tx_finished(void) {
	uint32_t status_reg;
	while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & SYS_STATUS_TXFRS)) {
	};
	return status_reg;
}

inline uint32_t cph_deca_wait_for_rx_finished(void) {
	uint32_t status_reg;
	while (!((status_reg = dwt_read32bitreg(SYS_STATUS_ID)) & (SYS_STATUS_RXFCG | SYS_STATUS_ALL_RX_ERR))) {
	};
	return status_reg;
}


void cph_deca_load_frame(cph_deca_msg_header_t * hdr, uint16_t size);
cph_deca_msg_header_t * cph_deca_read_frame(uint8_t * rx_buffer, uint32_t *frame_len);
uint32_t cph_deca_send_immediate();
uint32_t cph_deca_send_delayed();
uint32_t cph_deca_send_response_expected();
void cph_deca_init_device();
void cph_deca_init_network(uint16_t panid, uint16_t shortid);
#endif /* SRC_CPH_DECA_CPH_DECA_H_ */
