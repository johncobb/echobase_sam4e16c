/*
 * comm.c
 *
 *  Created on: Jul 8, 2015
 *      Author: jcobb
 */



#include <stdint.h>
#include <string.h>
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "freertos_usart_serial.h"
#include "sysclk.h"
#include "modem_defs.h"
#include "modem.h"
#include "comm.h"
#include "socket.h"


void unit_test_commidle(void);
void unit_test_guardtime(void);

QueueHandle_t xCommQueue;
QueueHandle_t xCommQueueRequest;

static uint8_t out_buffer[COMM_BUFFER_LEN+1];

comm_state_t comm_state = COMM_INIT;
comm_network_state_t comm_network_state = {0,0};

static volatile uint8_t comm_dispatch_sig = 0;

uint32_t bytes_received = 0;

volatile bool comm_ready = false;

void handle_modem_events(void);

modem_socket_t modem_sockets[] =
{
		// socket_id, cnx_id, ctx_id, pkt_size, glb_timeout, cnx_timeout (tenths of second), tx_timeout (tenths of second)
		// socket_status, endpoint
		// protocol, address, port
		// function_handler, data_buffer, bytes_received
		{0, 1, 1, 512, 90, 600, 2, 0, {SOCKET_TCP, SOCKET_PROT_HTTP, 1888}, {0}, {0}, {0},  0, comm_idle, {0,0,0,0}, 0, 0},
//		{0, 1, 1, 512, 90, 600, 2, 0, {SOCKET_TCP, SOCKET_PROT_HTTP, 1889}, {0}, {0}, http_handle_data, 0, comm_idle, {0,0,0,0}, 0, 0},
		{1, 2, 1, 512, 90, 600, 2, 0, {SOCKET_TCP, SOCKET_PROT_HTTP, 1889}, {0}, {0}, {0},  0, comm_idle, {0,0,0,0}, 0, 0},
		{0, 0, 0, 0, 0}
};


//static modem_socket_t *socket;


static void comm_handler_task(void *pvParameters);

extern void comm_signal_handler(uint8_t sig)
{
	comm_dispatch_sig = 1;
}


void create_comm_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority)
{
	memset(out_buffer, '\0', COMM_BUFFER_LEN+1);

	// semaphores to be processed from CLI (command line interpreter)
	vSemaphoreCreateBinary(comm_signal);
	vSemaphoreCreateBinary(tcp_connect_signal);
	vSemaphoreCreateBinary(tcp_send_signal);
	vSemaphoreCreateBinary(tcp_receive_signal);
	vSemaphoreCreateBinary(tcp_suspend_signal);
	vSemaphoreCreateBinary(tcp_close_signal);

	xCommQueue = xQueueCreate(10, sizeof(comm_frame_t));

	xCommQueueRequest = xQueueCreate(10, sizeof(comm_request_t));

	xTaskCreate(	comm_handler_task,			/* The task that implements the command console. */
					(const int8_t *const) "COMM_D",	/* Text name assigned to the task.  This is just to assist debugging.  The kernel does not use this name itself. */
					stack_depth_words,					/* The size of the stack allocated to the task. */
					NULL,			/* The parameter is used to pass the already configured USART port into the task. */
					task_priority,						/* The priority allocated to the task. */
					NULL);

}

void comm_enterstate(modem_socket_t *socket, comm_state_t state)
{
	comm_state = state;

	// we can set a state while passing in a null socket
	if(socket == NULL) {
		return;
	}

	// TODO: REVIEW NEW CODE TO RESET STATE on comm_enterstate
	socket->state_handle.state = 0;
	socket->state_handle.substate = 0;

	switch(state) {
		case COMM_IDLE:
#ifdef LOG_COMMSTATECHANGE
			printf("comm_enterstate: comm_idle\r\n");
#endif
			socket->task_handler = comm_idle;
			break;
		case COMM_CONNECT:
#ifdef LOG_COMMSTATECHANGE
			printf("comm_enterstate: comm_connect\r\n");
#endif
			socket->task_handler = comm_connect;
			break;
		case COMM_SEND:
#ifdef LOG_COMMSTATECHANGE
			printf("comm_enterstate: comm_send\r\n");
#endif
			socket->task_handler = comm_send;
			break;
		case COMM_RECEIVE:
#ifdef LOG_COMMSTATECHANGE
			printf("comm_enterstate: comm_receive\r\n");
#endif
			socket->task_handler = comm_receive;
			break;
		case COMM_SUSPEND:
#ifdef LOG_COMMSTATECHANGE
			printf("comm_enterstate: comm_suspend\r\n");
#endif
			socket->task_handler = comm_suspend;
			break;
		case COMM_CLOSE:
#ifdef LOG_COMMSTATECHANGE
			printf("comm_enterstate: comm_close\r\n");
#endif
			socket->task_handler = comm_close;
			break;
	}
}

void comm_set_state(comm_state_t state)
{
	comm_state = state;
}

uint8_t socket_index = 0;
static BaseType_t result;

uint8_t comm_buffer[COMM_BUFFER_LEN+1] = {0};
comm_task_t task_mgr;

static bool modem_ready = false;

static void request_queue(void);
static void response_queue(void);
static void next_socket(void);

static void next_socket(void)
{
	// sanity check
	if(++socket_index == (SOCKET_POOL_LEN-1))
		socket_index = 0;

	_socket = &(modem_sockets[socket_index]);
}

