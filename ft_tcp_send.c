#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "ft_common.h"

#define KEY_MAX_LEN   (64)
#define VALUE_MAX_LEN (64)

#define KEY_NAME_GEN              (0)
#define KEY_SHR_PATH_GEN          (1)
#define CONF_FILE                 "./.ftconf"
#define BUFFER_SIZE               1024
#define FILE_NAME_MAX_SIZE        256
#define UDP_BROADCAST_PORT        10086
#define LISTEN_PORT               10010
#define PARAM_LEN                 30
#define PARAM_NUM                 4
#define IP_LEN                    16



int send_file(const char *ip, const char *file_name)
{
    int ret = -1;
    int server_fd;
    struct sockaddr_in server_addr;
    FILE *fp = NULL;
    char send_buf[BUFFER_SIZE] = {0};

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LISTEN_PORT);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        printf("greate socket faile0d\n");
        return -1;char cmd[CMD_LEN] = {0};
        printf("send file error connect %s fail\n", ip);
        return -1;
    }
    
    ret = access(file_name, F_OK);
    if (ret == -1)
    {
        printf("send file error %s no exist\n", file_name);
        close(server_fd);
        return ret;
    }
    
    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        printf("send file error reason open %s failed \n", file_name);
        close(server_fd);
        return -1;
    }

    while (fgets(send_buf, BUFFER_SIZE, fp) != NULL)
    {
        ret = send(server_fd, send_buf, BUFFER_SIZE, 0);
        if (ret <= 0)
        {
            printf("send file error, send failed\n");
            fclose(fp);
            close(server_fd);
            return -1;
        }
    }

    fclose(fp);
    close(server_fd);
    return 0;


}



int main(int argc, char **argv)
{
    char cmd[CMD_LEN] = {0};
    char host_name[PARAM_LEN] = {0};
    char file_name[PARAM_LEN] = {0};
    char ip_addr[IP_LEN]      = {0};
    char set_cmd[3]           = {0};
    char set_param[20]        = {0};
    int  ret                  = -1;

    if (argc != PARAM_NUM)
    {
        printf("input param num error num = %d, please check\n", argc);
        return -1;
    }
    
    strcpy(cmd, argv[1]);

    if (strncmp(cmd, "-s", 2) == 0)
    {
       strcpy(host_name, argv[2]);
       strcpy(file_name, argv[3]);
       ret = get_dst_addr(host_name, ip_addr, IP_LEN);
       if (ret != 0)
       {
           printf("get send ip error \n");
           return -1;
       }
       ret = send_file(ip_addr, file_name);
       if (ret != 0)
       {
           printf("send %s to %s error\n", file_name, ip_addr);
           return -1;
       }
    }
    else
    {
        printf("input param error\n");
        return -1;
    }
    return 0;
}