#ifndef _HTTP_UTIL_H
#define _HTTP_UTIL_h

#include "header.h"

string generateHttp1_0Header(string url);
string getHostFromURL(string url);
string getResourceFromUrl(string url);
string decodeHostNameHttp1_0(string recv_header);
string decodeResourceHttp1_0(string recv_header);
string UrlFromHeader(string recv_header);
int getIpFromHost(string host_name, char* ip);
string getFileNameFromResource(string resource);

#endif
