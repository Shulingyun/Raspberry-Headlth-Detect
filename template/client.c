/*************************************************************************
	> File Name: client.c
	> Author: 
	> Mail: 
	> Created Time: 2019年03月16日 星期六 14时11分53秒
 ************************************************************************/

#include "common.h"

struct sm_msg{
    int flag;                     //检测次数
    int sm_time;                  
    pthread_mutex_t sm_mutex;     //互斥锁
    pthread_cond_t sm_ready;      //条件变量
};

char *config = "./conf_log";
char *shared_memory = NULL;
char log_dir[50] = {0};
char log_backup[50] = {0};
double dynamic;     //memlog脚本的动态平均值

struct sm_msg *msg;
pthread_mutexattr_t m_attr;
pthread_condattr_t c_attr;

int client_heart(char *ip, int port){
    int ret = 1;
    int fd;
    if((fd = sock_connect(port, ip)) < 0){
        ret = -1;
    }
    close(fd);
    return ret;
}

int sign_sys = 0; //标识系统是否需要自测

int file_size(char *filename){
    struct stat statbuf;
    stat(filename, &statbuf);
    int size = statbuf.st_size;

    return size;
}

int compress_file(char *infilename, char *outfilename){
    int num_read = 0;
    FILE *infile = fopen(infilename, "rb");
    gzFile outfile = gzopen(outfilename, "wb");
    char inbuffer[128] = {0};
    unsigned long total_read = 0;

    while((num_read = fread(inbuffer, 1, sizeof(inbuffer), infile)) > 0){
        total_read += num_read;
        gzwrite(outfile, inbuffer, num_read);
        memset(inbuffer, 0, sizeof(inbuffer));
    }
    fclose(infile);
    gzclose(outfile);
    
    printf("Read %lu bytes, Write %d bytes\n", total_read, file_size(outfilename));
    
    return 0;
}

int decompress_file(char *infilename, char *outfilename){
    int num_read = 0;
    char buffer[128] = {0};
    int total_num = 0;
    gzFile infile = gzopen(infilename, "rb");
    FILE *outfile = fopen(outfilename, "w");
    
    while((num_read = gzread(infile, buffer, sizeof(buffer))) > 0){
        fwrite(buffer, 1, num_read, outfile);
        memset(buffer, 0, sizeof(buffer));
    }
    
    gzclose(infile);
    fclose(outfile);
    return 0;
}

/*int detect(char *path, int i){
    FILE *fp = fopen(path, "r");
    char buffer[200] = {0};
    while(fgets(buffer, sizeof(buffer), fp)){
        char temp[20] = {0};
        int j = 0;
        //还需添加异常类型
        for(int i = 0; buffer[i]; i++){
            if(buffer[i] >= 'a' && buffer[i] <= 'z' && j < 10){
                temp[j++] = buffer[i];
            } else {
                continue;
            }
        }
        if(strcmp(temp, "warning") != 0) continue;
        char port_u[5] = {0};
        char server_ip[20] = {0};
        get_conf_value(config, "Server_Ip", server_ip);
        get_conf_value(config, "Udp_Port", port_u);
        int port_udp = atoi(port_u);
        int sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port_udp);
        addr.sin_addr.s_addr = inet_addr(server_ip);
        sendto(sock_udp, temp, sizeof(temp), 0, (struct sockaddr *)&addr, sizeof(addr));
    }
}*/

