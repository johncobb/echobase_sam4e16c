/*
 * cobs.h
 *
 *  Created on: Oct 14, 2015
 *      Author: jcobb
 */

#ifndef COBS_H_
#define COBS_H_

void decode_cobs(const unsigned char *ptr, unsigned long length, unsigned char *dst);

#endif /* COBS_H_ */
