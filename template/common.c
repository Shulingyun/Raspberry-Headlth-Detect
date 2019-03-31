/*************************************************************************
	> File Name: common.c
	> Author: 
	> Mail: 
	> Created Time: 2019年02月23日 星期六 11时00分29秒
 ************************************************************************/

 #include"common.h"

 int sock_create(int port){
    int server_listen;
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if((server_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("create sock:");
        return -1;
    }

    int yes = 1;

    int ans = setsockopt(server_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if(ans < 0){  
        close(server_listen);
        perror("setsockopt");
        return -1;
    }


    if(bind(server_listen, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("bind:");
        close(server_listen);
        return -1;
    }
    if(listen(server_listen, 100) < 0){
        perror("listen:");
        close(server_listen);
        return -1;
    }

    return server_listen;
 }

int udp_create(int port){
    int sock_fd;
    struct sockaddr_in addr;

    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("sock create:");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    int yes = 1;

    int ans = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if(ans < 0){  
        close(sock_fd);
        perror("setsockopt");
        return -1;
    }

    if(bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind:");
        close(sock_fd);
        return -1;
    } 

    return sock_fd;
}
    
int sock_connect(int port, char *host){
    int sock_listen;
    struct sockaddr_in client_addr;
    
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(port);
    client_addr.sin_addr.s_addr = inet_addr(host);

    if((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("create socket:");
        return -1;
    }
    if(connect(sock_listen, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0){
        perror("connect:");
        close(sock_listen);
        return -1;
    }

    return sock_listen;
}

int connect_nonblock(int port, char *host){
    int sock_fd;
    struct sockaddr_in addr;
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0 )) < 0){
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    struct timeval tm;
    unsigned long ul = 1;
    ioctl(sock_fd, FIONBIO, &ul);
    fd_set set;
    int error = -1;
    int ret = -1;

    if(connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        tm.tv_sec = 0;
        tm.tv_usec = 300000;
        FD_ZERO(&set);
        FD_SET(sock_fd, &set);
        int len = sizeof(int);
        if(select(sock_fd + 1, NULL, &set, NULL, &tm) > 0) {
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


int get_conf_value(char *pathname, const char *key_name, char *value){
    FILE *fd = NULL;
    char *line = NULL;
    char *substr = NULL;
    ssize_t read = 0;
    size_t len = 0;

    fd = fopen(pathname, "r");

    if(fd == NULL){
        printf("error\n");
        exit(1);
    }
    
    while((read = getline(&line, &len, fd)) != -1){   //getline()返回字节数
//        printf("%s", line);
        substr = strstr(line, key_name);   //key_name是否为line的子串
        if(substr == NULL){
            continue;
        } else {
            int temp = strlen(key_name);
            if(line[temp] == '='){
                strncpy(value, &line[temp + 1], (int)read - temp - 1);
                temp = strlen(value);
                *(value + temp - 1) = '\0';
                break;
            }
            else{
                printf("Next\n");
                continue;
            }
        }
    }

/*    while((read = getline(&line, &len, fd)) != 1 ){
        printf("%s", line);
        char *temp = strtok(line, "=");
        if(strcmp(key_name,temp) == 0){
            strncpy(value, &line[strlen(key_name) + 1], (int)read - strlen(key_name) - 2);
          *(value + strlen(key_name) - 1) = '\0';
            break;
        } else {
            continue;
        } 
    }*/
    return 0;
}

int write_Pi_log (char *PiHealthLog, const char *format, ...){
    time_t timep = time(NULL);
    struct tm *t;
    FILE *fp;
    int ret;
    va_list arg;
    va_start(arg, format);

    t = localtime(&timep);
    fp = fopen(PiHealthLog, "a+");

    if(fp == NULL){
        printf("error!\n");
        exit(1);
    }
    
    int a = t->tm_year + 1900;
    int b = t->tm_mon + 1;
    int c = t->tm_mday;
    int d = t->tm_hour;
    int e = t->tm_min;
    int f = t->tm_sec;    

    fprintf(fp, "%04d-%02d-%02d %02d:%02d:%02d", a, b, c, d, e, f);
    ret = vfprintf(fp, format, arg);

    fflush(fp);
    fclose(fp);
    va_end(arg);

    return ret;
}
 
/*int mysql_connect(MYSQL *conn_ptr){
    conn_ptr = mysql_real_connect(conn_ptr, "localhost", "root", "rst138.", "pihealth", 0, NULL, 0);
    
    if(!conn_ptr){
        perror("connect mysql:\n");
        return -1;
    }

    return 0;
}*/

int get_time(char *cur_time){
    time_t timep = time(NULL);
    struct tm *t;
    t = localtime(&timep);
    
    int a = t->tm_year + 1900;
    int b = t->tm_mon + 1;
    int c = t->tm_mday;
    int d = t->tm_hour;
    int e = t->tm_min;
    int f = t->tm_sec;    

    sprintf(cur_time, "%04d-%02d-%02d %02d:%02d:%02d", a, b, c, d, e, f);

    return 0;
}
