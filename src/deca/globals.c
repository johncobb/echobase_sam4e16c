/*
 * globals.c
 *
 *  Created on: Jan 27, 2016
 *      Author: ericrudisill
 */

#include <cph.h>
#include <deca_device_api.h>


dwt_config_t g_dwt_configs[] = {
	{
		2,				 	/* Channel number. */
		DWT_PRF_64M,	 	/* Pulse repetition frequency. */
		DWT_PLEN_1024, 		/* Preamble length. */
		DWT_PAC32, 			/* Preamble acquisition chunk size. Used in RX only. */
		9, 					/* TX preamble code. Used in TX only. */
		9, 					/* RX preamble code. Used in RX only. */
		1, 					/* Use non-standard SFD (Boolean) */
		DWT_BR_110K, 		/* Data rate. */
		DWT_PHRMODE_STD, 	/* PHY header mode. */
		(1025 + 64 - 32) 	/* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */
	},
	{																													\
		2,               /* Channel number. */																			\
		DWT_PRF_64M,     /* Pulse repetition frequency. */																\
		DWT_PLEN_128,    /* Preamble length. */																			\
		DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */										\
		9,               /* TX preamble code. Used in TX only. */														\
		9,               /* RX preamble code. Used in RX only. */														\
		0,               /* Use non-standard SFD (Boolean) */															\
		DWT_BR_6M8,      /* Data rate. */																				\
		DWT_PHRMODE_STD, /* PHY header mode. */																			\
		(129 + 8 - 8)    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */				\
	},
	{																													\
		2,               /* Channel number. */																			\
		DWT_PRF_64M,     /* Pulse repetition frequency. */																\
		DWT_PLEN_128,    /* Preamble length. */																			\
		DWT_PAC8,        /* Preamble acquisition chunk size. Used in RX only. */										\
		9,               /* TX preamble code. Used in TX only. */														\
		9,               /* RX preamble code. Used in RX only. */														\
		0,               /* Use non-standard SFD (Boolean) */															\
		DWT_BR_6M8,      /* Data rate. */																				\
		DWT_PHRMODE_STD, /* PHY header mode. */																			\
		(129 + 8 - 8)    /* SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only. */				\
	},
};


//int g_cph_mode = CPH_MODE_ANCHOR;

//uint32_t g_sender_period_ms = 250;
