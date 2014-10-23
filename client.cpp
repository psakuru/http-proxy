/*
 * client.cpp
 *
 *  Created on: Oct 15, 2014
 *      Author: psakuru
 */

/*
 ECEN602: Computer Networks
 HW3 programming assignment: HTTP1.0 client GET request
 */

#include "header.h"
#include "httputil.h"

void ClearAddress(struct sockaddr_in* sock_iadd) {
	memset((struct sockaddr*) sock_iadd, '\0', sizeof(sock_iadd));
}

void sendGetReqToServer(int fd, string url) {
	string message = generateHttp1_0Header(url);
	size_t length = message.length();
	cout<<message<<endl;
	if (send(fd, message.c_str(), length, 0) < 0) {
		cout << "ERROR SENDING" << endl;
		strerror(errno);
	}
}

void recvAndProcess(int fd,string url) {
	char message[BUFFER_SIZE];
	memset(message, '\0', BUFFER_SIZE);

	int rec = 0;
string recv_msg = "";
fd_set  temp_fds;
    do{
        if ((rec = recv(fd, message, sizeof(BUFFER_SIZE), 0)) < 0) {
					cout << "error receiving" << endl;
        	exit(1);
	    }
    
        if (rec == 0) {
		cout << "Server disconnected" << endl;
		close(fd);
		//exit(1);
	    }
	    
	    recv_msg = recv_msg + string(message);
       cout<<message; 
       memset(message, '\0', BUFFER_SIZE);
       FD_ZERO(&temp_fds);
       FD_SET(fd,&temp_fds);
    } while(rec>0); //check last 4 bytes is \r\n\r\n
		cout<<("entering into")<<endl;
	//	cin>>message;
		string file_name = getResourceFromUrl(url);
		if(file_name == "/"){
			file_name = "index.html";
		} else {
			file_name = getFileNameFromResource(file_name);
		}
		cout<<"test"<<file_name<<endl;
	
		 ofstream writeFile;
	    writeFile.open("1",ios::out);
	  //  assert(! writeFile.fail( ));   
	    if(writeFile.good()){
	    writeFile<<recv_msg;
	    writeFile.flush();}
	    else
	    cout<<"help "<<strerror(errno)<<endl;
	    //cout<<recv_msg;
	    writeFile.close();
	exit(1);
//	processMessage(message);
}

int main(int argc, char const *argv[]) {
	int csockfd, connectfd, port;
	struct sockaddr_in sadd;
	fd_set fds_all, fds_read;
	const char* server;
	string url;

	if ((csockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout << "ERROR CREATING THE SERVER SOCKET CONNECTION";
		exit(1);
	}

	if (argc != 4) {
		cout << "Usage : './client PROXY_SERVER_IP PROXY_SERVER_PORT URL'"
				<< endl;
		exit(1);
	} else {
		port = atoi(argv[2]);
		url = string(argv[3]);
		server = argv[1];
	}
	
	memset(&sadd, '\0', sizeof(sadd));
	sadd.sin_addr.s_addr = htonl(inet_network(server));
	sadd.sin_port = htons(port);
	sadd.sin_family = AF_INET;
	if ((connectfd = connect(csockfd, (struct sockaddr*) &sadd, sizeof(sadd)))
			< 0) {
		cout << "ERROR CONNECTING" << endl;
		cout << strerror(errno) << endl;
		exit(1);
	}

	//send get request to proxy server

	sendGetReqToServer(csockfd, url);

	//recv the file from proxy server
	recvAndProcess(csockfd,url);
	close(csockfd);
	//terminate connection
	return 0;
}