static void comm_handler_task(void *pvParameters)
{

	// get a reference to the socket we'll be working with
	_socket = &(modem_sockets[socket_index]);

	task_mgr = NULL;

	while(true) {

		// sanity check here
		if(modem_ready)
			bytes_received = modem_handler_async(20);

		// init the modem
		if(comm_state == COMM_INIT) {
			sys_result result = comm_init();
			if(result == SYS_OK){
				printf("modem ready!\r\n");
				modem_ready = true;
			}
		}

		// configure modem
		if(comm_state == COMM_CONFIG) {
			comm_config();
		}

		// register with network
		if(comm_state == COMM_REGISTER) {
			comm_register();
		}

		if(comm_state == COMM_WAITSOCKETREQUEST) {
			printf("waiting socket request...\r\n");
			vTaskDelay(1000);
		}

		if(comm_state == COMM_IDLE || comm_state == COMM_CONNECT || comm_state == COMM_SEND || comm_state == COMM_RECEIVE || comm_state == COMM_SUSPEND || comm_state == COMM_CLOSE) {

			// comm_idle, comm_connect, comm_send, comm_receive, comm_suspend
			// sanity check task_handler should never be null
			if(_socket->task_handler != NULL) {
				// copy received data to socket(n) buffer
				_socket->bytes_received = modem_copy_buffer(_socket->rx_buffer);

				// check for modem state changes like:
				// "NO CARRIER"
				// "ERROR"
				handle_modem_events();

				// execute the socket's task_handler
				// the task will also handle any bubbling up of data to the datareceived callback
				_socket->task_handler(_socket);
			} else {
				printf("task_handler is NULL\r\n");
			}

#ifdef SOCKET_ROUNDROBIN
			// move to next socket
			next_socket();
#endif

		}

		if(comm_ready) {
			// handle comm response queue
//			response_queue();
			// handle comm request queue
			request_queue();
		}


		UNIT_TEST_YIELD;

		vTaskDelay(100);

	}

}

void handle_modem_events(void)
{
	char * ptr = NULL;

	// just handle "NO CARRIER" for now
	if((ptr = strstr(_socket->rx_buffer, MODEM_TOKEN_NOCARRIER))) {
		_socket->event_handler->on_disconnect();
		_socket->socket_status = SCK_CLOSED;
		// todo: review following two lines
		comm_enterstate(_socket, COMM_IDLE);
		socket_exitstate(_socket);
	}

//	if((ptr = strstr(_socket->rx_buffer, MODEM_TOKEN_NOCARRIER))) {
//		_socket->event_handler->on_disconnect();
//	} else if ((ptr = strstr(_socket->rx_buffer, MODEM_TOKEN_ERROR))) {
//
//	}
}


static void request_queue(void)
{
//	BaseType_t result;
	comm_request_t request;

	result = xQueueReceive(xCommQueueRequest, &request, QUEUE_TICKS);

	if(result == pdTRUE) {

		// dispatch new comm requests
		if(request.type == REQUEST_CONNECT) {
			comm_enterstate(_socket, COMM_CONNECT);
		}

		if(request.type == REQUEST_SEND) {
			comm_enterstate(_socket, COMM_SEND);
		}

		if(request.type == REQUEST_RECEIVE) {
			comm_enterstate(_socket, COMM_RECEIVE);
		}

		if(request.type == REQUEST_SUSPEND) {
			comm_enterstate(_socket, COMM_SUSPEND);
		}

		if(request.type == REQUEST_CLOSE) {
			comm_enterstate(_socket, COMM_CLOSE);
		}

	}
}

static void response_queue(void)
{
//	BaseType_t result;
	comm_frame_t frame;

	result = xQueueReceive(xCommQueue, &frame, QUEUE_TICKS);

	if(result == pdTRUE) {


		//comm_frame_t *frame = (comm_frame_t*) out_buffer;

		if(frame.type == FRAME_TCP)
		{
			// TODO: Enqueue to TCP thread
		}

		if(frame.type == FRAME_HTTP)
		{
			printf("%s\r\n", frame.buffer);
			//printf("%s\r\n", out_buffer);
			// TODO: Enqueue to HTTP thread
		}

		if(frame.type == FRAME_UDP)
		{
			// TODO: Enqueue to UDP thread
		}
	}
}

void unit_test_commidle(void)
{
	// take a breather
	comm_set_state(COMM_IDLE);
	if(comm_state == COMM_IDLE) {
		comm_idle(&(modem_sockets[socket_index]));
	}

	// TODO: HACK TO TEST ABOVE CODE
	vTaskDelay(10);

}

comm_state_t last_state = 255;

void unit_test_guardtime(void)
{

	if(last_state == comm_state)
		return;
	// take a breather

	if(comm_state == COMM_CONNECT) {
		comm_enterstate(_socket, COMM_SUSPEND);
		last_state = comm_state;
	}

	if(comm_state == COMM_SUSPEND) {
		comm_enterstate(_socket, COMM_CONNECT);
		last_state = comm_state;
	}

	if(comm_state == COMM_IDLE) {
		comm_enterstate(_socket, COMM_CONNECT);
		last_state = comm_state;
	}



	// TODO: HACK TO TEST ABOVE CODE
	vTaskDelay(10);

}
