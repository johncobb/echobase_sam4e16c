
#include <stdio.h>
#include <string.h>


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "asf.h"
#include "conf_board.h"
#include "cli_tasks.h"
#include "comm.h"
#include "sysclk.h"
#include "app_task.h"





// folder structure modeled after FreeRTOS/Demo/CORTEX_ATSAM3X_Atmel_Studio
// source example: asf/thirdparty/freertos/demo/peripheral_control/sam3x8e_arduino_due_x


// Available commands
/*
 *
 *
help:
 Lists all the registered commands

task-stats:
 Displays a table showing the state of each FreeRTOS task

run-time-stats:
 Displays a table showing how much processing time each FreeRTOS task has used

echo-3-parameters <param1> <param2> <param3>:
 Expects three parameters, echos each in turn

echo-parameters <...>:
 Take variable number of parameters, echos each in turn

create-task <param>: *** NOT WORKING ***
 Creates a new task that periodically writes the parameter to the CLI output

delete-task:
 Deletes the task created by the create-task command
 *
 *
 */





/* The priorities at which various tasks will get created. */
#define mainUART_CLI_TASK_PRIORITY              (tskIDLE_PRIORITY + 1)
#define mainLED_TASK_PRIORITY					(tskIDLE_PRIORITY + 1)
#define mainCOMM_TASK_PRIORITY					(tskIDLE_PRIORITY + 1)
#define mainWAN_TASK_PRIORITY					(tskIDLE_PRIORITY + 1)
#define mainWANPROC_TASK_PRIORITY				(tskIDLE_PRIORITY + 1)
#define mainAPPTASK_TASK_PRIORITY				(tskIDLE_PRIORITY + 1)

/* The stack sizes allocated to the various tasks. */
#define mainUART_CLI_TASK_STACK_SIZE    		(1024)
#define mainLED_TASK_STACK_SIZE					(configMINIMAL_STACK_SIZE * 2)
#define mainWAN_TASK_STACK_SIZE					(1024)
#define mainWANPROC_TASK_STACK_SIZE					(1024)
#define mainCOMM_TASK_STACK_SIZE				(2048)
#define mainAPPTASK_TASK_STACK_SIZE				(2048)


// TODO: REVIEW STACK SIZE ALLOCATION
//#define TASK_MONITOR_STACK_SIZE            (2048/sizeof(portSTACK_TYPE))
//#define TASK_MONITOR_STACK_PRIORITY        (tskIDLE_PRIORITY)
//#define TASK_LED_STACK_SIZE                (1024/sizeof(portSTACK_TYPE))
//#define TASK_LED_STACK_PRIORITY            (tskIDLE_PRIORITY)
//#define TASK_CLI_STACK_SIZE                (2048/sizeof(portSTACK_TYPE))
//#define TASK_CLI_STACK_PRIORITY            (tskIDLE_PRIORITY)


void create_led_task(void);

uint32_t clock_millis = 0;

extern void vApplicationMallocFailedHook(void) {
	taskDISABLE_INTERRUPTS();
	for (;;)
		;
}

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *) pcTaskName);
	for (;;)
		;
}

extern void vApplicationIdleHook(void) {
}

extern void vApplicationTickHook(void) {
	clock_millis++;
}

uint32_t clock_time(void)
{
	return clock_millis;
}

bool pin_powmon = false;

//uint32_t timeout = 0;

static void task_led(void *pvParameters) {
	UNUSED(pvParameters);

//	timeout = ((uint32_t)sys_now()) + (2000);

	for (;;) {

//		if(sys_now() >= timeout) {
//			printf("timeout\r\n");
//			timeout = ((uint32_t)sys_now()) + (2000);
//		}

//		pio_toggle_pin(MDM_ONOFF_IDX);
//		pio_toggle_pin(MDM_ENABLE_IDX);
//		pio_toggle_pin(MDM_RESET_IDX);

		pio_toggle_pin(MCU_LED1);
		pio_toggle_pin(MCU_LED2);
		vTaskDelay(500);
	}
}

#include "conf_uart_serial.h"
#include "stdio_serial.h"

#define UART_BAUDRATE      115200
#define UART_CHAR_LENGTH   US_MR_CHRL_8_BIT
#define UART_PARITY      US_MR_PAR_NO

static void configure_console(void) {


	board_init_printf_uart();
	sysclk_enable_peripheral_clock(PRINTF_UART_ID);

	const usart_serial_options_t uart_serial_options = {
	      .baudrate = UART_BAUDRATE,
	      .charlength =   UART_CHAR_LENGTH,
	      .paritytype = UART_PARITY,
	      .stopbits = false
	      //US_MR_CHMODE_NORMAL
	   };

	usart_serial_init(PRINTF_UART, &uart_serial_options);
	stdio_serial_init(PRINTF_UART, &uart_serial_options);

	usart_enable_tx(PRINTF_UART);
	usart_enable_rx(PRINTF_UART);
}

int main(void) {





	sysclk_init();
	board_init();

	configure_console();
	printf("CPH EchoBase v%d\r\n", 1);

//	printf("create_uart_cli_task\r\n");
//	create_uart_cli_task(CONSOLE_UART, mainUART_CLI_TASK_STACK_SIZE, mainUART_CLI_TASK_PRIORITY);

//	printf("create_comm_task\r\n");
//	create_comm_task(mainCOMM_TASK_STACK_SIZE, mainCOMM_TASK_PRIORITY);


	printf("create_wan_task\r\n");
	create_wan_task(mainWAN_TASK_STACK_SIZE, mainWAN_TASK_PRIORITY);
	printf("create_proc_wan_task\r\n");
	create_wan_proc_task(mainWANPROC_TASK_STACK_SIZE, mainWANPROC_TASK_PRIORITY);
//
//
//	printf("create_apptask_task\r\n");
//	create_app_task(mainAPPTASK_TASK_STACK_SIZE, mainAPPTASK_TASK_PRIORITY);


	printf("create_led_task\r\n");
	create_led_task();


	printf("starting task scheduler\r\n");
	/* Start the scheduler. */
	vTaskStartScheduler();

	for (;;) {
	}

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}

void create_led_task(void)
{
	/* Create task to make led blink */
	if (xTaskCreate(task_led, "Led", mainLED_TASK_STACK_SIZE, NULL, mainLED_TASK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create test led task\r\n");
	}
}

void assert_triggered(const char *file, uint32_t line)
{
	volatile uint32_t block_var = 0, line_in;
	const char *file_in;

	/* These assignments are made to prevent the compiler optimizing the
	values away. */
	file_in = file;
	line_in = line;
	(void) file_in;
	(void) line_in;

	taskENTER_CRITICAL();
	{
		while (block_var == 0) {
			/* Set block_var to a non-zero value in the debugger to
			step out of this function. */
		}
	}
	taskEXIT_CRITICAL();
}



