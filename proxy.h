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

void initializeAddress(struct sockaddr_in* sock_iadd);

class Client{
    public:
        int clientfd;
        struct sockaddr_in client_address;
        string url;
        int out_fd;
        unsigned clength;
        
        void sendDataFromCache();
        Client(){
            clength = sizeof(struct sockaddr_in);
            initializeAddress(&client_address);
            out_fd = -1;
        };
        ~Client();
};



#endif /* PROXY_H_ */
