/*************************************************************************
	> File Name: master.c
	> Author: 
	> Mail: 
	> Created Time: 2019年03月09日 星期六 18时10分04秒
 ************************************************************************/

#include "master.h"

char config[20] = "./conf_log";

int main(){
    int start[4];
    int finish[4];
    char start_ip[20] = {0};
    char finish_ip[20] = {0};
    char ins_s[5] = {0};
    char port_t[6] = {0};
    char port_m[6] = {0};
    int ins = 0; 
    char port_u[5] = {0};
    pthread_t print_t, heart_t, file_t, udp_t;
    
    get_conf_value(config, "Udp_Port", port_u);
    get_conf_value(config, "From", start_ip);
    get_conf_value(config, "To", finish_ip);
    get_conf_value(config, "INS", ins_s);
    get_conf_value(config, "Client_Port", port_t);
    get_conf_value(config, "Server_Port", port_m);
    ins = atoi(ins_s);

    int port = atoi(port_t);        //10004
    int port_M = atoi(port_m);      //8731
    int udp_port = atoi(port_u);

    transIp(start_ip, start);
    transIp(finish_ip, finish);
    
    int *sum = (int *)malloc(ins * sizeof(int));
    memset(sum, 0, ins * sizeof(int));
    LinkedList *list = (LinkedList *)malloc(ins * sizeof(LinkedList));

    struct sockaddr_in initaddr;
    initaddr.sin_family = AF_INET;
    initaddr.sin_port = htons(port);   //10004
    initaddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    for(int i = 0; i< ins; i++){
        Node *p = (Node *)malloc(sizeof(Node));
        p->client_addr = initaddr;
        p->next = NULL;
        list[i] = p;
    }
    
    char host[20] = {0};

    for(int i = start[2]; i <= finish[2]; i++){
        int j = 1;
        if(i == start[2]) j = start[3];
        for(; j <= 254; j++){
            
            if(i == finish[2] && j > finish[3]) break;
            
            sprintf(host, "%d.%d.%d.%d", start[0], start[1], i, j);
            
            initaddr.sin_addr.s_addr = inet_addr(host);
            Node *p = (Node *)malloc(sizeof(Node));
            p->client_addr = initaddr;
            p->next = NULL;
            int sub = find_min(sum, ins);
            insert(list[sub], p);
            sum[sub]++;
        }
    }    

    pthread_t t[ins];

    struct Print print_para[ins + 5];
    struct Heart heart_arg;
    heart_arg.head = list;
    heart_arg.ins = ins;
    heart_arg.sum = sum;
    
    if(pthread_create(&heart_t, NULL, heartbeat, (void *)&heart_arg) == -1){
        perror("pthread_create:");
        return -1;
    }    
    
    for(int i = 0; i < ins; i++){
        print_para[i].index = i;
        print_para[i].head = list[i];
        if(pthread_create(&t[i], NULL, print, (void *)&print_para[i]) == -1){
            printf("error in pthread_create\n");
            return -1;
        } 
    }

    int server_listen = sock_create(port_M);   //8731
    if(server_listen < 0){
        perror("sock create:");
        return 1;
    }
    
    int server_udp = udp_create(udp_port);
    if(server_udp < 0){
        perror("udp_port:");
        return 1;
    }
//    pthread_create(&udp_t, NULL, udp_buff, (void *)server_udp);

    pthread_create(&file_t, NULL, recv_file, (void *)&heart_arg);
    while (1) {
        int fd;
        struct sockaddr_in c_addr;
        socklen_t len = sizeof(c_addr);
        if((fd = accept(server_listen, (struct sockaddr *)&c_addr, &len)) < 0){
            perror("accept:");
            close(fd);
            continue;
        }
        
        int sub = find_min(sum, ins);
        Node *node = (Node *)malloc(sizeof(Node));
        node->client_addr = c_addr;
        node->client_addr.sin_port = htons(port);
        node->next = NULL;
        if(check(list, ins, c_addr) != -1){
            insert(list[sub], node);
            sum[sub]++;
            printf("insert ip %s\n", inet_ntoa(node->client_addr.sin_addr));
        }
    }

    for(int i = 0; i < ins; i++){
        pthread_join(t[i], NULL);
    }

    pthread_join(print_t, NULL);
    pthread_join(file_t, NULL);
    return 0;
}
