/*
 * usart.c
 *
 *  Created on: Oct 16, 2015
 *      Author: jcobb
 */

#include <stdint.h>
#include "ring_buffer.h"



void usart_put_char(usart_ring_buffer_t *usart_buffer, uint8_t c)
{
	int i = (unsigned int)(usart_buffer->head + 1) % USART_RX_BUFFER_SIZE;

	// if we should be storing the received character into the location
	// just before the tail (meaning that the head would advance to the
	// current location of the tail), we're about to overflow the buffer
	// and so we don't write the character or advance the head.
	if (i != usart_buffer->tail) {
		usart_buffer->buffer[usart_buffer->head] = c;
		usart_buffer->head = i;
	}
}

uint8_t usart_clear_buffer(usart_ring_buffer_t *usart_buffer)
{
	return (uint8_t)(USART_RX_BUFFER_SIZE + usart_buffer->head - usart_buffer->tail) % USART_RX_BUFFER_SIZE;
}

uint8_t usart_data_available(usart_ring_buffer_t *usart_buffer)
{
	return (uint8_t)(USART_RX_BUFFER_SIZE + usart_buffer->head - usart_buffer->tail) % USART_RX_BUFFER_SIZE;
}

uint8_t usart_data_read(usart_ring_buffer_t *usart_buffer)
{
	// if the head isn't ahead of the tail, we don't have any characters
	if (usart_buffer->head == usart_buffer->tail) {
		return -1;
	} else {
		uint8_t c = usart_buffer->buffer[usart_buffer->tail];
		usart_buffer->tail = (unsigned int)(usart_buffer->tail + 1) % USART_RX_BUFFER_SIZE;
		return c;
	}
}

