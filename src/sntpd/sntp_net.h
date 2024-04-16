
#ifndef _SNTP_NET_H_
#define _SNTP_NET_H_

#include <stdint.h>
#include <sys/socket.h>

int setup_socket(char *srv, uint16_t srv_port, uint16_t bind_port);
int server_init(uint16_t port);
int server_recv(int sd);
int check_source(int data_len, struct sockaddr_storage *ss, uint16_t srv_port);


#endif /* _SNTP_NET_H_ */

