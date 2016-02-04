/*
 * deca_task.h
 *
 *  Created on: Jan 28, 2016
 *      Author: jcobb
 */

#ifndef SRC_APP_DECA_TASK_H_
#define SRC_APP_DECA_TASK_H_

#define DECA_MSG_SIZE		128

void create_deca_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority);

#endif /* SRC_APP_DECA_TASK_H_ */
