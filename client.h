/*
 * client.h
 *
 *  Created on: Oct 15, 2014
 *      Author: psakuru
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "header.h"
#include "httputil.h"



typedef struct data_block {
	char message[BUFFER_SIZE];
	int size;
} Data_block;

#endif /* CLIENT_H_ */
