/*
 * configure.c
 *
 *  Created on: Jan 27, 2016
 *      Author: ericrudisill
 */

#include <string.h>
#include <cph.h>
#include <globals.h>

#define BUFFER_SIZE 80
static char choice = 0;
static int config_idx = -1;
static char buffer[BUFFER_SIZE];


#define PLEN_COUNT 8
static int plen_values[] = {
	DWT_PLEN_4096,
	DWT_PLEN_2048,
	DWT_PLEN_1536,
	DWT_PLEN_1024,
	DWT_PLEN_512,
	DWT_PLEN_256,
	DWT_PLEN_128,
	DWT_PLEN_64
};
static int plen_inputs[] = {
	4096,
	2048,
	1536,
	1024,
	512,
	256,
	128,
	64
};


#define PAC_COUNT 4
static int pac_values[] = {
		DWT_PAC8,
		DWT_PAC16,
		DWT_PAC32,
		DWT_PAC64
};
static int pac_inputs[] = {
	8,
	16,
	32,
	64
};

static bool get_line(uint8_t * buf, int max_len) {
	int i = 0;
	char c;
	memset(buf, 0, max_len);

	while ((c = getchar() & 0xFF) != '\r') {
		TRACE("%c", c);
		buf[i++] = c;
		if (i == max_len) {
			TRACE("--OVERRUN--\r\n");
			return false;
			break;
		}
	}

	TRACE("\r\n");
	return true;
}

void configure_print_dwt_config(dwt_config_t * source) {
	int i;

	TRACE("chan:%02X  ", source->chan);

	if (source->prf == DWT_PRF_16M)
		TRACE("PRF_16M  ");
	else if (source->prf == DWT_PRF_64M) {
		TRACE("PRF_64M  ");
	}
	else {
		TRACE("PRF_??  ");
	}

	for (i=0;i<PLEN_COUNT;i++) {
		if (source->txPreambLength == plen_values[i]) {
			TRACE("PLEN_%-4d  ", plen_inputs[i]);
			break;
		}
	}
	if (i==PLEN_COUNT)
		TRACE("PLEN_??  ");

	for (i=0;i<PAC_COUNT;i++) {
		if (source->rxPAC == pac_values[i]) {
			TRACE("PAC_%-2d  ", pac_inputs[i]);
			break;
		}
	}
	if (i==PAC_COUNT)
		TRACE("PAC_??  ");

	TRACE("txCode:%02X  rxCode:%02X  nsSFD:%02X  ", source->txCode, source->rxCode, source->nsSFD);


	if (source->dataRate == DWT_BR_110K) {
		TRACE("BR_110K  ");
	}
	else if (source->dataRate == DWT_BR_850K) {
		TRACE("BR_850K  ");
	}
	else if (source->dataRate == DWT_BR_6M8) {
		TRACE("BR_6M8   ");
	}
	else {
		TRACE("BR_??  ");
	}

	if (source->phrMode == DWT_PHRMODE_STD) {
		TRACE("PHRMODE_STD  ");
	}
	else if (source->phrMode == DWT_PHRMODE_EXT) {
		TRACE("PHRMODE_EXT  ");
	}
	else {
		TRACE("PHRMODE_??");
	}

	TRACE("sfdTO:%d", source->sfdTO);
}

static bool parse_config_tuples(char * tuples, dwt_config_t * target) {
	int ch, prf, plen, pac, txcode, rxcode, nssfd, datarate, phrmode, sfdto;
	int j = 0;
	bool valid = true;

	if (sscanf(tuples, "%d %d %d %d %d %d %d %d %d %d", &ch, &prf, &plen, &pac, &txcode, &rxcode, &nssfd, &datarate, &phrmode, &sfdto) != 10) {
		TRACE("BAD INPUT\r\n");
		valid = false;
	}
	else {
		target->chan = ch;
		target->txCode = txcode;
		target->rxCode = rxcode;
		target->nsSFD = nssfd;
		target->sfdTO = sfdto;


		// Parse PRF
		//
		if (prf == 64) {
			target->prf = DWT_PRF_64M;
		}
		else if (prf == 16) {
			target->prf = DWT_PRF_16M;
		}
		else {
			TRACE("BAD PRF: 16 or 64\r\n");
			valid = false;
		}

		// Parse PLEN
		//
		for (j=0;j<PLEN_COUNT;j++) {
			if (plen == plen_inputs[j]) {
				target->txPreambLength = plen_values[j];
				break;
			}
		}
		if (j == PLEN_COUNT) {
			TRACE("BAD PLEN\r\n");
			valid = false;
		}


		// Parse PAC
		//
		for (j=0;j<PAC_COUNT;j++) {
			if (pac == pac_inputs[j]) {
				target->rxPAC = pac_values[j];
				break;
			}
		}
		if (j == PAC_COUNT) {
			TRACE("BAD PAC\r\n");
			valid = false;
		}


		// Parse Data Rate
		//
		if (datarate == 110) {
			target->dataRate = DWT_BR_110K;
		}
		else if (datarate == 850) {
			target->dataRate = DWT_BR_850K;
		}
		else if (datarate == 6) {
			target->dataRate = DWT_BR_6M8;
		}
		else {
			TRACE("BAD DATA RATE: 110, 850, or 6\r\n");
			valid = false;
		}


		// Parse PHRMODE
		if (phrmode == 0) {
			target->phrMode = DWT_PHRMODE_STD;
		}
		else if (phrmode == 1) {
			target->phrMode = DWT_PHRMODE_EXT;
		}
		else {
			TRACE("BAD PHRMODE: 0 for STD, 1 for EXT\r\n");
			valid = false;
		}
	}

	return valid;
}

