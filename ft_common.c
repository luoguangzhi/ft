#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "ft_common.h"
#include "ft_udp.h"
/*****************************************************************************/
#define KEY_MAX_LEN       (32)
#define VALUE_MAX_LEN     (32)

#define CONF_FILE         ".ftconf"
/*****************************************************************************/

int get_dst_addr(char *p_name, char *p_ip, size_t ip_len)
{
    int ret;
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("Creat socket fail, err(%d:%s)\n", errno, strerror(errno));
        return -1;
    }

    const int on = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in broad_addr;
    broad_addr.sin_family = AF_INET;
    broad_addr.sin_port   = htons(BROAD_PORT);
    broad_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    ret = sendto(sock, (void *)CMD_GET_IP, CMD_LEN, 0, (struct sockaddr*)&broad_addr, sizeof(broad_addr));
    if (ret != CMD_LEN) {
        ret = -1;
        goto _exit;
    }
    printf("Send -I commond\n");

    ret = sendto(sock, (void *)p_name, strlen(p_name), 0, (struct sockaddr*)&broad_addr, sizeof(broad_addr));
    if (ret != strlen(p_name)) {
        ret = -2;
        goto _exit;
    }
    printf("Send target host name\n");

    struct sockaddr_in dst_addr;
    socklen_t addr_len = sizeof(dst_addr);
    memset(p_ip, 0, ip_len); 
    ret = recvfrom(sock, p_ip, ip_len, 0, (struct sockaddr*)&dst_addr, &addr_len);
    if(ret <= 0) {
        ret = -3;
        goto _exit;
    }

    printf("Target compter IP: %s\n", p_ip);
    ret = 0;

_exit:
    close(sock);
    return ret;
}

char *cfg_value_get(int gen, char *p_value)
{
    int fd;
    char *home_path = getenv("HOME");
    char path[128]  = {0};
    sprintf(path, "%s/%s", home_path, CONF_FILE);
    fd = open(path, O_RDWR | O_CREAT, 0744);
    if(fd < 0) {
        return NULL;
    }

    lseek(fd, gen * KEY_MAX_LEN * VALUE_MAX_LEN, SEEK_SET);
    read(fd, (void *)p_value, VALUE_MAX_LEN);

    close(fd);

    if(0 == strlen(p_value)) {
        return NULL;
    } else {
        return p_value;
    }
}

int cfg_value_set(int gen, const char *p_value)
{
    int fd, ret;
    char *home_path = getenv("HOME");
    char path[128]  = {0};
    sprintf(path, "%s/%s", home_path, CONF_FILE);
    printf("config file path:%s\n", path);
    fd = open(path, O_RDWR | O_CREAT, 0744);
    if (fd < 0) {
        printf("(%s:%d)Open config file fail!\n", __func__, __LINE__);
        return -1;
    }
    lseek(fd, gen * KEY_MAX_LEN * VALUE_MAX_LEN, SEEK_SET);
    ret = write(fd, p_value, strlen(p_value));
    if (ret != strlen(p_value)) {
        close(fd);
        printf("(%s:%d)write fail!\n", __func__, __LINE__);
    }

    close(fd);

    return 0;
}

#define UDP
int send_file(const char *p_ip, const char *p_file_path)
{
    int ret;
#ifdef UDP
    ret = udp_send_file(p_ip, p_file_path);
#endif

#ifdef TCP

#endif
    return ret;
}

int recv_file(const char *p_ip, const char *p_file_path)
{
    int ret;
#ifdef UDP
    ret = udp_recv_file(p_ip, p_file_path);
#endif

#ifdef TCP

#endif
    return ret;
}