/*
 * httputil.cpp
 *
 *  Created on: Oct 15, 2014
 *      Author: psakuru
 */

#include "httputil.h"

string getHostFromURL(string url) {

	size_t isHttpFound = -1;
	isHttpFound = url.find("http://");
	if (isHttpFound != string::npos) {
		url = url.substr(isHttpFound + 7);
	}
	size_t isHttpsFound = -1;
	isHttpsFound = url.find("https://");
	if (isHttpsFound != string::npos) {
		url = url.substr(isHttpsFound + 8);
	}
	
	size_t host_end = url.find("/");
	
	string host = url.substr(0,host_end);
	//cout <<"url: "<< url << endl;
	//cout <<"host: "<< host << endl;

	return host;
}

string getResourceFromUrl(string url){
    size_t isHttpFound = -1;
	isHttpFound = url.find("http://");
	if (isHttpFound != string::npos) {
		url = url.substr(isHttpFound + 7);
	}
	size_t isHttpsFound = -1;
	isHttpsFound = url.find("https://");
	if (isHttpsFound != string::npos) {
		url = url.substr(isHttpsFound + 8);
	}
	
	size_t host_end = url.find("/");
	if(host_end == string::npos){
	    return string("/");
	}
	string resource = url.substr(host_end);
	//cout << url << endl;
	//cout << resource << endl;

	return resource;
}

string generateHttp1_0Header(string url){
    string query2 = string("GET ")+getResourceFromUrl(url)+string(" HTTP/1.0\r\n")+string("Host: ")+getHostFromURL(url)+string("\r\n")+string("\r\n");
    return query2;
}


string decodeHostNameHttp1_0(string recv_header){
		size_t isHostFound = -1;
		isHostFound = recv_header.find("Host: ");
		if(isHostFound != string::npos){
			string host_name = recv_header.substr(isHostFound+6);
			host_name = host_name.substr(0,host_name.find("\r\n"));
			return host_name;
		} else{
			//cout<<"illegal Header";
			return "ILLEGAL";
		}
		
}

string decodeResourceHttp1_0(string recv_header){
			size_t isGetFound = -1;
		isGetFound = recv_header.find("GET ");
		if(isGetFound != string::npos){
			string resource_name = recv_header.substr(isGetFound+5);
			resource_name = resource_name.substr(0,resource_name.find(" HTTP/1.0\r\n"));
			return resource_name;
		} else{
			//cout<<"illegal Header";
			return "ILLEGAL";
		}
}


string UrlFromHeader(string recv_header){
		string url = decodeHostNameHttp1_0(recv_header)+string("/")+decodeResourceHttp1_0(recv_header);
		return url;
}


int getIpFromHost(string host_name,char* ip){
		struct hostent *he;
    struct in_addr **addr_list;
    int i;
    int occurrences = 0;
		string::size_type start = 0;
		string dot = ".";
while ((start = host_name.find(dot, start)) != string::npos) {
    ++occurrences;
    start += dot.length(); // see the note
}     
//cout<<"occ"<<occurrences<<endl;
		if(occurrences == 3){
			strcpy(ip,host_name.c_str());
			return 1;
		}
    if ( (he = gethostbyname( host_name.c_str() ) ) == NULL) 
    {
        // get the host info

        return -1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
//cout<<sizeof(inet_ntoa(*addr_list[0]));
    if(addr_list[0] != NULL) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[0]) );
        return 1;
    }
     
    return -1;
}


string getFileNameFromResource(string url){
	string file_name;
	file_name = url;
	do{
	size_t host_end = file_name.find("/");
	if (host_end != string::npos) {
		file_name = file_name.substr(host_end+1);
	}
	//cout<<"file "<<file_name<<endl;
	} while(file_name.find("/") != string::npos);
//	cout<<"file name: "<<endl;
	return file_name;
}
