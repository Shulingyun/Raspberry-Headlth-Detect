/*************************************************************************
	> File Name: master.c
	> Author: 
	> Mail: 
	> Created Time: Sat 09 Mar 2019 18:09:47 CST
 ************************************************************************/

#include "master.h"

int main() {
    char *config = "PiHealth.conf";
    int start[4];
    int finish[4];
    char start_ip[20] = {0};
    char finish_ip[20] = {0};
    char port_t[6] = {0}; 
    char port_m[6] = {0};
    char port_u[6] = {0};
    char ins_s[5] = {0};
    int ins = 0, port, port_M, port_Udp;
    pthread_t print_t, heart_t, file_t;

    get_conf_value(config, "INS", ins_s);
    get_conf_value(config, "From", start_ip);
    get_conf_value(config, "To", finish_ip);
    get_conf_value(config, "Client_Port", port_t);   
    get_conf_value(config, "Master_Port", port_m);
    get_conf_value(config, "Udp_Port", port_u);

    ins = atoi(ins_s);
    port = atoi(port_t);
    port_M = atoi(port_m);
    port_Udp = atoi(port_u);

    transip(start_ip, start);
    transip(finish_ip, finish);

    int *sum = (int *)malloc(ins * sizeof(int));
    memset(sum, 0, ins *sizeof(int));

    DBG("start = %d.%d.%d.%d\n", start[0], start[1], start[2], start[3]);
    DBG("finish = %d.%d.%d.%d\n", finish[0], finish[1], finish[2], finish[3]);

    LinkedList *linkedlist = (LinkedList *)malloc(ins * sizeof(LinkedList));

    struct sockaddr_in initaddr;
    initaddr.sin_family = AF_INET;
    initaddr.sin_port = htons(port);
    initaddr.sin_addr.s_addr = inet_addr("0.0.0.0");//定义链表头指针

    for (int i = 0; i < ins; i++) {//初始化五个链表的头指针
        Node *p;
        p = (Node *)malloc(sizeof(Node));
        p->client_addr = initaddr;
        p->next = NULL;
        linkedlist[i] = p;
    }
    
    DBG("init[0]-->>%s\n", inet_ntoa(linkedlist[0]->client_addr.sin_addr));
    DBG("init[1]-->>%s\n", inet_ntoa(linkedlist[1]->client_addr.sin_addr));
    DBG("init[2]-->>%s\n", inet_ntoa(linkedlist[2]->client_addr.sin_addr));
    DBG("init[3]-->>%s\n", inet_ntoa(linkedlist[3]->client_addr.sin_addr));
    DBG("init[4]-->>%s\n", inet_ntoa(linkedlist[4]->client_addr.sin_addr));
    
    char host[20] = {0};

    for (int j = start[2]; j <= finish[2]; j++) {//并发初始化所有链表
        for (int i = start[3]; i <= finish[3]; i++) {
            sprintf(host, "%d.%d.%d.%d", start[0], start[1], j, i);
            initaddr.sin_addr.s_addr = inet_addr(host);
            Node *p;
            p = (Node *)malloc(sizeof(Node));
            p->client_addr = initaddr;
            p->next = NULL;
            int sub = find_min(sum, ins); //返回并发度中存储最小的链表
             
            insert(linkedlist[sub], p);//将当前ip插入到最短的链表中
            sum[sub]++;//该链表长度+1
        }
    }

    printf("before pthread_create\n");

    fflush(stdout);

    struct HEART heart_arg;
    heart_arg.head = linkedlist;
    heart_arg.ins = ins;
    heart_arg.sum = sum;

    if (pthread_create(&heart_t, NULL, heartbeat, (void *)&heart_arg) == -1) {//心跳线程-断线重连建立
        DBG("error in pthread_create for heartbeat\n");
        return -1;
    }

    struct PRINT print_para[ins];
    pthread_t t[ins];//建立五个线程不断刷新链表并打印出来

    for (int i = 0; i < ins; i++) {
        print_para[i].index = i;
        print_para[i].head = linkedlist[i];
        if (pthread_create(&t[i], NULL, print, (void *)&print_para[i]) == -1) {
            DBG("error in pthread_create!:print\n");
            return -1;
        }
    }

    int server_listen;//建立监听状态的套接字、监听端口
    if ((server_listen = socket_create(port_M)) < 0) {
        DBG("%s\n", strerror(errno));
        close(server_listen);
        return 1;
    }

    int server_udp = udp_create(port_Udp);
    if (server_udp < 0) {
        perror("udp_port:");
        return 1;
    }
    
    /*if (pthread_create(&file_t, NULL, recv_file, (void *)&heart_arg) == -1) {//读取发来的文件
        DBG("error in pthread_create!:recv_file\n");
        return -1;
    }*/

    while (1) {
        int fd;
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);//接受创建链接
        if ((fd = accept(server_listen, (struct sockaddr *)&client_addr, &len)) < 0) {
            DBG("%s\n", strerror(errno));
            close(fd);
            continue;
        }

        Node *p;
        p = (Node *)malloc(sizeof(Node));
        p->client_addr = client_addr;
        p->client_addr.sin_port = htons(port);
        p->next = NULL;
        
        printf("%s:%d \033[36mlog in\033[0m\n", inet_ntoa(p->client_addr.sin_addr), ntohs(p->client_addr.sin_port));

        if (check(linkedlist, client_addr, ins) != -1) {//查重登录ip地址
            int sub = find_min(sum, ins);
            insert(linkedlist[sub], p);
            sum[sub]++;
            DBG("%s:%d insert 2\n", inet_ntoa(p->client_addr.sin_addr), ntohs(p->client_addr.sin_port));
        }
    }

    for (int i = 0; i < ins; i++) {
        pthread_join(t[i], NULL);
    } 

    pthread_join(heart_t, NULL);
    pthread_join(print_t, NULL);
    pthread_join(file_t, NULL);
    return 0;
}

