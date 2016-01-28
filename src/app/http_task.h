/*
 * http_task.h
 *
 *  Created on: Jan 27, 2016
 *      Author: jcobb
 */

#ifndef SRC_APP_HTTP_TASK_H_
#define SRC_APP_HTTP_TASK_H_

#define HTTP_MSG_SIZE				1024
#define IP_ENDPIONT_HTTPTASK		"google.com"

#define 	HTTP_HTML_START			"<!doctype html>"
#define 	HTTP_HTML_END			"</html>"
//#define 	HTTP_HTML_LOGCONTENT		1

void create_http_task(uint16_t stack_depth_words, unsigned portBASE_TYPE task_priority);

#endif /* SRC_APP_HTTP_TASK_H_ */
