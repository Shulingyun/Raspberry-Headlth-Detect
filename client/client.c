/*************************************************************************
	> File Name: client.c
	> Author: 
	> Mail: 
	> Created Time: 2019年03月16日 星期六 19时20分20秒
 ************************************************************************/

#include <stdio.h>
#include "common1.h"
struct sm_msg{//共享内存的结构体
    int flag;  //检测次数
    int sm_time;
    pthread_mutex_t sm_mutex;//两个锁
    pthread_cond_t sm_ready;
};

char *config = "./PiHealth.conf";
char *share_memory = NULL;//共享内存
char log_dir[50] = {0};
char log_backup[50] = {0};
double dyaver = 0;     //memlog脚本的动态平均值
int Max_KeepAlive_Interval = 0;
int ctrl_port;
int sock_ctrl;

struct sm_msg *msg; 

pthread_mutexattr_t m_attr;//定义两个属性参数
pthread_condattr_t c_attr;

void sys_detect(int type) {
    int time_i = 0;
    char temp_sc[20] = {0};
    char src[50] = {0};
    sprintf(temp_sc, "Src%d", type);
    char run[100] = {0};
    char buffer[4096] = {0};
    //DBG("temp_sc:%s\n", temp_sc);
    get_conf_value(config, temp_sc, src);
    //DBG("SRC----->>>:%s\n", src);
    FILE *fstream = NULL;
    int times = 0;
    int Inactive_Interval = 0;
    char temp[4] = {0};
    char logname[50] = {0};
    
    get_conf_value(config, "WriteEveryTimes", temp);
    times = atoi(temp);
    //printf("----times:%d\n", times);
    get_conf_value(config, "Inactive_Interval", temp);
    Inactive_Interval = atoi(temp);
    get_conf_value(config, "LogDir", log_dir);

    //DBG("sys_detect<>--<>\n");

    switch (type) {//每隔多少秒执行一次
        case 100://cpu
            time_i = 1;
            sprintf(logname, "%s/cpu.log", log_dir);
            break;
        case 101://mem
            time_i = 5;
            sprintf(logname, "%s/mem.log", log_dir);
            break;
        case 102://disk
            time_i = 60;
            sprintf(logname, "%s/disk.log", log_dir);
            printf("logname = %s/disk.log", log_dir);
            break;
        case 103://pro恶意进程
            time_i = 30;
            sprintf(logname, "%s/pro.log", log_dir);
            break;
        case 104://sysinfo
            time_i = 60;
            sprintf(logname, "%s/sys.log", log_dir);
            break;
        case 105://unserinfo
            time_i = 60;
            sprintf(logname, "%s/user.log", log_dir);
            break;
    }

    sprintf(run, "bash ./script/%s", src);
    //DBG("run:%s\n", run);
    if (type == 101) sprintf(run, "bash ./script/%s %f", src, dyaver);
    while (1) {
        //DBG("run2:%s\n", run);
        for (int i = 0; i < times; i++) {
            char buff[400] = {0};
            if (NULL == (fstream = popen(run, "r"))) {
                DBG("popen error!\n");
                exit(1);
            }
            if (type == 101) {
                if (fgets(buff, sizeof(buff), fstream)) {
                    strcat(buffer, buff);
                    //DBG("---------\n");
                }
                if (NULL != fgets(buff, sizeof(buff), fstream)) {
                    dyaver = atof(buff);
                } else {
                    dyaver = 0;
                }
            } else {
                while (NULL != fgets(buff, sizeof(buff), fstream)) {
                    strcat(buffer, buff);
                }
            }
            sleep(time_i);
            //DBG("time_i:%d", time_i);
            pclose(fstream);
            if (type == 100) {
                DBG("\033[31m*\033[0m ");
                fflush(stdout);
                pthread_mutex_lock(&msg->sm_mutex);
                if (msg->flag++ >= Inactive_Interval - 1) {
                    if (msg->sm_time == 0) {
                        DBG("\n系统自检超过 \033[033m%d\033[0m 次, Master 无连接\n", msg->flag);
                        pthread_cond_signal(&msg->sm_ready);
                        DBG("发送信号，开启心跳程序 ❤ \n");
                    }
                    msg->flag = 0;
                }
                pthread_mutex_unlock(&msg->sm_mutex);
            }
        }
        FILE *fd = fopen(logname, "a+");
        if (NULL == fd) {
            DBG("Error open logfile!\n");
            exit(1);
        }
        if (flock(fd->_fileno, LOCK_EX) < 0) {
            DBG("flock : %s\n", strerror(errno));
        }
        fwrite(buffer, 1, strlen(buffer), fd);
        fclose(fd);
    }
    return ;
}



bool client_heart(char *ip, int port) {
    bool ret = true;
    int fd;
    if ((fd = socket_connect(port, ip)) < 0) {
        ret = false;
    }
    close(fd);
    return ret;  
}


