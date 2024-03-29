#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "csapp.h"
#include "proxy.h"
#include "cache.h"


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

static cache_t cache;
static sem_t mutex;

void send_cached_response(int connfd, cache_content_t data);

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    int port = strtol(argv[1], NULL, 10);
    if (port < 1024 || port > 49151) {
        fprintf(stderr, "Illegal port. Please enter a valid integer ranged from 1024 to 49151\n");
        exit(0);
    }

    int connfd;
    int listenfd;
    struct sockaddr_in client_addr;
    socklen_t sock_len = sizeof(client_addr);

    listenfd = open_listenfd(argv[1]);
    if (listenfd < 0) {
        fprintf(stderr, "Can't listen on port %d\n", port);
        exit(0);
    }

    cache_new(&cache);
    sem_init(&mutex, 0, 1);
    while (1) {
        connfd = accept(listenfd, (struct sockaddr *)&client_addr, &sock_len);
        int *pconnfd = malloc(sizeof(int));
        *pconnfd = connfd;
        if (connfd >= 0) {
            pthread_t thread;
            pthread_create(&thread, NULL, thread_handler, pconnfd);
        }
    }

    close(listenfd);

    return 0;
}

void *thread_handler(void *pconnfd)
{
    int connfd = *(int *)pconnfd;
    proxy(connfd);
    close(connfd);
    free(pconnfd);
    return NULL;
}

/*
 * Parse url, leave request info in request.
 * Return 0 if successful, otherwise -1.
 * 
*/
int parse_url(char *url, RequestInfo_t *request)
{
    char buf[MAX_URL];
    char host[HOST_NAME_MAX];
    char uri[MAX_URL];
    int port;

    char url_format[64];
    char host_uri_format[64];

    sprintf(url_format, "http://%%%ds", MAX_URL);
    sprintf(host_uri_format, "%%%d[^/]%%%d[^\r\n]", HOST_NAME_MAX, MAX_URL);

    if (sscanf(url, url_format, buf) <= 0) {
        return -1;
    }

    sscanf(buf, host_uri_format, host, uri);

    strncpy(request->uri, uri, MAX_URL - 1);
    request->uri[MAX_URL - 1] = '\0';
    if (request->uri[0] == '\0') {
        request->uri[0] = '/';
        request->uri[1] = '\0';
    }

    char *port_start = strchr(host, ':');
    if (port_start == NULL) {
        port = 80;
    }
    else {
        port = atoi(port_start + 1);
        *port_start = '\0';
    }
    strncpy(request->host, host, HOST_NAME_MAX - 1);
    request->host[HOST_NAME_MAX - 1] = '\0';
    size_t url_len = snprintf(request->url, MAX_URL, "%s%s", request->host, request->uri);
    if (url_len >= MAX_URL) {
        url_len = MAX_URL - 1;
    }
    if (request->url[url_len - 1] == '/') {
        request->url[url_len - 1] = '\0';
    }

    request->port = port;

    return 0;
}

void proxy(int connfd)
{
    rio_t rio;
    char buf[MAXLINE];
    rio_readinitb(&rio, connfd);

    rio_readlineb(&rio, buf, MAXLINE);

    char method[MAX_METHOD];
    char version[MAX_VERSION];
    char url[MAX_URL];

    if (sscanf(buf, "%s %s %[^\r\n]", method, url, version) != 3) {
        fprintf(stderr, "malformed http request\n");
        return;
    }

    printf("* %s", buf);

    RequestInfo_t req_info;
    if (parse_url(url, &req_info) < 0) {
        fprintf(stderr, "illegal url %s\n", url);
        return;
    }

    cache_content_t *data;
    P(&mutex);
    if (find_cache(&cache, req_info.url, &data) == 0) {
        send_cached_response(connfd, *data);
        return;
    }
    V(&mutex);

    strncpy(req_info.method, method, MAX_METHOD - 1);
    strncpy(req_info.version, version, MAX_VERSION - 1);
    req_info.method[MAX_METHOD - 1] = '\0';
    req_info.version[MAX_VERSION - 1] = '\0';

    int clientfd = open_clientfd_n(req_info.host, req_info.port);
    if (clientfd < 0) {
        fprintf(stderr, "Can't connect to host %s\n", req_info.host);
    }
    forward_request(clientfd, &rio, &req_info);

    handle_response(clientfd, connfd, &req_info);

    close(clientfd);
}

