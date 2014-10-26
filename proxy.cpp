#include "proxy.h"

vector<Client*> clients;
fd_set fds_all, fds_read;
map<string, int> cache_map;
map<string, int> file_map;
int cache_count = -1;
int cache_rank = 0;

void initializeAddress(struct sockaddr_in* sock_iadd) {
	memset((struct sockaddr*) sock_iadd, '\0', sizeof(sock_iadd));
}

string to_string(int Number){
	string Result;          // string which will contain the result
	ostringstream convert;   // stream used for the conversion
	convert << Number;      // insert the textual representation of 'Number' in the characters in the stream
	Result = convert.str();
	return Result;
}

bool fileExist(string file) {
	ifstream checkFile;
	checkFile.open(file.c_str());
	if (checkFile.good()) {
		checkFile.close();
		return true;
	} else {
		checkFile.close();
		return false;
	}
}

int createHttpConnectionAndSendReq(string url) {
	int httpfd;
	string host_name = getHostFromURL(url);
	string resource = getResourceFromUrl(url);
	cout << host_name << "\t" << resource << "\t" << url << endl;
	char* IP;
	IP = (char*) malloc(8);

	int status = getIpFromHost(host_name, IP);
	cout << "web server " << host_name << " translated to IP " << IP << endl;
	//cout<<status<<endl;
	//fflush(stdout);
	if (status == -1) {
		cout << "url cannot be resolved" << endl;
	}

	if ((httpfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout << "ERROR CREATING THE SERVER SOCKET CONNECTION" << endl;
		exit(1);
	}

	struct sockaddr_in hadd;

	initializeAddress(&hadd);

	hadd.sin_family = AF_INET;
	hadd.sin_port = htons(80);
	hadd.sin_addr.s_addr = htonl(inet_network(IP));
//cout<<IP<<endl;
	if ((connect(httpfd, (struct sockaddr*) &hadd, sizeof(hadd))) < 0) {
		cout << "ERROR CONNECTING" << endl;
		cout << strerror(errno) << endl;
		exit(1);
	}
	cout << "http socket is " << httpfd << endl;
	string message = generateHttp1_0Header(url);
	cout << message << endl;
	if ((send(httpfd, message.c_str(), message.length(), 0)) < 0) {
		cout << "ERROR SENDING" << endl;
	}
	FD_SET(httpfd, &fds_all);

	cout << "sent request to web server" << endl;
	return httpfd;
}

string getUrlFromOutFD(int fd) {
	for (std::vector<Client*>::iterator it = clients.begin();
			it != clients.end(); ++it) {
		if ((*it)->out_fd == fd) {
			//isClient = true;
			return (*it)->url;
			break;
		}
	}
}

bool parseMsgForExp(string recv_header) {
	size_t expirePos = -1;
	expirePos = recv_header.find("\r\nExpires: ");
	string exp;
	if (expirePos != string::npos) {
		exp = recv_header.substr(expirePos + 11);
	}
	expirePos = exp.find("\r\n");
	string final;
	if (expirePos != string::npos)
		final = exp.substr(0, expirePos);

	//cout<<"final"<<final<<endl;
	if (final == "-1" || final.length() == 0)
		return false;
	else
		return true;
}

int getClientfdFromOutfd(int fd) {
	for (std::vector<Client*>::iterator it = clients.begin();
			it != clients.end(); ++it) {
		if ((*it)->out_fd == fd) {
			//isClient = true;
			return (*it)->clientfd;
			break;
		}
	}
}

int getClientPos(int fd) {
	int pos = 0;
	for (std::vector<Client*>::iterator it = clients.begin();
			it != clients.end(); ++it, ++pos) {
		if ((*it)->clientfd == fd) {
			//isClient = true;
			return pos;
			break;
		}
	}
}

int getLocalFileFromUrl(string url) {
	for (map<string, int>::iterator it = file_map.begin(); it != file_map.end();
			++it) {
		if (it->first == url)
			return it->second;
	}
	return -1;
}

void setCacheRank(string url){
	cache_rank++;
	bool flag = false;
	for (map<string, int>::iterator it = cache_map.begin(); it != cache_map.end();
			++it) {
		if (it->first == url){
			it->second = cache_rank;
			flag = true;
		}
	}
	if(!flag){
		//add new entry to cache_count
		cache_map[url] = cache_rank;
	}
	
}

string findLastRankedUrl(){
	for (map<string, int>::iterator it = cache_map.begin(); it != cache_map.end();
			++it) {
		if(it->second == cache_rank - CACHE_SIZE)
			return it->first;
	}
}

void recvAndProcessServer(int fd) {
	char message[BUFFER_SIZE];
	memset(message, '\0', BUFFER_SIZE);
	vector<Data_block*> recvd;
	
	//memset(d1,'\0',sizeof(Data_block));
	string recv_header = "";
	int rec = 0;
	fd_set temp_fds;
	Client* c = new Client();
	bool isClient = false;
	cout << "recv from : " << fd << endl;
	for (std::vector<Client*>::iterator it = clients.begin();
			it != clients.end(); ++it) {
		if ((*it)->clientfd == fd) {
			isClient = true;
			c = *it;
			break;
		}
	}
	if (isClient) {
		cout << "message is recv from client" << endl;
		do {
			if ((rec = recv(fd, message, sizeof(message), 0)) < 0) {
				cout << "error receiving...closing client connection" << endl;
				FD_CLR(fd, &fds_all);
				close(fd);
				break;
			}

			if (rec == 0) {
				cout << "Client disconnected" << endl;
				close(c->clientfd);
//		handle vector delete
				//exit(1);
				FD_CLR(fd, &fds_all);
				close(fd);
				break;
			}

			recv_header = recv_header + string(message);

		} while (strcmp(message + rec - 4, "\r\n\r\n")); //check last 4 bytes is \r\n\r\n
//cout<<recv_header.length()<<endl;
		if (recv_header.length() != 0) {
			string url = UrlFromHeader(recv_header);
			cout << "url requested by client " << fd << " is " << url << endl;
			if (url == "ILLEGALILLEGAL") {
				cout << "Bad GET header" << endl;
				FD_CLR(c->clientfd, &fds_all);
				close(c->clientfd);
				//handle vector delete
				clients.erase(clients.begin()+getClientPos(c->clientfd));
			} else {
				c->url = url;
				string file = to_string(getLocalFileFromUrl(url));
				bool file_exist = fileExist(file);
				//cout<<"fileexist: "<<file_exist<<endl;
				//cout<<"url requested i.e., "<<url<<endl;
				//cout<<"cache_map is as follows"<<endl;
				//for(map<string, int>::iterator it = cache_map.begin(); it != cache_map.end();
			//++it) {
				//cout<<"it_first "<<it->first<<"\tit_Second "<<it->second<<endl;
			//}
				if (file_exist) {
				cout<<"file found in cache and using it to respond to client"<<endl;
					setCacheRank(url);
				//	cout<<"stage 1"<<endl;
					//transfer File
					ifstream cachedFile;
					//cout<<"stage 2 "<< file<<endl;
					string toSendMessage = "";
					cachedFile.open(file.c_str(), ios::binary | ios::in);
					char p;
					while (cachedFile.read(&p, 1)) {
						toSendMessage.push_back(p);
						//cout<<string(message)<<endl;
						memset(&p, '\0', sizeof(char));
					}
					cachedFile.close();
					if ((send(fd, toSendMessage.c_str(),
							toSendMessage.length(), 0)) < 0) {
						cout << "ERROR SENDING" << endl;
					}
					FD_CLR(fd, &fds_all);
					close(fd);
				}else {
			//send http request
			cout
					<< "Requested resource not found in cache. Connecting to web server."
					<< endl;
			c->out_fd = createHttpConnectionAndSendReq(c->url);

		}
			}

		} 
	} else {
		//recv from http server and save to file
		cout << "recv from http server and save to file if exp exist" << endl;
		
		do {
			cout<<"i"<<endl;
			Data_block* d1 = new Data_block;
			if ((rec = recv(fd, message, BUFFER_SIZE, 0)) < 0) {
				cout << "error receiving...closing web connection" << endl;
				FD_CLR(fd, &fds_all);
				close(fd);
				break;
			}

			if (rec == 0) {
				cout << "web disconnected" << endl;
				FD_CLR(fd, &fds_all);
				close(fd);
				//exit(1);	
			}
			memcpy(d1->message, message, rec);
			d1->size = rec;
			recvd.push_back(d1);
			cout<<"message";
			recv_header = recv_header + string(message);
//cout<<message;
//cout<<"\ntest compare "<<(rec<BUFFER_SIZE)<<endl;
			//memset(message, '\0', BUFFER_SIZE);
			//memset(d1,'\0',sizeof(Data_block));
		} while (rec >= 0); //check last 4 bytes is \r\n\r\n

		//string url = UrlFromHeader(recv_header);

		bool expireExists = parseMsgForExp(recv_header);
		if (expireExists == true) {
			string url = getUrlFromOutFD(fd);
			cout << "writing to a file" << endl;
			//check cache_count
			//if cache_count < cache_size
			if(cache_count < CACHE_SIZE - 1 ){
			cout<<"cache limit not reached. writing to cache"<<endl;
				cache_count++;
//				cache_rank++;
				setCacheRank(url);
				//entry to cache_map for rank
				file_map[url] = cache_count;
				//entry to file_map
				ofstream writeFile;
			  cout << url << endl;
			  writeFile.open(to_string(cache_count).c_str(),ios::app|ios::binary|ios::out);
				//assert(! writeFile.fail( ));  
				for(vector<Data_block*>::iterator it = recvd.begin();it != recvd.end();it++)   
				writeFile.write((*it)->message,(*it)->size);
				writeFile.flush();
				//cout<<recv_header;
				writeFile.close();
				
			} else {
	//			cache_rank++;
		//		cache_map[url] = cache_rank;
		cout<<"cache limit reached. Removing last used file to write new one"<<endl;
				setCacheRank(url);
				string oldUrl = findLastRankedUrl();	
				cout<<"url last used is "<<oldUrl<<endl;
				int file_n1 = file_map[oldUrl];
				file_map.erase(file_map.find(oldUrl));
				file_map[url]=file_n1;
				
				ofstream writeFile;
				cout << url << endl;
				writeFile.open(to_string(file_n1).c_str(),ios::app|ios::binary|ios::out);
				//assert(! writeFile.fail( ));  
				for(vector<Data_block*>::iterator it = recvd.begin();it != recvd.end();it++)      
			writeFile.write((*it)->message,(*it)->size);
				writeFile.flush();
				//cout<<recv_header;
				writeFile.close();
			}
			//create new
			//else
			//find last ranked cache
			//write to that file
			//update appropriate files
			
			
			

		}
		int client = getClientfdFromOutfd(fd);
		cout << "web server fd " << fd << " was connected to client fd "
				<< client << endl;
					for(vector<Data_block*>::iterator it = recvd.begin();it != recvd.end();it++)   
		if ((send(client, (*it)->message, (*it)->size, 0)) < 0) {
			cout << "ERROR SENDING" << endl;
		}
		FD_CLR(client, &fds_all);
		close(client);
	}
}

void Client::sendDataFromCache() {
	ifstream file;
	file.open(url.c_str());

	string message = "";
	char * p = new char[BUFFER_SIZE];
	memset(p, '\0', BUFFER_SIZE);
	while (file.read(p, BUFFER_SIZE)) {
		message = message + string(p);
		memset(&p, '\0', BUFFER_SIZE);
	}
	cout << message << endl;

	if ((send(clientfd, message.c_str(), message.length(), 0)) < 0) {
		cout << "error sending file to client" << endl;
	} else {
		cout << "sent file to client " << clientfd << " requesting url " << url
				<< endl;
	}
}

int main(int argc, char const *argv[]) {
	struct sockaddr_in sadd;
	int ssockfd;
	string server_ip;
	unsigned int port;
	const char* server;

	//fd_set fds_all, fds_read;

	if (argc != 3) {
		cout << "Usage './proxy PROXY_SERVER_IP PROXY_SERVER_PORT" << endl;
		exit(1);
	} else {
		port = atoi(argv[2]);
		server = argv[1];
	}

	cout << "Initiating server socket creation with the below parameters..."
			<< endl;
	cout << "Port " << port << "\t" << "Server " << server << "\t" << endl;

	if ((ssockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout << "ERROR CREATING THE SERVER SOCKET CONNECTION" << endl;
		exit(1);
	}

	cout << "Created Socket Successfully!!" << endl;
	initializeAddress(&sadd);

	sadd.sin_family = AF_INET;
	sadd.sin_port = htons(port);
	sadd.sin_addr.s_addr = htonl(inet_network(server));

	if ((bind(ssockfd, (struct sockaddr*) &sadd, sizeof(sadd))) < 0) {
		cout << "ERROR BINDING" << endl;
		cout << strerror(errno) << endl;
		exit(1);
	}

	if ((listen(ssockfd, 5)) < 0) {
		cout << "ERROR LISTENING" << endl;
		cout << strerror(errno);
		exit(1);
	}

	cout << "Listening..." << endl;

	FD_ZERO(&fds_all);
	FD_ZERO(&fds_read);

	FD_SET(ssockfd, &fds_all);

	struct sockaddr_in cadd;

	int client_count = 0;
	while (1) {

		fds_read = fds_all;

		int fd_count = select(FD_SETSIZE, &fds_read, NULL, NULL, NULL);
		if (fd_count == -1) {
			cout << "Error in select call" << endl;
			break;
		}
		for (int i = 0; i < FD_SETSIZE; i++) {
			if (FD_ISSET(i, &fds_read)) {
				if (ssockfd == i) {
					//handle new client connection
					Client* service_client = new Client();
					if ((service_client->clientfd =
							accept(ssockfd,
									(struct sockaddr*) &(service_client->client_address),
									&service_client->clength)) < 0) {
						cout << "error connecting" << endl;
						exit(1);
					}
					cout << "connected to client\t" << service_client->clientfd
							<< endl;
					clients.push_back(service_client);
					FD_SET(service_client->clientfd, &fds_all);

				} else {
					//handle recv from client
					cout << "Select broke on FD: " << i << endl;
					fflush(stdout);

					recvAndProcessServer(i);
				}
			}
		}
	}

	close(ssockfd);
	return EXIT_SUCCESS;

}