int main() {
    int shmid;
    int heat_listen;
    int port_heart, port_master;
    char temp_ctrl[5] = {0};
    char temp_inter[5] = {0};
    char temp_client[5] = {0};
    char temp_master[5] = {0};
    char ip_master[20] = {0};
    char port_temp[5] = {0};

    get_conf_value(config, "Client_Port", temp_client);
    get_conf_value(config, "Master_Port", temp_master);
    get_conf_value(config, "Master_Ip", ip_master);
    get_conf_value(config, "LogDir", log_dir);
    get_conf_value(config, "BackupLogDir", log_backup);
    get_conf_value(config, "Max_KeepAlive_Interval", temp_inter);
    get_conf_value(config, "Ctrl_Port", temp_ctrl);
    
    port_heart = atoi(temp_client);//10001
    port_master = atoi(temp_master);//8731
    Max_KeepAlive_Interval = atoi(temp_inter);
    ctrl_port = atoi(temp_ctrl);//9000
    
    mkdir(log_dir, 0755);
    mkdir(log_backup, 0755);

    //虚拟共享内存
    if ((shmid = shmget(IPC_PRIVATE, sizeof(struct sm_msg), 0666|IPC_CREAT)) == -1) {
        DBG("Error in shmget: %s\n", strerror(errno));
        return -1;
    }
    if ((share_memory = (char *)shmat(shmid, 0, 0)) == NULL) {
        DBG("shmat : %s\n", strerror(errno));
        return -1;
    }

    msg = (struct sm_msg *)share_memory;//指针指向结构体
    msg->flag = 0;//初始化
    msg->sm_time = 0;
    pthread_mutexattr_init(&m_attr);//线程锁
    pthread_condattr_init(&c_attr);
    pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&c_attr, PTHREAD_PROCESS_SHARED);

    //初始化互斥锁和条件变量
    pthread_mutex_init(&msg->sm_mutex, &m_attr);
    pthread_cond_init(&msg->sm_ready, &c_attr);

    int pid;
    connect_nonblock(port_master, ip_master, 1);//上线主动链接一次Master

    if ((pid = fork()) < 0) {
        DBG("fork error\n");
        return -1;
    }
    
    if (pid != 0) {//父进程监听心跳
        if ((heat_listen = socket_create(port_heart)) < 0) {
            DBG("Error in socket_create!");
            return -1;
        }
        while (1) {
            int fd;//阻塞
            if ((fd = accept(heat_listen, NULL, NULL)) < 0) {
                DBG("accept error on heart_listen!\n");
                close(fd);
            }
            DBG("\033[35m*\033[0m ");
            fflush(stdout);
            close(fd);
        }

    } else {//子进程
        int pid1;
        if ((pid1 = fork()) < 0) {
            DBG("fork error pid1!\n");
            return -1;
        }
        if (pid1 == 0) {
            DBG("子进程监听中...%d\n", port_master);
            while (1) {//子进程主动心跳
                pthread_mutex_lock(&msg->sm_mutex);
                DBG("子进程等待信号开启心跳!\n");
                fflush(stdout);
                pthread_cond_wait(&msg->sm_ready, &msg->sm_mutex);
                DBG("获得心跳信号，开始心跳 ❤ \n");
                pthread_mutex_unlock(&msg->sm_mutex);
                while (1) {
                    if (client_heart(ip_master, port_master)) {
                        DBG("\n第%d次心跳成功!\n", msg->sm_time);
                        pthread_mutex_lock(&msg->sm_mutex);
                        msg->sm_time = 0;
                        msg->flag = 0;
                        pthread_mutex_unlock(&msg->sm_mutex);
                        fflush(stdout);
                        break;       
                    } else {
                        DBG("\n第%d次心跳失败\n", msg->sm_time);
                        pthread_mutex_lock(&msg->sm_mutex);
                        msg->sm_time++;
                        pthread_mutex_unlock(&msg->sm_mutex);
                        fflush(stdout);
                    }
                    sleep(6 * msg->sm_time);
                    if (msg->sm_time > Max_KeepAlive_Interval) msg->sm_time = Max_KeepAlive_Interval;
                    pthread_mutex_unlock(&msg->sm_mutex);
                }
            } 
        } else {
            DBG("孙子进程\n");
            int x = 0;
            int pid2;//给每个孙子进程加ID
            for (int i = 100; i < 106; i++) {//开6个进程分别写.log文件
                x = i;
                if ((pid2 = fork()) < 0) {
                    DBG("error fork pid2\n");
                    continue;
                }
                if (pid2 == 0) break;//子进程执行到这结束
            }
            if (pid2 == 0) {
                sys_detect(x);
            } else {
                DBG("Father!\n");
                int client_listen;
                if ((client_listen = socket_create(ctrl_port)) < 0) {
                    DBG("create client_listen error : %s\n", strerror(errno));
                }
                DBG("client_listen <<----->>\n");
                while (1) {
                    DBG("输出sock_crtl\n");
                    if ((sock_ctrl = accept(client_listen, NULL, NULL)) < 0) {
                        DBG("Error in accept client_listen : %s\n", strerror(errno));
                        continue;
                    }
                    DBG("sock_ctrl connect\n");
                    fflush(stdout);
                    DBG("\033[35maccept success!\033[0m ");
                    fflush(stdout);
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