void forward_request(int clientfd,
                     rio_t *headers, 
                     const RequestInfo_t *request)
{
    char buf[MAXLINE];
    sprintf(buf, "%s %s %s\r\n", request->method, request->uri, "HTTP/1.0");
    sprintf(buf, "%sHost: %s\r\n", buf, request->host);
    sprintf(buf, "%s%s", buf, user_agent_hdr);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sProxy-Connection: close\r\n", buf);
    ssize_t buf_size = strlen(buf);
    
    char usrbuf[MAXLINE];
    ssize_t recv_n;
    while ((recv_n = rio_readlineb(headers, usrbuf, MAXLINE)) > 2) { // not \r\n
        if (!strncasecmp(usrbuf, "Host:", strlen("Host:"))
            || !strncasecmp(usrbuf, "User-Agent:", strlen("User-Agent:"))
            || !strncasecmp(usrbuf, "Connection:", strlen("Connection:"))
            || !strncasecmp(usrbuf, "Proxy-Connection:", strlen("Proxy-Connection:"))) {
            continue;
        }
        else {
            // here my proxy just truncate headers larger than 8k
            if (recv_n + buf_size > MAXLINE - 2) break;
            memcpy(buf + buf_size, usrbuf, recv_n);
            buf_size += recv_n;
        }
    }

    buf[buf_size] = '\r';
    buf[buf_size + 1] = '\n';
    rio_writen(clientfd, buf, buf_size + 2);
/*
 *   ssize_t recv_n;
 *   // body, if exist
 *   while ((recv_n = rio_readnb(headers, usrbuf, MAXLINE)) > 0) {
 *     rio_wirten(clientfd, usrbuf, recv_n);
 *   }
 */
}

void handle_response(int clientfd, 
                     int connfd, 
                     const RequestInfo_t *request)
{
    rio_t rio;
    char buf[MAXLINE];
    char usrbuf[MAXLINE];
    size_t content_length = -1;
    ssize_t buf_size;

    rio_readinitb(&rio, clientfd);
    if ((buf_size = rio_readlineb(&rio, buf, MAXLINE)) <= 0) {
        return;
    }
    printf("<-- %s", buf);
    ssize_t recv_n;

    while ((recv_n = rio_readlineb(&rio, usrbuf, MAXLINE)) > 2) {
        size_t lens = strlen("Content-Length:");
        if (strncasecmp(usrbuf, "Content-Length:", lens) == 0) {
            if (sscanf(usrbuf + lens, "%ld", &content_length) == 0) {
                fprintf(stderr, "malformed http response\n");
                return;
            }
        }
        // consume and truncate the header
        if (recv_n + buf_size > MAXLINE - 2) continue;
        memcpy(buf + buf_size, usrbuf, recv_n);
        buf_size += recv_n;
    }

    buf[buf_size] = '\r';
    buf[buf_size + 1] = '\n';
    rio_writen(connfd, buf, buf_size + 2);

    if (content_length == -1) { // no response body
        return;
    }

    // body
    char *body_buf = (char *)malloc(content_length);
    if (body_buf) {
        if ((recv_n = rio_readnb(&rio, body_buf, content_length)) < content_length) {
            fprintf(stderr, "warning: potential packet loss due to unknown reason\n");
        }
    }
    else {
        fprintf(stderr, "memory insufficiency or entity too large\n");
        return;
    }

    P(&mutex);
    insert_cache(&cache, request->url, buf, buf_size + 2, body_buf, recv_n);
    V(&mutex);
    
    rio_writen(connfd, body_buf, recv_n);
    free(body_buf);
}

int open_clientfd_n(char *host, int port)
{
    char p[16];
    sprintf(p, "%d", port);
    return open_clientfd(host, p);
}

void send_cached_response(int connfd, cache_content_t data)
{
    rio_writen(connfd, data.data, data.content_length);
}