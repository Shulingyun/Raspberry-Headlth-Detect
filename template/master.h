/*************************************************************************
	> File Name: master.h
	> Author: 
	> Mail: 
	> Created Time: 2019年03月09日 星期六 18时09分17秒
 ************************************************************************/

#ifndef _MASTER_H
#define _MASTER_H

#include "common.h"

typedef struct Node{
    struct sockaddr_in client_addr;
    struct Node *next;
}Node, *LinkedList;

struct Sock{
    int fd;
    struct sockaddr_in addr;
    int num;
};

struct Print{
    LinkedList head;
    int index;
};

struct Heart{
    LinkedList *head;
    int ins;
    int *sum;
};

int transIp(char *sip, int *ip){
    if(sip == NULL) return -1;
    char temp[4];
    int count = 0;
    while(1){
        int index = 0;
        while(*sip != '\0' && *sip != '.' && count < 4){
            temp[index++] = *sip;
            sip++;
        }
        if(index == 4) return -1;
        temp[index] = '\0';
        ip[count] = atoi(temp);
        count++;
        if(*sip == '\0'){
            if(count == 4) return -1;
        } else {
            sip++;
        }
    }
    return 1;
}

int insert(LinkedList head, Node *node){
    Node *p = head;
    while(p->next != NULL){
        p = p->next;
    }
    p->next = node;
    return 0;
}

int find_min(int *sum, int ins){
    int ret = 0;
    for(int i = 0; i < ins; i++){
        if(*(sum + i) < *(sum + ret))  ret = i;
    }
    return ret;
}

void *print(void *arg) {
    struct Print *print_para = (struct Print *)arg;
    LinkedList linkedlist = print_para->head;
    int ind = print_para->index;
    char filename[50] = {0};

    sprintf(filename, "./%d.log", ind);
    while (1) {
        FILE *file = fopen(filename, "w");
        Node *p = print_para->head;
        while(p->next != NULL){
            fprintf(file, "%s: %d\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));;
//           printf("%s: %d\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));;
            p = p->next;
        }
        fflush(file);
        fclose(file);
        sleep(1);
    }
    return NULL;
}

int connect_sock_2(struct sockaddr_in addr){
    int sock_fd;
    
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }

    struct timeval tm;
    unsigned long ul = 1;
    ioctl(sock_fd, FIONBIO, &ul);
    int error = -1;
    int ret = -1;
    fd_set set;

    if(connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        tm.tv_sec = 0;
        tm.tv_usec = 300000;
        FD_ZERO(&set);
        FD_SET(sock_fd, &set);
        int len = sizeof(int);
        if(select(sock_fd + 1, NULL, &set, NULL, &tm) > 0){
            getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
            if(error == 0){
                ret = 1;
            } else {
                ret = -1;
            }
        } else {
            ret = -1;
        }
    } else {
        ret = 1;
    }
    
    close(sock_fd);
    return ret;
}

int connect_sock(struct sockaddr_in addr, fd_set *set, struct Sock socks[], int *j, int *maxfd, int i){
    int sock_fd;
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }
    
    unsigned long ul = 1;
    ioctl(sock_fd, FIONBIO, &ul);

    int error = -1;
    int ret = -1;

    socks[*j].fd = sock_fd;
    socks[*j].addr = addr;
    socks[*j].num = i;

    *j = *j + 1;
    for(int i = 0; i < *j; i++){
        if(socks[i].fd > *maxfd) *maxfd = socks[i].fd;
    }
    
    if(connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        
        return -1;
    }
    
    return 0;
}

int check(LinkedList *head, int ins, struct sockaddr_in addr){
    for(int i = 0; i < ins; i++){
        Node *p = head[i];
        while(p->next != NULL){
            if(p->next->client_addr.sin_addr.s_addr == addr.sin_addr.s_addr) {
                return -1;
            }
            p = p->next;
        }
    }
    return 0;
}

void deletes(struct sockaddr_in addr, int i, LinkedList *lists){
    Node *p = lists[i];
    Node *q = p->next;
    char temp[20] = {0};
    strcpy(temp, inet_ntoa(addr.sin_addr));
    while(q != NULL){
        char ip[20] = {0};
        strcpy(ip, inet_ntoa(q->client_addr.sin_addr));
        if(strcmp(ip, temp) == 0){
            p->next = q->next;
            free(q);
            return ;
        }
        p = p->next;
        q = q->next; 
    }
    return ;
}

