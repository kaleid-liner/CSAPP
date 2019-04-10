#ifndef _PROXY_H
#define _PROXY_H

#include "csapp.h"

#define MAX_URL 4096
#define MAX_METHOD 8
#define MAX_VERSION 16

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct RequestInfo {
    char host[HOST_NAME_MAX];
    int port;
    char uri[MAX_URL];
    char method[MAX_METHOD];
    char version[MAX_VERSION];
};

typedef struct RequestInfo RequestInfo_t;

int open_clientfd_n(char *host, int port);
void forward_request(int clientfd, rio_t *headers, const RequestInfo_t *request);
void proxy(int connfd);
int parse_url(char *url, RequestInfo_t *request);
void handle_response(int clientfd, int connfd);

#endif