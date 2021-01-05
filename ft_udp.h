#ifndef _FT_UDP_h
#define _FT_UDP_h

#include <stdio.h>

/******************************************************************************/
#define PIECE_SIZE   (10 * 1024)
#define RECV_TIMEOUT (10)
/******************************************************************************/
int sock_udp(unsigned short port, char *ip);

int udp_recv_save_file(int sock);

int udp_send_file(const char *p_ip, const char *p_file_path);

int udp_recv_file(const char *p_ip, const char *p_file_name);

int udp_recv(int sock, void *p_buf, size_t buf_size, unsigned int timeout);
#endif /* _FT_UDP_h */