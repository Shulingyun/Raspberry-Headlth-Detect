#ifndef COMMON_H
#define COMMON_H

#include "head.h"


int socket_create(int port);


int socket_connect(int port, char *host);

int get_conf_value(char *pathname, char* key_name, char *value);

int UDP_create(int port);

int transip(char *sip, int *ip);

int find_min(int *sum, int ins);

int connect_nonblock(int port, char *host);

int write_Pi_log (char *PiHealthLog, const char *format, ...);

int get_time(char *cur_time);


#endif
