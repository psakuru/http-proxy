/*
 * proxy.h
 *
 *  Created on: Oct 15, 2014
 *      Author: psakuru
 */

#ifndef PROXY_H_
#define PROXY_H_

#include "header.h"
#include "httputil.h"

#define NORMAL 0
#define REFRESH 1

void initializeAddress(struct sockaddr_in* sock_iadd);

class Client{
    public:
        int clientfd;
        struct sockaddr_in client_address;
        string url;
        int out_fd;
        unsigned clength;
        int status;
        void sendDataFromCache();
        Client(){
            clength = sizeof(struct sockaddr_in);
            initializeAddress(&client_address);
            out_fd = -1;
            status = NORMAL;
        };
        ~Client(){
            clientfd = -1;
            out_fd = -1;
        };
};

typedef struct data_block {
	char message[BUFFER_SIZE];
	int size;
} Data_block;


typedef struct expire_st{
	int file_number;
	time_t exp_time;
	string exp_time_instr;
} expire_st;
#endif /* PROXY_H_ */
