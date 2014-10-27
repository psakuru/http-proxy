#include "proxy.h"

vector<Client*> clients;
fd_set fds_all, fds_read;
map<string, int> cache_map;
map<string, int> file_map;
map<int, time_t> expire_map;
int cache_count = -1;
int cache_rank = 0;

void initializeAddress(struct sockaddr_in* sock_iadd) {
	memset((struct sockaddr*) sock_iadd, '\0', sizeof(sock_iadd));
}

void convertToChar(char* buf,time_t time1){

	struct tm * ptm;
	ptm = gmtime ( &time1 );
	strftime (buf,30,"%a, %d %b %Y %H:%M:%S %Z",ptm);

}

void printHeader(string message){
	cout<<message.substr(0,message.find("\r\n\r\n"))<<endl;
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

int createHttpConnectionAndSendReq(string url, int req_status = NORMAL,char* buf = NULL) {
	int httpfd;
	string host_name = getHostFromURL(url);
	string resource = getResourceFromUrl(url);
	cout << host_name << "\t" << resource << "\t" << url << endl;
	char* IP;
	IP = (char*) malloc(8);

	int status = getIpFromHost(host_name, IP);

//cout<<status<<endl;
//fflush(stdout);
	if (status == -1) {
		cout << "url cannot be resolved" << endl;
	}

	cout << "web server " << host_name << " translated to IP " << IP << endl;

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
	string message;
	if(req_status == NORMAL)
		message = generateHttp1_0Header(url);
	else if(req_status == REFRESH && buf != NULL)
		message = generateConditionalGet(url,buf);
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
time_t parseExpdate(string recv_header){
	size_t expirePos = -1;
	expirePos = recv_header.find("\r\nExpires: ");
	string exp="";
	if (expirePos != string::npos) {
		exp = recv_header.substr(expirePos + 11);
	}
	expirePos = exp.find("\r\n");
	string final1 = "";
	if (expirePos != string::npos)
		final1 = exp.substr(0, expirePos);
	time_t rawtime;
	struct tm timeinfo={0};
//  Thu, 23 Oct 2014 07:25:34 GMT
	cout<<final1<<endl;
	
	strptime (final1.c_str(),"%a, %d %b %Y %H:%M:%S %Z",&timeinfo);	
	rawtime = timegm(&timeinfo);
	//cout<<"file time is "<<final1<<" and current time is "<<p<<endl;
	cout<<"file time is "<<rawtime<<" and current time is "<<time(0)<<endl;
	return rawtime;
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

int getStatusFromClientfd(int fd){
	for (std::vector<Client*>::iterator it = clients.begin();
		it != clients.end(); ++it) {
		if ((*it)->clientfd == fd) {
		//isClient = true;
			return (*it)->status;
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

int setCacheRank(string url){
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
	return cache_rank;
}

string findLastRankedUrl(){
	for (map<string, int>::iterator it = cache_map.begin(); it != cache_map.end();
		++it) {
		if(it->second == cache_rank - CACHE_SIZE)
			return it->first;
	}
}



int sendCondGET(string url){

	return 0;
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
	printHeader(recv_header);
	if (recv_header.length() != 0) {
		string url = UrlFromHeader(recv_header);
		cout << "url requested by client " << fd << " is " << url << endl;
		if (url == "ILLEGALILLEGAL") {
			cout << "Bad GET header" << endl;
			FD_CLR(c->clientfd, &fds_all);
			close(c->clientfd);
			c->~Client();
			//handle vector delete
			clients.erase(clients.begin()+getClientPos(c->clientfd));
		} else {
			c->url = url;
			string file = to_string(getLocalFileFromUrl(url));
			bool file_exist = fileExist(file);

			if (file_exist) {
				cout<<"file found in cache and using it to respond to client"<<endl;
				//check expire map and then if..else
				time_t cacheExpire = expire_map.find(getLocalFileFromUrl(url))->second;
				time_t now = time(0);
				if(now > cacheExpire){
				//send cond get and handle
					cout<<"Cache is expired. Sending conditional GET to server"<<endl;
					char* buf;
					buf = (char*)malloc(30);
					memset(buf,'\0',30);
					convertToChar(buf,cacheExpire);
					cout<<buf<<endl;
					c->out_fd = createHttpConnectionAndSendReq(url,REFRESH,buf);
					c->status = REFRESH;
				//cout<<cquery<<endl;
				} else{
			//-------------------------------------//
					cout<<"cache hasn't expired yet. Hence directly sending cached data to client"<<endl;
					cout<<"url: "<<url<<" is now ranked highest at "<<setCacheRank(url)<<endl;
					cout<<"file name to be send is "<<file<<endl;
					ifstream cachedFile;

					string toSendMessage = "";
					cachedFile.open(file.c_str(), ios::binary | ios::in);
				//Data_block* db = new Data_block;
				//vector<Data_block*> cache_send;
					if(cachedFile.good())
						cout<<"Opened cache file is in good state"<<endl;

					cachedFile.seekg (0, cachedFile.end);
					int length = cachedFile.tellg();
					cachedFile.seekg (0, cachedFile.beg);
					cout<<"length of the cached file is "<<length<<endl;
					char* p;
					p = (char*) malloc(length);
					memset(p,'\0',length);
					while (cachedFile.read(p, length)) {
					//toSendMessage.push_back(p);
					//cout<<string(message)<<endl;	
					//cout<<p<<endl;	
						if ((send(fd, p,
							length, 0)) < 0) {
							cout << "ERROR SENDING" << endl;
					}
					memset(p,'\0',length);
				}
				cachedFile.close();

				FD_CLR(fd, &fds_all);
				close(fd);
				c->~Client();
			}}else {
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
		//cout<<"i"<<endl;
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
		//cout<<"message";
		recv_header = recv_header + string(message);
//cout<<message;
//cout<<"\ntest compare "<<(rec<BUFFER_SIZE)<<endl;
		memset(message, '\0', BUFFER_SIZE);
		//memset(d1,'\0',sizeof(Data_block));
	} while (rec >= 0); //check last 4 bytes is \r\n\r\n
	printHeader(recv_header);
	//string url = UrlFromHeader(recv_header);
	int client = getClientfdFromOutfd(fd);
	cout << "web server fd " << fd << " was connected to client fd "
	<< client << endl;
	int client_status = getStatusFromClientfd(client);
	cout<<"client_statu: "<<client_status<<endl;
	if(client_status == REFRESH){
		Client* sclient = new Client;
		for (std::vector<Client*>::iterator it = clients.begin();
			it != clients.end(); ++it) {
			if((*it)->clientfd == client){
				sclient = (*it);
				break;
			}
		}
		setCacheRank(sclient->url);
		int ep_file = getLocalFileFromUrl(sclient->url);
		if(recv_header.find("304")!=string::npos){
			//update expire date and send file from cache.	
			cout<<"304 not modified was observed"<<endl;
			expire_map[ep_file] = parseExpdate(recv_header);
			ifstream cachedFile;

			cachedFile.open(to_string(ep_file).c_str(), ios::binary | ios::in);
			cachedFile.seekg (0, cachedFile.end);
			int length = cachedFile.tellg();
			cachedFile.seekg (0, cachedFile.beg);
			char* p;
			p = (char*) malloc(length);
			memset(p,'\0',length);
			while (cachedFile.read(p, length)) {

				if ((send(sclient->clientfd, p,
					length, 0)) < 0) {
					cout << "ERROR SENDING" << endl;

			}
		}
		cachedFile.close();

		FD_CLR(sclient->clientfd, &fds_all);
		close(sclient->clientfd);
		sclient->~Client();


	} else {
			//update cache
		cout<<"file has changed since the last cache"<<endl;
		bool expireExists = parseMsgForExp(recv_header);
		if (expireExists == true) {
			time_t expireDate = parseExpdate(recv_header);
			time_t now = time(0);
			if(expireDate > now){
				ofstream ex_writeFile;
				expire_map[ep_file] = parseExpdate(recv_header);
				ex_writeFile.open(to_string(ep_file).c_str(),ios::binary|ios::out);
				for(vector<Data_block*>::iterator it = recvd.begin();it != recvd.end();it++)   
					ex_writeFile.write((*it)->message,(*it)->size);
				ex_writeFile.flush();
			//cout<<recv_header;
				ex_writeFile.close();
			}
		}
		for(vector<Data_block*>::iterator it = recvd.begin();it != recvd.end();++it)   
			if ((send(sclient->clientfd, (*it)->message, (*it)->size, 0)) < 0) {
				cout << "ERROR SENDING" << endl;
			}

			FD_CLR(client, &fds_all);
			close(client);
			sclient->~Client();
		}
	} else{
		bool file_write = false;
		bool expireExists = parseMsgForExp(recv_header);
		string file_string = "";
		if (expireExists == true) {
			time_t expireDate = parseExpdate(recv_header);
			time_t now = time(0);
			if(expireDate > now-4){
				file_write = true;
				string url = getUrlFromOutFD(fd);
				cout << "writing to a file" << endl;
		//check cache_count
		//if cache_count < cache_size
				if(cache_count < CACHE_SIZE - 1 ){
					cout<<"cache limit not reached. writing to cache"<<endl;
					cache_count++;
					cout<<"cache count is now: "<<cache_count<<endl;
//				cache_rank++;
					setCacheRank(url);
			//entry to cache_map for rank
					file_map[url] = cache_count;
			//entry to file_map
					ofstream writeFile;
					cout << url << endl;
					expire_map[cache_count] = expireDate;
					writeFile.open(to_string(cache_count).c_str(),ios::binary|ios::out);
					file_string = to_string(cache_count);
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
					expire_map[file_n1] = expireDate;
					writeFile.open(to_string(file_n1).c_str(),ios::binary|ios::out);
					file_string = to_string(file_n1);
			//assert(! writeFile.fail( ));  
					for(vector<Data_block*>::iterator it = recvd.begin();it != recvd.end();++it)      
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
		}
if(!file_write){
		for(vector<Data_block*>::iterator it = recvd.begin();it != recvd.end();++it)   
			if ((send(client, (*it)->message, (*it)->size, 0)) < 0) {
				cout << "ERROR SENDING" << endl;
			}
} else{
			ifstream cachedFile;

			cachedFile.open(file_string.c_str(), ios::binary | ios::in);
			cachedFile.seekg (0, cachedFile.end);
			int length = cachedFile.tellg();
			cachedFile.seekg (0, cachedFile.beg);
			char* p;
			p = (char*) malloc(length);
			memset(p,'\0',length);
			while (cachedFile.read(p, length)) {

				if ((send(client, p,
					length, 0)) < 0) {
					cout << "ERROR SENDING" << endl;

			}
		}
		cachedFile.close();
}
			FD_CLR(client, &fds_all);
			close(client);
			for (std::vector<Client*>::iterator it = clients.begin();
				it != clients.end(); ++it) {
				if((*it)->clientfd == client){
					Client* sc = (*it);
					sc->~Client();
					break;
				}
			}
		}
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