void *heartbeat(void *arg){
    struct Heart *heart;
    heart = (struct Heart *)arg;
    
    while(1){
        printf("hearting**************\n");
        struct Sock socks[1000];
        int j = 0;
        int maxfd;
        fd_set set;
        FD_ZERO(&set);
        for(int i = 0; i < heart->ins; i++){
            Node *p = heart->head[i];
            while(p != NULL && p->next != NULL){
/*                char ip[20] = {0};
                strcpy(ip, inet_ntoa(p->next->client_addr.sin_addr));
                if(connect_sock_2(p->next->client_addr) < 0){
                    Node *q = p->next;
                    printf("deleting ip %s\n", ip);
                    p->next = q->next;
                    free(q);
                    heart->sum[i]--;
                } else {
                    printf("%s: %d online\n", ip, ntohs(p->next->client_addr.sin_port));
                    p = p->next;
                }*/
                if(connect_sock(p->next->client_addr, &set, socks, &j, &maxfd, i) < 0){
                    FD_SET(socks[j - 1].fd, &set);
                }
                p = p->next;
            }       
        }
        struct timeval tm;
        tm.tv_sec = 1;
        tm.tv_usec = 0;
        int rc = select(maxfd + 1, NULL, &set, NULL, &tm);
        printf("rc = %d\n", rc);
        for(int i = 0; i < j; i++){
            if(rc > 0 && FD_ISSET(socks[i].fd, &set)){
                int error = -1;
                int len = sizeof(int);
                getsockopt(socks[i].fd, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
                if(error == 0){
                    printf("\033[32m%s: %d online\033[0m\n", inet_ntoa(socks[i].addr.sin_addr), ntohs(socks[i].addr.sin_port));
                    rc--;
                } else {
                    printf("ip %s is deleting\n", inet_ntoa(socks[i].addr.sin_addr));
                    deletes(socks[i].addr, socks[i].num, heart->head);
                }
            } else {
                printf("ip %s is deleting\n", inet_ntoa(socks[i].addr.sin_addr));
                deletes(socks[i].addr, socks[i].num, heart->head);
            }
            close(socks[i].fd);
        }
        sleep(5);
    }
    return NULL;
}

void find_file(int i, char *files, char *ip){
    char config[20] = "./conf_log";
    switch(i){
        case 100 : sprintf(files, "%s/cpu.log", ip); break;
        case 101 : sprintf(files, "%s/mem.log", ip); break;
        case 102 : sprintf(files, "%s/disk.log", ip); break;
        case 103 : sprintf(files, "%s/pro.log", ip); break;
        case 104 : sprintf(files, "%s/sys.log", ip); break;
        case 105 : sprintf(files, "%s/user.log", ip); break;
    }
    return ;
}

void *recv_file(void *arg){
    struct Heart *file;
    file = (struct Heart *)arg;
    
    char temp_ctrl[5] = {0};
    char temp_msg[5] = {0};
    char server_ip[20] = {0};
    char config[20] = "./conf_log";
    get_conf_value(config, "Server_Ip", server_ip);
    get_conf_value(config, "Ctrl_Port", temp_ctrl);
    get_conf_value(config, "Msg_Port", temp_msg);
    int msg_port = atoi(temp_msg);
    int ctrl_port = atoi(temp_ctrl);
    
    for(int i = 0; i < file->ins; i++){
        Node *p = file->head[i];
        int k = 0;
        while(p != NULL && p->next != NULL){
            char ip[20] = {0};
            printf("%d\n", k++);
            strcpy(ip, inet_ntoa(p->next->client_addr.sin_addr));
            printf("sock_ctrl ip is %s\n", ip);
            int sock_ctrl;
            if((sock_ctrl = sock_connect(ctrl_port, ip)) < 0){
                printf("error in sock_ctrl connect!\n");
                p = p->next;
                continue;
            }
            mkdir(ip, 0777);    
            printf("connect success ! begin send file\n");
            for(int i = 100; i <= 105; i++){
                send(sock_ctrl, &i, sizeof(i), 0);    //100
                int temp = 0;
                recv(sock_ctrl, &temp, sizeof(temp), 0);   // 200
                printf("temp = %d\n", temp);
                if(temp == i + 100){
                    int msg_fd;
                    if((msg_fd = sock_connect(msg_port, server_ip)) < 0){
                        printf("msg_fd connect error\n");
                        continue;
                    }
                    char buffer[9128] = {0};
                    recv(msg_fd, buffer, sizeof(buffer), 0);
                    char files[20] = {0};
                    find_file(i, files, ip);
                    printf("file : %s\n", files);
                    FILE *fp = fopen(files, "w");
                    fwrite(buffer, 1, sizeof(buffer), fp);
                    printf("recv success!\n");
                } else {
                    continue;
                }
            }
            p = p->next;
        }
    }
}

/*void *udp_buff(void *arg){
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int sock_fd = (int)arg;
    MYSQL *conn_ptr;

    mysql_connect(conn_ptr);

    while(1){
        char buffer[200] = {0};
        recvfrom(sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)addr, &len);
        char ip[20] = {0};
        strcpy(ip, inet_ntoa(addr.sin_addr));
        char cur_time[20] = {0};
        get_time(cur_time);
        int type;
        char details[256] = {0};
        int j = 0;
        for(int i = 0; buffer[i]; i++){
            if(buffer[i] >= '0' && buffer[i] <= '9'){
                type = type * 10 + (buffer[i] - '0');
                continue;
            } 
            if(buffer[i] != ' '){
                details[j++] = buffer[i];
            }
        }
        details[j] = '\0';
        char sql_state[100];
        sprintf(sql_state, "INSERT INTO waring_events VALUES('%s','%s','%d','%s')", cur_time, ip, type, details);
        if(mysql_query(conn_ptr, sql_state) != 0){
            printf("insert failed\n");
            continue;
        }
    } 
}*/


#endif
