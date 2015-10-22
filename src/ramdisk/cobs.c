/*
 * cobs.c
 *
 *  Created on: Oct 14, 2015
 *      Author: jcobb
 */

#include "cobs.h"

void decode_cobs(const unsigned char *ptr, unsigned long length, unsigned char *dst)
{
	const unsigned char *end = ptr + length;
	while (ptr < end)
	{
		int i, code = *ptr++;
		for (i = 1; i < code; i++)
			*dst++ = *ptr++;
		if (code < 0xFF)
			*dst++ = 0;
	}
}

