/*************************************************************************
	> File Name: master.c
	> Author: 
	> Mail: 
	> Created Time: Sat 09 Mar 2019 18:09:47 CST
 ************************************************************************/

#include "master.h"


int main() {
    char *config = "/etc/pihealthd.conf";
    int start[4];
    int finish[4];
    char start_ip[20] = {0};
    char finish_ip[20] = {0};
    char port_t[6] = {0}; 
    char ins_s[5] = {0};
    int ins = 0, port;
    pthread_t print_t;

    get_conf_value(config, "INS", ins_s);
    get_conf_value(config, "From", start_ip);
    get_conf_value(config, "To", finish_ip);
    get_conf_value(config, "client_port", port_t);
        
    ins = atoi(ins_s);
    port = atoi(port_t);
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
    initaddr.sin_addr.s_addr = inet_addr("0.12.0.0");

    for (int i = 0; i < ins; i++) {
        Node *p;
        p = (Node *)malloc(sizeof(Node));
        p->client_addr = initaddr;
        p->next = NULL;
        linkedlist[i] = p;
    }
    
    printf("%s\n", inet_ntoa(linkedlist[0]->client_addr.sin_addr));
    char host[20] = {0};

    for (int i = start[3]; i <= finish[3]; i++) {
        
        sprintf(host, "%d.%d.%d.%d", start[0], start[1], start[2], i);
        initaddr.sin_addr.s_addr = inet_addr(host);
        Node *p;
        p = (Node *)malloc(sizeof(Node));
        p->client_addr = initaddr;
        p->next = NULL;
        int sub = find_min(sum, ins); //返回并发度中存储最小的链表
         
        insert(linkedlist[sub], p);
        sum[sub]++;
    }

    printf("before pthread_create\n");

    fflush(stdout);

    
    struct PRINT print_para[ins];
    pthread_t t[ins];
    for (int i = 0; i < ins; i++) {
        print_para[i].index = i;
        print_para[i].head = linkedlist[i];
        if (pthread_create(&t[i], NULL, print, (void *)&print_para[i]) == -1) {
            DBG("error in pthread_create!\n");
            return -1;
        }
    }
    for (int i = 0; i < ins; i++) {
        pthread_join(t[i], NULL);
    } 
    pthread_join(print_t, NULL);
    return 0;
}