static bool configure_user_defined(void) {

	int ch, prf, plen, pac, txcode, rxcode, nssfd, datarate, phrmode, sfdto;
	int j = 0;
	bool valid = false;

	while (!valid) {
		TRACE("\r\n\r\nUser Defined\r\n");
		TRACE("===============\r\n");
		TRACE("ENTER x to exit to Main Menu\r\n");
		TRACE("NOTE: Use 6 for 6M8, 0 for STD PHRMODE, 1 for EXT PHRMODE\r\n");
		TRACE("FORMAT: ch prf plen pac txcode rxcode nssfd datarate phrmode sfdto\r\n");
		TRACE("\r\n> ");

		if (get_line(buffer, BUFFER_SIZE) == false)
			continue;

		if (buffer[0] == 'x') {
			TRACE("Exiting to Main Menu. No changes made.\r\n");
			return false;
		}

		valid = parse_config_tuples(buffer, &g_dwt_configs[G_CONFIG_USER_IDX]);
		if (valid)
			config_idx = G_CONFIG_USER_IDX;
	}

	return true;
}

static bool configure_parameters(void) {

	while (true) {
		TRACE("\r\n\rParameters\r\n");
		TRACE("===============\r\n");
		TRACE("ENTER x to exit to Main Menu\r\n");
		TRACE("0) g_sender_period_ms %d\r\n", cph_config->sender_period);
		TRACE("1) pan_id 0x%02X\r\n", cph_config->panid);
		TRACE("2) short_id 0x%02X\r\n", cph_config->shortid);
		TRACE("\r\n> ");

		choice = getchar() & 0xFF;
		TRACE("%c\r\n", choice);

		if (choice == 'x') {
			TRACE("Exiting to Main Menu. No changes made.\r\n");
			return false;
		}

		if (choice < '0' || choice > '2') {
			TRACE("INVALID CHOICE\r\n");
			continue;
		}

		TRACE("\r\nEnter New Value: ");

		if (get_line(buffer, BUFFER_SIZE) == false)
			continue;

		bool valid = true;
		uint32_t hex = 0;

		if (choice == '0') {
			if (sscanf(buffer, "%d", &cph_config->sender_period) != 1) {
				valid = false;
			}
		} else if (choice == '1') {
			if (sscanf(buffer, "%x", &hex) == 1) {
				cph_config->panid = 0xffff & hex;
			} else {
				valid = false;
			}
		} else if (choice == '2') {
			if (sscanf(buffer, "%x", &hex) == 1) {
				cph_config->shortid = 0xffff & hex;
			} else {
				valid = false;
			}
		}

		if (valid == false) {
			TRACE("BAD INPUT\r\n");
		}
		else {
			TRACE("CHANGE ACCEPTED\r\n");
		}
	}

	return true;
}

