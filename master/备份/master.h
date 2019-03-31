/*************************************************************************
	> File Name: master.h
	> Author: 
	> Mail: 
	> Created Time: Sat 09 Mar 2019 18:08:47 CST
 ************************************************************************/
#include "common1.h"



typedef struct Node{
    struct sockaddr_in client_addr;
    struct Node *next;
}Node, *LinkedList;


struct PRINT{
    LinkedList head;
    int index;
}; 

struct HEART{
    LinkedList head;
    int ins;
};

int transip(char *sip, int *ip) {
    if(sip == NULL) return -1;
    char temp[4];
    int count = 0;
    while (1) {
        int index = 0;
        while(*sip != '\0' && *sip != '.' && count < 4) {
            temp[index++] = *sip;
            sip++;
        }
        if (index == 4) return -1;
        temp[index] = '\0';
        ip[count] = atoi(temp);
        printf("ip[%d] = %d\n",count, ip[count]);
        count++;
        if (*sip == '\0') {
            if (count == 4) return 0;
        } else {
            sip++;
        }
    }
    printf("\n");
    return 0;
}


int insert(LinkedList head, Node *node) {
    Node *p;
    p = head;
    while (p->next != NULL) {
       // printf("insert %s\n", inet_ntoa(p->next->client_addr.sin_addr));
        p = p->next;

    }
    p->next = node;
    return 1;
}


int find_min(int *sum, int ins) {
    int ans = 0;
    for (int i = 0; i < ins; i++) {
        if (*(sum +i) < *(sum + ans)) {
            ans = i;
        }
    }
    return ans;
}


void *print(void *arg) {
    struct PRINT *print_para = (struct PRINT *)arg;
    int index = print_para->index;
    printf("index = %d, %d\n", index, print_para->index);
    char filename[50] = {0};
    sprintf(filename, "./%d.log", index);
    int temp = 0;
    while (1) {
        FILE *file = fopen(filename, "w");
        Node *p = print_para->head;
        fprintf(file, "%d\n", temp++);
        while (p -> next != NULL) {
            //printf("%s:%d\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
            fprintf(file, "%s:%d\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
            p = p->next;
        }
        fclose(file);
        sleep(1);
    }

    return NULL;
}

int connent_sock(struct sockaddr_in addr) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DBG("%s", strerror(errorno));
        return 1;
    }
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        DBG("%s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    close(sockfd);
    return 0;
}


void *heartbeat(void *arg) {
    struct HEART *heart;
    heart = (struct HEART *)arg;

    for (int i = 0; i < heart->ins; i++) {
        Node *p;
        p = heart->head + i;
        while (p != NULL && p->next != NULL) {
            //char ip[20] = {0};
            //strcpy(ip, inet_ntoa(p->next->client_addr.sin_addr));
            if (connect_sock(p->next->client_addr) < 0) {
                Node *temp;
                temp = p->next;
                p->next = p->next->next;
                free(temp);
            } else {
                DBG("%s:%d online\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
            }
            p = p->next;
        }
    }
    
    return NULL;
}

bool check_connect(struct sockaddr_in addr, long timeout) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DBG("%s\n", strerror(errno));
        return false;
    }
    int error = -1, len;
    len = sizeof(int);
    struct timeval tm;
    fd_set set;
    unsigned long ul = 1;
    ioctl(sockfd, FIONBIO, &ul);

    bool ret = false;
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        tm.tv_sec = 0;
        tm.tv_usec = timeout;
        FD_ZERO(&set);
        FD_SET(sockfd, &set);
        if (select(sockfd + 1, NULL, &set, NULL, &tm) > 0) {
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
            if (error == 0) {
                ret = true;
            } else 
        }
    }
}


