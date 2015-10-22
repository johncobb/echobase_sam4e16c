/*
 * usart.h
 *
 *  Created on: Oct 16, 2015
 *      Author: jcobb
 */

#ifndef USART_H_
#define USART_H_

#define USART_RX_BUFFER_SIZE 512

typedef struct
{
	unsigned char buffer[USART_RX_BUFFER_SIZE];
	int head;
	int tail;
} usart_ring_buffer_t;


void usart_put_char(usart_ring_buffer_t *usart_buffer, uint8_t c);
uint8_t usart_clear_buffer(usart_ring_buffer_t *usart_buffer);
uint8_t usart_data_available(usart_ring_buffer_t *usart_buffer);
uint8_t usart_data_read(usart_ring_buffer_t *usart_buffer);


#endif /* USART_H_ */
