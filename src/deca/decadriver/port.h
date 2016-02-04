/*
 * port.h
 *
 *  Created on: Nov 19, 2015
 *      Author: ericrudisill
 */

#ifndef SRC_DECADRIVER_PORT_H_
#define SRC_DECADRIVER_PORT_H_

// Convenient redirect to keep decadriver fairly clean.
// The "port.h" that is included in all of the decadrive .c files is a poor choice for naming.
// It clashes with the port.h driver in ASF and would require us to keep the port implementation
// for the platform in the decadriver directory.

#include <cph_deca_port.h>

#endif /* SRC_DECADRIVER_PORT_H_ */
