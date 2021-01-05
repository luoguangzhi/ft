#include <stdio.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ft_common.h"
#include "ft_udp.h"

/******************************************************************************/
#define BROAD_PORT        (10086)
/******************************************************************************/
static int _get_local_ip(int sock)
{
    int ret;
    struct ifconf ifconf;
    struct ifreq *ifreq;
    struct ifreq  ifmask;

    char if_buf[512];
    ifconf.ifc_len = 512;
    ifconf.ifc_buf = (caddr_t)if_buf;
    char msg_buf[256] = {0};

    in_addr_t if_addr;
    in_addr_t re_addr;
    in_addr_t mask;

    struct sockaddr_in remote_addr;
    (void)memset(&remote_addr, 0, sizeof(remote_addr));
    socklen_t sock_len = sizeof(remote_addr);

    ret = recvfrom(sock, (void *)msg_buf, sizeof(msg_buf), 0, (struct sockaddr *)&remote_addr, &sock_len);
    if (ret < 0) {
        printf("recvfrom fail, err(%d:%s)\n", errno, strerror(errno));
        return -1;
    }
    printf("remote ip: %s\n", inet_ntoa(remote_addr.sin_addr));
    printf("recv name: %s\n", msg_buf);
    char name[128] = {0};
    if (NULL == cfg_value_get(KEY_NAME_GEN, name)) {
        printf("Not set local host name\n");
        return -1;
    }

    /* check name is right */
    if(!strcmp(msg_buf, name)) {
        printf("host name right\n");
        ioctl(sock, SIOCGIFCONF, &ifconf); //获取所有接口信息
        ifreq = (struct ifreq*)ifconf.ifc_buf;
        
        re_addr = remote_addr.sin_addr.s_addr;
        for (int i=(ifconf.ifc_len/sizeof (struct ifreq)); i > 0; i--) {
            if(ifreq->ifr_flags == AF_INET) { //for ipv4
                if_addr = ((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr.s_addr;

                strcpy(ifmask.ifr_name, ifreq->ifr_name);
                if(0 != ioctl(sock, SIOCGIFNETMASK, &ifmask)) {
                    return -1;
                }
                mask = ((struct sockaddr_in*)&ifmask.ifr_addr)->sin_addr.s_addr;

                if((if_addr & mask) == (re_addr & mask)) {
                    char *p = inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);
                    printf("local name = [%s], addr = [%s]\n" , ifreq->ifr_name, p);
                    ret = sendto(sock, p, strlen(p), 0, (struct sockaddr *)&remote_addr, sock_len);
                    if(ret != strlen(p)) {
                        printf("(%s:%d)sendto fail, err(%d:%s)\n", __func__, __LINE__, errno, strerror(errno));
                        return -1;
                    }
                    return 0;
                }
                ifreq++;
            }
        }
    } 
    return -1;
}

static int _send_file(int sock, struct sockaddr_in dst_addr)
{
    int  ret;
    char file_name[128]  = {0};
    char path[256]       = {0};
    char share_path[128] = {0};

    ret = udp_recv(sock, (void *)file_name, sizeof(file_name), 5);
    if(ret <= 0) {
        printf("udp recv fail!\n");
        return -1;
    }

    cfg_value_get(KEY_SHR_PATH_GEN, share_path);
    sprintf(path, "%s/%s", share_path, file_name);
    char *ip = inet_ntoa(dst_addr.sin_addr);
    printf("dst ip : %s\n", ip);
    ret = udp_send_file(ip, path);
    if(ret < 0) {
        printf("file send fail\n");
        return -1;
    }
}

static int _commond_handle(int sock)
{
    int        ret;
    socklen_t  sock_len;
    struct sockaddr_in remote_addr;
    char cmd[5] = {0};
    sock_len = sizeof(struct sockaddr);
    
    while (1) {
        printf("waiting CMD...\n");
        if(recvfrom(sock, cmd, CMD_LEN, 0, (struct sockaddr*)&remote_addr, &sock_len) > 0) {
            printf("recv cmd: %s\n", cmd);
            if(!strncmp(cmd, CMD_GET_IP, CMD_LEN)) {
               ret = _get_local_ip(sock);
               if(ret < 0) {
                   printf("Not find IP\n");
                   continue;
               }
            } else if(!strncmp(cmd, CMD_SEND, CMD_LEN)) {
                ret = udp_recv_save_file(sock);
                if(ret < 0) {
                    printf("receive file fail!\n");
                    continue;
                }
            } else if(!strncmp(cmd, CMD_RECV,CMD_LEN)) {
               ret = _send_file(sock, remote_addr);
               if(ret < 0) {
                    printf("send file fail!\n");
               }
            } else {
                printf("commond not support\n");
            }
        }
    }
}

static int _ft_daemon(void)
{
    int ret;
    int sock;
    sock = sock_udp(BROAD_PORT, NULL);
    if(sock < 0) {
        return -1;
    }
    if(_commond_handle(sock) < 0) {
        close(sock);
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    _ft_daemon();
    return 0;
}
