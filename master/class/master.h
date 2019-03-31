/*************************************************************************
	> File Name: master.h
	> Author: 
	> Mail: 
	> Created Time: Sat 09 Mar 2019 18:08:47 CST
 ************************************************************************/
#include "common1.h"

typedef struct Node{
    struct sockaddr_in client_addr;
    int node_sock;
    struct Node *next;
}Node, *LinkedList;


struct PRINT{
    LinkedList head;
    int index;
}; 

struct HEART{
    LinkedList *head;
    int ins;
    int *sum;
    long timeout;
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
        DBG("insert %s\n", inet_ntoa(p->next->client_addr.sin_addr));
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
        FILE *file = fopen(filename, "w");//a+追加文件尾--w覆盖
        Node *p = print_para->head;
        fprintf(file, "%d\n", temp++);
        while (p -> next != NULL) {
            //printf("%s:%d\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
            fprintf(file, "%s:%d\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
            p = p->next;
        }
        fclose(file);
        sleep(1);//1
    }

    return NULL;
}

int connect_sock(struct sockaddr_in addr) {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DBG("%s", strerror(errno));
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


int check(LinkedList *head, struct sockaddr_in addr, int ins) {
    for (int i = 0; i < ins; i++) {
        Node *p;
        p = head[i];
        while (p->next != NULL) {
            if (addr.sin_addr.s_addr == p->next->client_addr.sin_addr.s_addr) {
                DBG("%s \033[032min list\033[0m\n", inet_ntoa(addr.sin_addr));
                return -1;
            }
            p = p->next;
        }
    }
    return 0;
}

void *heartbeat(void *arg) {
    struct HEART *heart;
    heart = (struct HEART *)arg;
    struct timeval tm;
    tm.tv_sec = 0;
    tm.tv_usec = heart->timeout;
    int times = 0;
    while (1) {
        //sleep(10);
        DBG("times = %d\n", times++);
        fflush(stdout);
        for (int i = 0; i < heart->ins; i++) {
            Node *p;
            p = heart->head[i];
            unsigned long ul = 1;
            fd_set wset;
            FD_ZERO(&wset);
            int max = 0;
            while (p != NULL && p->next != NULL) {
                //printf("sum in %d is %d\n", i, heart->sum[i]);
                int sockfd = -1;
                if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    DBG("%s\n", strerror(errno));
                }
                ioctl(sockfd, FIONBIO, &ul);
                connect(sockfd, (struct sockaddr *)&(p->next->client_addr), sizeof(p->next->client_addr));
                FD_SET(sockfd, &wset);
                if (max < sockfd) max = sockfd;
                //max = sockfd;
                p->next->node_sock = sockfd;
                p = p->next;
            }
            usleep(300000);
            int ret_select = select(max + 1, NULL, &wset, NULL, &tm);
            if (ret_select < 0) {
                perror("error");
            }
            p = heart->head[i];
            if (ret_select > 0) {
                DBG("select > 0  ret_select = %d\n", ret_select);
                while (p != NULL && p->next != NULL) {
                    if (FD_ISSET(p->next->node_sock, &wset)) {
                        int error = -1, len;
                        len = sizeof(int);
                        getsockopt(p->next->node_sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
                        if (error == 0) {
                            //struct sockaddr_in temp;
                            //int t_len = sizeof(temp);
                            FD_CLR(p->next->node_sock, &wset);
                            close(p->next->node_sock);
                            DBG("%s:%d \033[032monline\033[0m\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
                            p = p->next;
                        } else {
                            FD_CLR(p->next->node_sock, &wset);
                            close(p->next->node_sock);
                            //p->next->node_sock = -1;
                            DBG("%s:%d \033[31mdelet...1\033[0m\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
                            Node *temp;
                            temp = p->next;
                            p->next = p->next->next;
                            free(temp);
                            heart->sum[i]--;
                        }
                    } else {
                        FD_CLR(p->next->node_sock, &wset);
                        close(p->next->node_sock);
                        p->next->node_sock = -1;
                        DBG("%s:%d \033[31mdelet...2\033[0m\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
                        Node *temp;
                        temp = p->next;
                        p->next = p->next->next;
                        free(temp);
                        heart->sum[i]--;
                    } 
                }
            } else {
                DBG("select !> 0\n");
                while (p != NULL && p->next != NULL) {
                    FD_CLR(p->next->node_sock, &wset);
                    close(p->next->node_sock);
                    DBG("%s:%d \033[31mdelet...3\033[0m\n", inet_ntoa(p->next->client_addr.sin_addr), ntohs(p->next->client_addr.sin_port));
                    Node *temp;
                    temp = p->next;
                    p->next = p->next->next;
                    free(temp);
                    heart->sum[i]--;
                }
            }
            fflush(stdout);
        }
        sleep(10);
    }
    return NULL;
}

void find_file(int i, char *files, char *ip) {
    char config[20] = "./PiHealth.conf";
    switch (i) {
        case 100: sprintf(files, "%s/cpu.log", ip);break;
        case 101: sprintf(files, "%s/mem.log", ip);break;
        case 102: sprintf(files, "%s/disk.log", ip);break;
        case 103: sprintf(files, "%s/pro.log", ip);break;
        case 104: sprintf(files, "%s/sys.log", ip);break;
        case 106: sprintf(files, "%s/user.log", ip);break;
    }
    return ;
}

void *recv_file(void *arg) {
    struct HEART *file;
    file = (struct HEART *)arg;
    
    char temp_ctrl[5] = {0};
    char temp_msg[5] = {0};
    char server_ip[5] = {0};
    char config[20] = "./PiHealth.conf";
    get_conf_value(config, "Ctrl_Port", temp_ctrl);
    get_conf_value(config, "Msg_Port", temp_msg);
    get_conf_value(config, "Server_Ip", server_ip);

    int msg_port = atoi(temp_msg);
    int ctrl_port = atoi(temp_ctrl);
    for (int i = 0; i < file->ins; i++) {
        Node *p = file->head[i];
        int k = 0;
        while (p != NULL && p->next != NULL) {
            char ip[20] = {0};
            DBG("%d\n", k++);
            strcpy(ip, inet_ntoa(p->next->client_addr.sin_addr));
            DBG("sock_ctrl %s:%d\n", ip, ctrl_port);
            //DBG("ctrl_port:%d\n", ctrl_port);
            int sock_ctrl;
            if ((sock_ctrl = socket_connect(ctrl_port, ip)) < 0) {
                DBG("error in sock_ctrl connent!\n");
                p = p->next;
                continue;
            }
            mkdir(ip, 0777);
            DBG("connet success ! begin send file\n");
            for (int i = 100; i <= 105; i++) {
                send(sock_ctrl, &i, sizeof(i), 0);   //100
                int temp = 0;
                recv(sock_ctrl, &temp, sizeof(temp), 0);  //200
                DBG("temp = %d\n", temp);
                if (temp == i + 100) { //在线发信息
                    int msg_fd;
                        if ((msg_fd = socket_connect(msg_port, server_ip)) < 0) {
                            DBG("msg_fd connent error\n");
                            continue;             
                        }
                        char buffer[10000] = {0};
                        recv(msg_fd, buffer, sizeof(buffer), 0);
                        char files[20] = {0};
                        find_file(i, files, ip);
                        FILE *fp = fopen(files, "w");
                        fwrite(buffer, 1, sizeof(buffer), fp);
                        DBG("recv success!\n");
                } else {
                        continue;                     
                }
            }
            p = p->next;
        }
    }
}

/*bool check_connect(struct sockaddr_in addr, long timeout) {
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
}*/