static bool test_flash(void) {

	cph_config_t * config;
	uint8_t test = 0x01;

	while (true) {
		TRACE("\r\nTest Flash\r\n");
		TRACE("=================\r\n");
		TRACE("I) Init\r\n");
		TRACE("R) Read\r\n");
		TRACE("W) Write\r\n");
		TRACE("X) Exit to menu\r\n");
		TRACE("\r\n> ");

		choice = getchar() & 0xFF;
		TRACE("%c\r\n", choice);

		if (choice == 'x' || choice == 'X') {
			TRACE("Exiting to Main Menu. No changes made.\r\n");
			return false;
		}

		if (choice == 'i' || choice == 'I') {
			config = cph_config_init();
			printf("config: ");
			for (int i=0;i<sizeof(cph_config_t);i++)
				printf("%02X ", ((uint8_t*)config)[i]);
			printf("\r\n");
		}
		else if (choice == 'r' || choice == 'R') {
			config = cph_config_read();
			printf("config: ");
			for (int i=0;i<sizeof(cph_config_t);i++)
				printf("%02X ", ((uint8_t*)config)[i]);
			printf("\r\n");
		}
		else if (choice == 'w' || choice == 'W') {
			config->magic[0] = test++;
			config->magic[1] = test++;
			config->magic[2] = test++;
			config->magic[3] = test++;
			config->hw_major = test++;
			config->hw_minor = test++;
			config->fw_major = test++;
			config->fw_minor = test++;
			config->panid = test++;
			config->shortid = test++;
			cph_config_write();
			printf("config: ");
			for (int i=0;i<sizeof(cph_config_t);i++)
				printf("%02X ", ((uint8_t*)config)[i]);
			printf("\r\n");
		}
		else {
			TRACE("INVALID CHOICE\r\n");
		}

	}

	return true;
}

void configure_main(void) {

	uint8_t buffer[10];

	config_idx = -1;

	cph_config = cph_config_init();

	while (1) {
		TRACE("\r\n\r\nMain Menu\r\n");
		TRACE("==============\r\n");

		for (int i=0;i<G_CONFIG_COUNT - 1;i++) {
			TRACE("%d) ", i);
			configure_print_dwt_config(&g_dwt_configs[i]);
			TRACE("\r\n");
		}

		TRACE("U) User defined\r\n");
		TRACE("P) Set parameters\r\n");
		TRACE("A) Exit and run as ANCHOR\r\n");
		TRACE("C) Exit and run as COORDINATOR\r\n");
		TRACE("T) Exit and run as TAG\r\n");
		TRACE("L) Exit and run as LISTENER\r\n");
		TRACE("S) Exit and run as SENDER\r\n");
		TRACE("D) Exit and run with compiled DEFAULTS\r\n");
		TRACE("F) Test flash\r\n");

		TRACE("\r\nCurrent Config : ");
		configure_print_dwt_config(&cph_config->dwt_config);
		TRACE("\r\nNew Config     : ");
		if (config_idx == -1) {
			TRACE("none selected");
		}
		else {
			configure_print_dwt_config(&g_dwt_configs[config_idx]);
		}
		TRACE("\r\n");

		TRACE("\r\n> ");

		choice = getchar() & 0xFF;
		TRACE("%c - ", choice);

		if (choice >= '0' && choice <= ('0' + G_CONFIG_COUNT - 1 - 1)) {
			TRACE("VALID\r\n");
			config_idx = choice - '0';
		}
		else if (choice == 'u' || choice == 'U') {
			configure_user_defined();
		}
		else if (choice == 'p' || choice == 'P') {
			configure_parameters();
		}
		else if (choice == 'a' || choice == 'A') {
			cph_config->mode = CPH_MODE_ANCHOR;
			TRACE("Exiting as CPH_MODE_ANCHOR\r\n");
			break;
		}
		else if (choice == 'c' || choice == 'C') {
			cph_config->mode = CPH_MODE_COORD;
			TRACE("Exiting as CPH_MODE_COORD\r\n");
			break;
		}
		else if (choice == 't' || choice == 'T') {
			cph_config->mode = CPH_MODE_TAG;
			TRACE("Exiting as CPH_MODE_TAG\r\n");
			break;
		}
		else if (choice == 'l' || choice == 'L') {
			cph_config->mode = CPH_MODE_LISTENER;
			TRACE("Exiting as CPH_MODE_LISTENER\r\n");
			break;
		}
		else if (choice == 's' || choice == 'S') {
			cph_config->mode = CPH_MODE_SENDER;
			TRACE("Exiting as CPH_MODE_SENDER\r\n");
			break;
		}
		else if (choice == 'd' || choice == 'D') {
			memset(cph_config, 0, sizeof(cph_config_t));
			cph_config_write();
			TRACE("Exiting as compiled DEFAULT\r\n");
			break;
		}
		else if (choice == 'f' || choice == 'F') {
			test_flash();
		}
		else {
			TRACE("NOT VALID\r\n");
		}
	};

	TRACE("\r\n");

	TRACE("Writing config...");
	if (config_idx >= 0) {
		memcpy(&cph_config->dwt_config, &g_dwt_configs[config_idx], sizeof(dwt_config_t));
	}
	cph_config_write();
	TRACE("done.\r\n");

	TRACE("Starting in ");
	for (int i=3;i>0;i--) {
		TRACE("%d ", i);
		cph_millis_delay(1000);
	}
	TRACE("\r\n");
}
