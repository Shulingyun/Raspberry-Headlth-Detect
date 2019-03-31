/*************************************************************************
	> File Name: common1.c
	> Author: 
	> Mail: 
	> Created Time: Sat 23 Feb 2019 11:29:45 CST
 ************************************************************************/

#include "common1.h"

int socket_create(int port) {
    int sockfd;
    struct sockaddr_in addr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 20) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    
    return sockfd;
}


int socket_connect(int port, char *host) {
    int sockfd;
    struct sockaddr_in server;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create");
        close(sockfd);
        return -1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(host);

    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) <0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    
    return sockfd; 
}

int get_conf_value(char *pathname, char* key_name, char *value) {
    FILE *fd = NULL;
    char *line = NULL;
    char *substr = NULL;
    ssize_t read = 0;
    size_t len = 0;
    int tmp = 0;


    fd = fopen(pathname, "r");
    
    if (fd == NULL) {
        printf("Error in Open\n");
        exit(1);
    }
    
    while ((read = getline(&line, &len, fd)) != 1) {
        //DBG("%s", line);
        fflush(stdout);
        substr = strstr(line, key_name);
        if (substr == NULL) {
            continue;
        } else {
            tmp = strlen(key_name);
            if (line[tmp] == '=') {
                //DBG("tmp = %d\n", tmp);
                //DBG("read = %d\n", read);

                strncpy(value, &line[tmp + 1], (int)read - tmp - 1);
                tmp = strlen(value);
                *(value + tmp - 1) = '\0';
                break;
            }
            else {
                printf("Next\n");
                continue;
            }
        }

    }
}

int udp_create(int port) {
    int sock_fd;
    struct sockaddr_in addr;

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("sock create:");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    int value = 1;
    int ans = 0;
    if ((ans = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) < 0)) {
        close(sock_fd);
        perror("setsockpot");
        return -1;
    }
    if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind:");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

/*bool connect_nonblock(int port, char *host, long timeout) {
    int sockfd;
    struct sockaddr_in dest_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() error");
        return false;
    } 

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr.s_addr = inet_addr(host);

    DBG("connect TO %s:%d\n", host, port);

    int error = -1, len;
    len = sizeof(int);
    struct timeval tm;
    fd_set set;
    unsigned long ul = 1;
    ioctl(sockfd, FIONBIO, &ul);

    bool ret = false;
    if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        tm.tv_sec = 0;
        tm.tv_usec = timeout;
        FD_ZERO(&set);
        FD_SET(sockfd, &set);
        if (select(sockfd + 1, NULL, &set, NULL, &tm) > 0) {
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
            if (error == 0) {
                ret = true;
            }
            else ret = false;
        }
        else ret = false;
    }
    close(sockfd);
    return ret;

}*/
