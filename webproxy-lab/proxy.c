#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void parse_uri(char *uri, char *hostname, char *port, char *query);
void read_requesthdrs(rio_t *rp, char *headers, char *hostname);
void send_request(int serverfd, char *method, char *query, char *headers);

void sigchld_handler(int sig) {
    while (waitpid(-1, 0, WNOHANG) > 0)
        ;
    return;
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    Signal(SIGCHLD, sigchld_handler);
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s %s)\n", hostname, port);

        if (Fork() == 0) {
            Close(listenfd);
            doit(connfd);
            Close(connfd);
            exit(0);
        }
        Close(connfd);
    }
}

void doit(int clientfd) {
    int serverfd;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char headers[MAXBUF];
    char hostname[MAXLINE], port[MAXLINE], query[MAXLINE];
    rio_t client_rio, server_rio;

    Rio_readinitb(&client_rio, clientfd);
    Rio_readlineb(&client_rio, buf, MAXLINE);
    printf("Request headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    parse_uri(uri, hostname, port, query);
    read_requesthdrs(&client_rio, headers, hostname);
    
    
    serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0) return;

    send_request(serverfd, method, query, headers);
    Rio_readinitb(&server_rio, serverfd);

    int n;
    while ((n = Rio_readnb(&server_rio, buf, MAXLINE)) > 0) {
        Rio_writen(clientfd, buf, n);
    }

    Close(serverfd);
}

void read_requesthdrs(rio_t *rp, char *hdrs, char *hostname) {
    char buf[MAXLINE];
    int has_host = 0;

    hdrs[0] = '\0';
    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        printf("%s", buf);
        
        if (strncasecmp(buf, "Host:", 5) == 0) {
            has_host = 1;
            strcat(hdrs, buf);    
        } else if (strncasecmp(buf, "User-Agent:", 11) == 0 || 
                   strncasecmp(buf, "Connection:", 11) == 0 ||
                   strncasecmp(buf, "Proxy-Connection:", 17) == 0) {
            ;
        } else {
            strcat(hdrs, buf);
        }
        
        Rio_readlineb(rp, buf, MAXLINE);
    }

    if (!has_host) {
        strcat(hdrs, "Host: ");
        strcat(hdrs, hostname);
        strcat(hdrs, "\r\n");
    }
    strcat(hdrs, user_agent_hdr);
    strcat(hdrs, "Connection: close\r\n");
    strcat(hdrs, "Proxy-Connection: close\r\n");
}

void parse_uri(char *uri, char *hostname, char *port, char *query) {
    char *host_start, *port_start, *query_start;

    host_start = strstr(uri, "://");
    host_start = host_start ? host_start + 3 : uri;

    query_start = strchr(host_start, '/');
    if (query_start) {
        *query_start = '\0';
        query_start += 1;
    }

    port_start = strchr(host_start, ':');
    if (port_start) {
        *port_start = '\0';
        port_start += 1;
    }

    strcpy(hostname, host_start);
    
    strcpy(port, port_start ? port_start : "80");

    strcpy(query, "/");
    if (query_start) {
        strcat(query, query_start);
    }
}

void send_request(int serverfd, char *method, char *query, char *headers) {
    char buf[MAXBUF];

    sprintf(buf, "%s %s HTTP/1.0\r\n", method, query);
    Rio_writen(serverfd, buf, strlen(buf));
    Rio_writen(serverfd, headers, strlen(headers));
    Rio_writen(serverfd, "\r\n", 2);
}
