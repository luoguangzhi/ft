#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "ft_common.h"

#define KEY_MAX_LEN   (64)
#define VALUE_MAX_LEN (64)

#define KEY_NAME_GEN              (0)
#define KEY_SHR_PATH_GEN          (1)
#define CONF_FILE                 "./.ftconf"
#define BUFFER_SIZE               1024
#define FILE_NAME_MAX_SIZE        128
#define FULL_NAME_LEN             256
#define RENAME_CDM_LEN            520
#define UDP_BROADCAST_PORT        10086
#define LISTEN_PORT               10010
#define IP_LEN                    16
#define HOST_NAME_LEN             10



static int get_down_name(const char *name, char *full_name)
{
    int ret = -1;
    char path[128] = {0};
    char *path_name = cfg_value_get(0, path);
    ret = sprintf(full_name, "%s/%s",path_name,name);
    if (ret < 0)
    {
        printf("get full name  failedn");
        return -1;
    }
    return 0;
}

int return_ip(void)
{
    int  ret                           = -1;
    char recv_name[HOST_NAME_LEN]      = {0};
    char host_name[HOST_NAME_LEN]      = {0};
    char file_path[FILE_NAME_MAX_SIZE] = {0};
    char *gateway                      = NULL;
    int gateway_num                    = 0;
    unsigned int server_addr_len       = 0;
    struct in_addr *addr               = NULL;

    char *ip = malloc(IP_LEN);
    if (ip == NULL)
    {
        printf("malloc failed\n");
        return -1;
    }
    memset(ip, 0x00, IP_LEN);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(UDP_BROADCAST_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr_len = sizeof(server_addr);
    if(recvfrom(sock, recv_name, HOST_NAME_LEN, 0, (struct sockaddr*)&server_addr, &server_addr_len) < 0) 
    {
        printf("recvfrom failed\n");
        return -1;
    }
    
    cfg_value_get(KEY_NAME_GEN, host_name);
    if (strcmp(host_name, recv_name) != 0)
    {
        printf("recv name %s host name %s\n", recv_name, host_name);
        return -1;
    }
    
    gateway = get_gateway();
    gateway_num = inet_aton(gateway, addr);
    if (gateway_num == 0) 
    {
        printf("inet_aton  fail\n");
        return -1;
    }

    ret = get_host_ip(gateway_num, &ip);
    if (ret != 0)
    {
        printf("get host ip failed\n");
        return -1;
    }
    
    ret = sendto(sock, ip, IP_LEN, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        printf("send ip failed\n");
        return -1;
    }   

    return 0;
}


int main(int argc, char **argv)
{
    int ret                            = -1;
    int server_fd;
    char read_buffer[BUFFER_SIZE]      = {0};
    char file_name[FILE_NAME_MAX_SIZE] = {0};
    char full_name[FULL_NAME_LEN] = {0};
    char tmp_full_name[FULL_NAME_LEN + 4] = {0};
    FILE *fp                           = NULL;
    char set_cmd[3]                    = {0};
    char set_param[20]                 = {0};
    char cmd[CMD_LEN]                  = {0};
    char mv_cmd[RENAME_CDM_LEN]    = {0};

    printf("argc = %d\n", argc);
    if (argc != 4 && argc != 1)
    {
        printf("cmd num error need 4 param\n");
        return -1;
    }
    
    
    if (argc == 4) 
    {
        printf("%s %s %s\n", argv[1], argv[2], argv[3]);
        if (strncmp(argv[1], "set", 3) == 0)
        {
            strcpy(set_cmd, argv[2]);
            if (strncmp(set_cmd, "-n", 2) == 0)
            {
                ret = cfg_value_set(KEY_NAME_GEN, argv[3]);
                if (ret < 0)
                {
                     printf("set host name failed\n");
                    return -1;
                }
            }
            else if (strncmp(set_cmd, "-p", 2) == 0)
            {
                ret = cfg_value_set(KEY_SHR_PATH_GEN, argv[3]);
                if (ret < 0)
                {
                    printf("set recv path failed\n");
                    return -1;
                }
            }
            else
            {
                printf("input cmd error allow  set -n   hostname  set -p pathname\n");
                return -1;
            }
            return 0;
        }
    }
    ret = return_ip();
    if (ret < 0)
    {
        printf("return ip failed\n");
        return -1;
    }
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LISTEN_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        printf("greate socket faile0d\n");
        return -1;
    }

    ret = bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret == -1)
    {
        printf("bind error\n");
        return -1;
    }

    ret = listen(server_fd, 5);
    if (ret != 0)
    {
            printf("listen failed");
            return -1;
    }

    while(1)
    {
        struct sockaddr_in client_addr;
        int size = sizeof(client_addr);
        int new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &size);
        
        if (new_fd == -1)
        {
            printf("accept error continue\n");
            continue;
        }
        

        ret = read(new_fd, read_buffer, sizeof(read_buffer));
        if (ret < 0)
        {
            printf("get file name error\n");
            continue;
        }
        memset(file_name,0x00, sizeof(file_name));
        strncpy(file_name, read_buffer, FILE_NAME_MAX_SIZE);
        // get file name 
        ret = get_down_name(file_name, full_name);
        if (ret < 0)
        {
            printf("get full name failed file name %s\n", full_name);
            continue;
        }
        sprintf(tmp_full_name, "%s.tmp", full_name);
        fp = fopen(tmp_full_name, "w");
        if (fp == NULL)
        {
            printf("open %s failed\n", full_name);
            continue;
        }

        while (recv(new_fd, read_buffer, BUFFER_SIZE, 0) >0)
        {
            ret = fwrite(read_buffer, BUFFER_SIZE, 1, fp);
            if (ret != 1)
            {
                printf("write %s fail \n", full_name);
                fclose(fp);
                close(new_fd);
                break;
            }
        }
        sprintf(mv_cmd, "mv %s %s", tmp_full_name, full_name);
        system(mv_cmd);
        fclose(fp);
        close(new_fd);
    }
}