void sys_detect(int type){
    int time_i = 5;
    char src[50] = {0};
    char run[50] = {0};
    sprintf(src, "Src%d", type);
    get_conf_value(config, src, src);
    FILE *fstream = NULL;
    char buffer[4096] = {0};
    char logname[50] = {0};
    int times = 0;
    char temp[4] = {0};
    char path[20] = {0};
    char inter[5] = {0};
    char fsize[10] = {0};

    get_conf_value(config, "File_Max", fsize);
    get_conf_value(config, "Backfile", path);
    get_conf_value(config, "WriteEveryTime", temp);
    get_conf_value(config, "Interaction", inter);
    
    int fmsize = atoi(fsize);
    times = atoi(temp);
    int interaction = atoi(inter);
    switch(type) {
        case 100: {                   
            time_i = 5;
            sprintf(logname, "%s/cpu.log", path); 
            break; 
        }
        case 101: {
            time_i = 5; 
            sprintf(logname, "%s/mem.log", path); 
            break; 
        }
        case 102: { 
            time_i = 60;   
            sprintf(logname, "%s/disk.log", path);
            break;
        }
        case 103: {
            time_i = 30;   
            sprintf(logname, "%s/proc.log", path); 
            break;
        }
        case 104: {
            time_i = 60;   
            sprintf(logname, "%s/sys.log", path); 
            break;
        }
        case 105: {
            time_i = 60; 
            sprintf(logname, "%s/user.log", path); 
            break;
        }
    }

    sprintf(run, "bash ./%s", src);
    if(type == 101) sprintf(run, "bash ./%s %lf", src, dynamic);
    while(1){

        for(int i = 0; i < times; i++){
            char buff[400] = {0};
            if(NULL == (fstream = popen(run, "r"))){
                printf("popen error");
                exit(1);
            }
            if(type == 101) {
                if(fgets(buff, sizeof(buff), fstream)){
                    strcat(buffer, buff);
                }
                if(fgets(buff, sizeof(buff), fstream)){
                    dynamic = atof(buff);
                }else {
                    dynamic = 0;
                }
            } else {
                while(fgets(buff, sizeof(buff), fstream)){
                    strcat(buffer, buff);
                }
            }
            sleep(time_i);
            pclose(fstream);
            if(type == 100 && sign_sys == 0){
                printf("\033[31m*\033[0m");
                fflush(stdout);
                pthread_mutex_lock(&msg->sm_mutex);
                if(msg->flag++ >= interaction - 1){
                    if(msg->sm_time == 0){
                        printf("系统自检超过\033[31m%d\033[0m 次, master 无法连接\n", msg->flag);
                        pthread_cond_signal(&msg->sm_ready);
                        sign_sys = 1;
                        printf("发送信号 心跳开始启动 ❤ \n");
                    }
                    msg->flag = 0;
                }
                pthread_mutex_unlock(&msg->sm_mutex);
            }
        }
        
        FILE *fd = fopen(logname, "a+");
        if(NULL == fd){
            printf("error open logfile\n");
            exit(1);
        }
        fwrite(buffer, 1, strlen(buffer), fd);
 /*       if(file_size(logname) > fmsize){
            printf("%s size : %d\n", logname, file_size(logname));
            time_t timep = time(NULL);
            struct tm *t;
            t = localtime(&timep);
            char newfile[20] = {0};
            sprintf(newfile, "%s_%02d%02d%02d%02d%02d", logname, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
            printf("newfile : %s\n", newfile);
            compress_file(logname, newfile);
            char outname[20] = {0};
            sprintf(outname, "%s_file", logname);
            decompress_file(newfile, outname);
        }*/
        fclose(fd);
    }
    return ;
}

int send_to(int sock_ctrl, int num1, int msg_port){
    char logname[20] = {0};
    char path[10] = {0};
    get_conf_value(config, "Backfile", path);
    switch(num1) {
        case 100: sprintf(logname, "%s/cpu.log", path);  break; 
        case 101: sprintf(logname, "%s/mem.log", path);  break;
        case 102: sprintf(logname, "%s/disk.log", path); break;
        case 103: sprintf(logname, "%s/proc.log", path); break;
        case 104: sprintf(logname, "%s/sys.log", path);  break;
        case 105: sprintf(logname, "%s/user.log", path); break;   
    }
    FILE *fp = fopen(logname, "r");
    char ch = fgetc(fp);
    int sign = 0;
    if(ch == EOF){
        num1 += 300;
    } else {
        num1 += 100;
        sign = 1;
    } 
    send(sock_ctrl, &num1, sizeof(num1), 0);
    printf("send num1 = %d\n", num1);
    if(sign == 1){
        int msg_fd;
        if((msg_fd = sock_create(msg_port)) < 0){
            printf("error in msg_fd create\n");
            return -1;
        }
        int sock_msg = accept(msg_fd, NULL, NULL);
        if(sock_msg < 0){
            printf("msg_fd error in accept\n");
            return -1;
        }
        printf("sock_msg accept!\n");
        char buffer[9128] = {0};
        fread(buffer, sizeof(buffer), 1, fp);
        send(sock_msg, buffer, sizeof(buffer), 0);
        remove(logname);                       //发送数据之后请空文件
        close(msg_fd);
    }                      
    return 0;
}


