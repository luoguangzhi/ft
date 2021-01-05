#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "ft_common.h"
/*****************************************************************************/
#define HELP \
    {        \
\
    }
/*****************************************************************************/

static void _printf_help_info(void)
{
    printf("usage:\n"); 
    printf("     ft -h/-help : help infomartion\n");
    printf("     ft set -n [user name] : set user name.\n");
    printf("     ft set -p [share path] : set share directory.\n"); 
    printf("     ft -s [user name] [file name] : send file to user\n"); 
    printf("     ft -r [user name] [file name] : recvive file from user\n"); 
}

int main(int argc, char *argv[])
{
    int ret;
    if (4 != argc && 2 != argc) {
        _printf_help_info();
        return -1;
    }

    if (2 == argc && (!strncmp(argv[1], "-h", 2) || !strncmp(argv[1], "-help", 5))) {
        _printf_help_info;
        return 0;
    }

    if (!strncmp(argv[1], "-s", 2)) {
        char ip[16] = {0};
        if(get_dst_addr(argv[2], ip, sizeof(ip)) < 0) {
            return -1;
        }
        printf("send file\n");
        send_file(ip, argv[3]); 
    } 
    else if(!strncmp(argv[1], "-r", 2)) {
        char ip[16] = {0};
        ret = get_dst_addr(argv[2], ip, sizeof(ip));
        if (ret < 0) {
            printf("(%s:%d)Get ip fail\n", __func__, __LINE__);
            return -1;
        }
        ret = recv_file(ip, argv[3]);
        if (ret < 0) {
            printf("(%s:%d)Send file fail\n", __func__, __LINE__);
            return -1;
        }
    } 
    else if(!strncmp(argv[1], "set", 3)) {
        if (!strncmp(argv[2], "-p", 2)) {
            cfg_value_set(KEY_SHR_PATH_GEN, argv[3]);
        }
        if (!strncmp(argv[2], "-n", 2)) {
            cfg_value_set(KEY_NAME_GEN, argv[3]);
        }
    }
    return 0;
}
