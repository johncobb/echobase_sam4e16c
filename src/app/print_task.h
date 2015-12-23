/*
 * print_task.h
 *
 *  Created on: Dec 21, 2015
 *      Author: jcobb
 */

#ifndef SRC_APP_PRINT_TASK_H_
#define SRC_APP_PRINT_TASK_H_

#define PRINTCMD_MSG_SIZE		1024

void create_printer_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority);

#endif /* SRC_APP_PRINT_TASK_H_ */