int main(){
    int shmid;
    int heart_listen;
    int port_heart, port_server;
    char temp_port[5] = {0};
    char temp_server[5] = {0};
    char ip_master[20] = {0};
    char temp_max[20] = {0};
    char ctrl_p[5] = {0};
    char msg_p[5] = {0};

    get_conf_value(config, "Msg_Port", msg_p);
    get_conf_value(config, "Ctrl_Port", ctrl_p);
    get_conf_value(config, "Client_Port", temp_port);
    get_conf_value(config, "Server_Port", temp_server);
    get_conf_value(config, "Server_Ip", ip_master);
    get_conf_value(config, "MaxTimes", temp_max);
    
    int msg_port = atoi(msg_p);       //8732
    int ctrl_port = atoi(ctrl_p);     //9000
    int max_times = atoi(temp_max);
    port_heart = atoi(temp_port);     //10004
    port_server = atoi(temp_server);  //8731

    //建立共享内存区
    if((shmid = shmget(IPC_PRIVATE, sizeof(struct sm_msg), 0666|IPC_CREAT)) == -1){
        perror("error in shmget:");
        return -1;
    }
    shared_memory = (char *)shmat(shmid, 0, 0);
    if(shared_memory == NULL){
        perror("shmat:");
        return -1;
    }
    msg = (struct sm_msg *)shared_memory;
    msg->flag = 0;
    msg->sm_time = 0;
    pthread_mutexattr_init(&m_attr);
    pthread_condattr_init(&c_attr);
    pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED); 
    pthread_condattr_setpshared(&c_attr, PTHREAD_PROCESS_SHARED);
    
    //初始化互斥锁和条件变量
    pthread_mutex_init(&msg->sm_mutex, &m_attr);
    pthread_cond_init(&msg->sm_ready, &c_attr);

    connect_nonblock(port_server, ip_master);   //上线就与master连接8731 并且断开

    int pid;

    if((pid = fork()) < 0){
        perror("fork error:");
        return -1;
    }

    if(pid != 0){
        printf("父进程\n");
        if((heart_listen = sock_create(port_heart)) < 0){  //10004
            perror("error in sock create heart_listen");
            return -1;
        }
        while(1){
            int fd;
            if((fd = accept(heart_listen, NULL, NULL)) < 0){
                perror("heart_listen accept:");
                close(fd);
                return -1;
            }
            printf("\033[31m*\033[0m");
            fflush(stdout);
            close(fd);
        }
    } else {
        int pid_2;
        if((pid_2 = fork()) < 0){
            perror("fork error pid_2");
            return -1;
        }
        if(pid_2 == 0){
            printf("子进程监听中...  %d\n", port_server);      //8731服务端是否在线
            while(1){
                pthread_mutex_lock(&msg->sm_mutex);
                printf("子进程等待信号开启心跳！\n");
                pthread_cond_wait(&msg->sm_ready, &msg->sm_mutex);
                printf("获得心跳信号，开始心跳  ❤\n");
                pthread_mutex_unlock(&msg->sm_mutex);
                while(1){
                    if(client_heart(ip_master, port_server) > 0){   //如果master断线，client尝试连接8731
                        printf("第%d次心跳成功!\n", msg->flag);
                        pthread_mutex_lock(&msg->sm_mutex);
                        msg->sm_time = 0;
                        msg->flag = 0;
                        pthread_mutex_unlock(&msg->sm_mutex);
                        fflush(stdout);
                        sign_sys = 0;
                        break;
                    } else {
                        printf("第%d次心跳失败!\n", msg->sm_time);
                        pthread_mutex_lock(&msg->sm_mutex);
                        msg->sm_time++;
                        pthread_mutex_unlock(&msg->sm_mutex);
                        fflush(stdout);
                    }
                    if(msg->sm_time > max_times) msg->sm_time = max_times;
                    sleep(6 * msg->sm_time);
                    pthread_mutex_unlock(&msg->sm_mutex);
                }
            }   
        } else {
            printf("孙子进程\n");
            int pid_3;
            int x = 0;
            for(int i = 100; i <= 105; i++){    //开6个进程分别写.log文件
                x = i;
                if((pid_3 = fork()) < 0){
                    perror("pid_3");
                    continue;
                }
                if(pid_3 == 0) break;    //子进程执行到这一步结束
            }
            if(pid_3 == 0){
                sys_detect(x);
            } else {
                printf("Father!\n");
                int client_listen;
                if((client_listen = sock_create(ctrl_port)) < 0){
                    printf("client_listen error!\n");
                }
                int sock_ctrl;
                while(1){
                    if((sock_ctrl = accept(client_listen, NULL, NULL)) < 0){
                        printf("error in accept client_listen!\n");
                        continue;
                    }
                    printf("sock_ctrl connect!\n");
                    printf("\033[31m#\033[0m");
                    fflush(stdout);
                    int k = 0;
                    while(1){
                        int num1;
                        recv(sock_ctrl, &num1, sizeof(num1), 0);
                        printf("num1 = %d\n", num1);
                        send_to(sock_ctrl, num1, msg_port);
                        k++;
                        if(k > 6) break;
                    }
                    pthread_mutex_lock(&msg->sm_mutex);
                    msg->flag = 0;
                    pthread_mutex_unlock(&msg->sm_mutex);
                    close(sock_ctrl);
                }
            }
        }
    }

    return 0;
}
