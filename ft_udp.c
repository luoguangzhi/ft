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
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "ft_udp.h"
#include "ft_common.h"

/******************************************************************************/
#define UDP_PORT          (10010)
/******************************************************************************/
int sock_udp(unsigned short port, char *ip)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("Creat socket fail, err(%d:%s)\n", errno, strerror(errno));
        return -1;
    }

    struct sockaddr_in bind_addr;
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port   = htons(port);
    if(ip) {
        bind_addr.sin_addr.s_addr = inet_addr(ip);
    } else {
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
    }

    const int opt = 1;
    if(0 != setsockopt(sock, SOL_SOCKET ,SO_REUSEADDR,(const char*)&opt,sizeof(opt))) {
        printf("(%s:%d)setsockopt fail, err(%d:%s)", __func__, __LINE__, errno, strerror(errno));
        return -1;
    }

    if(bind(sock, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0){//绑定端口号
        printf("bind fail, err(%d:%s)", errno, strerror(errno));
        return -1;
    } 
    return sock;
}

int udp_recv(int sock, void *p_buf, size_t size, unsigned int timeout)
{
    int    ret;
    fd_set rfds;  
    if(p_buf == NULL) {
        return -1;
    }

    struct timeval time;
    time.tv_sec = timeout; 
    time.tv_usec = 0;

    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        ret = select(sock + 1, &rfds, NULL, NULL, &time);
        if (ret < 0) {
            printf("(%s:%d)receive fail, err(%d:%s)\n", __func__, __LINE__, errno, strerror(errno));
            return -1;
        } else if(ret == 0) {
            /* timeout */
            printf("(%s:%d)receive timeout!\n", __func__, __LINE__);
            return -1;
        } else {
            if(FD_ISSET(sock, &rfds)) {
                ret = recvfrom(sock, (void *)p_buf, size, 0, NULL, NULL);
                if(ret <= 0) {
                    //printf("(%s:%d)receive expect size(%d) unequal to factual size(%d)\n", __func__, __LINE__, (int)size, (int)ret);
                    ret = -1;
                } 
                return ret;
            }
        }
    }
}

static const char *_find_file_name(const char *path)
{
    int ret;
    char sep = '/';
    char *local;
    if(NULL == path) {
        return NULL;
    }
    local = strrchr(path, sep);
    if(local == NULL) {
        return path;
    }
    return local + 1;
}

int udp_send_file(const char *p_ip, const char *p_file_path)
{
    int sock;
    int fd;
    int ret, ret_len, r_size;
    int i;
    int file_len;
    int s_size = 0;
    unsigned char *p_sbuf;
    char buf[4096] = {0};

    if(p_ip == NULL || p_file_path == NULL) {
        return -1;
    }

    printf("file name : %s\n", p_file_path);

    fd = open(p_file_path, O_RDONLY);
    if(fd < 0) {
        printf("file not exist\n");
        return -1;
    }

    struct stat statbuf;
    fstat(fd, &statbuf);
    file_len = statbuf.st_size;
    printf("file length :%d\n", file_len);

    const char *p = _find_file_name(p_file_path);
    if(NULL == p) {
        printf("Can't find file name\n");
        return -1;
    }

    sock = sock_udp(BROAD_PORT, NULL);
    if (sock < 0) {
        return -1;
    }

    struct sockaddr_in dst_addr;
    dst_addr.sin_family      = AF_INET;
    dst_addr.sin_port        = htons(BROAD_PORT);
    dst_addr.sin_addr.s_addr = inet_addr(p_ip);

    ret = sendto(sock, (void *)CMD_SEND, CMD_LEN, 0, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
    if (ret != CMD_LEN) {
        printf("sendto fail, err(%d:%s)\n", errno, strerror(errno));
        goto _exit;
    }

    trans_head_t trans_head;
    memset(&trans_head, 0, sizeof(trans_head_t));
    strncpy(trans_head.p_name, p, strlen(p));
    trans_head.magic = FT_MAGIC_NUM;
    trans_head.file_len = file_len;

    if(0 != (file_len % PIECE_SIZE)) {
        trans_head.blk_num = (file_len / PIECE_SIZE) + 1;
    } 
    else {
        trans_head.blk_num = file_len / PIECE_SIZE;
    }

    p_sbuf = (unsigned char *)malloc(PIECE_SIZE + sizeof(trans_head_t) + 1);

    for (i = 0; i < trans_head.blk_num; i++) {

        r_size = read(fd, p_sbuf + sizeof(trans_head_t), PIECE_SIZE);
        if(r_size < 0) {
            printf("(%s:%d)read file fail\n", __func__, __LINE__);
            goto _exit;
        }
        // if(r_size == 0) {
        //     break;
        // }
        trans_head.blk_index = i;
        trans_head.len       = r_size;

        memcpy(p_sbuf, &trans_head, sizeof(trans_head_t));
        r_size += sizeof(trans_head_t);

        s_size = 0;
        while (1) {
            ret_len = sendto(sock, (void *)p_sbuf + s_size, r_size , 0, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
            if (ret_len < 0) {
                printf("(%s:%d)sendto fail, err(%d:%s)\n", __func__, __LINE__, errno, strerror(errno));
                goto _exit;
            }
            
            r_size -= ret_len;
            if (r_size == 0) {
                printf("send file(%d / %d): %d%%\r", i + 1, trans_head.blk_num, ((i + 1)* 100) / trans_head.blk_num);
                fflush(stdout);
                break;
            }
            s_size += ret_len;
        }
    }

    printf("\nfile send OK\n");
    close(fd);
    return 0;
_exit:
    printf("\n");
    close(sock);
    return -1;
} 

static int udp_recv_head(int sock, trans_head_t *p_head)
{
    int    ret;
    fd_set rfds;  
    if(p_head == NULL) {
        return -1;
    }

    struct timeval time;
    time.tv_sec = RECV_TIMEOUT; 
    time.tv_usec = 0;

    struct sockaddr_in dst_addr;
    socklen_t          sock_len = sizeof(struct sockaddr);
    while (1)
    {
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        ret = select(sock + 1, &rfds, NULL, NULL, &time);
        if (ret < 0) {
            printf("(%s:%d)receive fail, err(%d:%s)\n", __func__, __LINE__, errno, strerror(errno));
            return -1;
        } else if(ret == 0) {
            /* timeout */
            printf("(%s:%d)receive timeout!\n", __func__, __LINE__);
            return -1;
        } else {
            if(FD_ISSET(sock, &rfds)) {
                ret = recvfrom(sock, (void *)p_head, sizeof(trans_head_t), MSG_PEEK, (struct sockaddr *)&dst_addr, &sock_len);
                if(ret != sizeof(trans_head_t)) {
                    printf("(%s:%d)receive expect size(%d) unequal to factual size(%d)\n", __func__, __LINE__, (int)sizeof(trans_head_t), (int)ret);
                    ret = -1;
                }
                if (p_head->magic != FT_MAGIC_NUM) {
                    continue;
                } 
                return 0;
            }
        }
    }
}

int udp_recv_save_file(int sock)
{
    int   ret, i;
    int   length;
    char  file_name[128] = {0};
    int   file_len    = 0;
    int   total_size  = 0;
    int   blk_head[2] = {0};
    unsigned int blk_num     = 0;
    unsigned int blk_index   = 0;
    char  path[256]   = {0};
    char  share_path[100] = {0};
    unsigned char *p_msg_buf = NULL;
    unsigned int magic_num;
    int   recv_size = PIECE_SIZE + sizeof(trans_head_t) + 1;
    
    trans_head_t trans_head;
    memset(&trans_head, 0, sizeof(trans_head));

    p_msg_buf = (unsigned char *)malloc(recv_size);

    ret = udp_recv_head(sock, &trans_head);
    if (ret != 0) {
        printf("(%s:%d)Recvive trans head fail\n", __func__, __LINE__);
        return -1;
    }

    strncpy(file_name, trans_head.p_name, strlen(trans_head.p_name));
    cfg_value_get(KEY_SHR_PATH_GEN, share_path);
    sprintf(path, "%s/%s", share_path, file_name);
    printf("receive file: %s\n", file_name);

    blk_num = trans_head.blk_num;
    magic_num = trans_head.magic;
    file_len  = trans_head.file_len;

    int re_fd = open(path, O_RDWR | O_CREAT, 0744);

    while(1) {
        trans_head_t head;
        int r_size = sizeof(trans_head_t) + PIECE_SIZE;
        recv_size = udp_recv(sock, (void *)p_msg_buf, r_size, RECV_TIMEOUT);
        if (recv_size < 0) {
            printf("(%s:%d) udp_recv fail\n", __func__, __LINE__);
            return -1;
        }

        memcpy(&head, p_msg_buf, sizeof(head));
        if (head.magic != FT_MAGIC_NUM) {
            continue;
        }

        if (0 != strcmp(head.p_name, file_name)) {
            continue;
        }

        if ((head.len + sizeof(trans_head_t)) != recv_size) {
            printf("(%s:%d)receive date size fail\n", __func__, __LINE__);
            return -1;
        }
        blk_index = head.blk_index;
        
        printf("receive file(%d / %d): %d%%\r", blk_index + 1, blk_num, (total_size * 100) / file_len);
        fflush(stdout);

        lseek(re_fd, PIECE_SIZE * blk_index, SEEK_SET);
        ret = write(re_fd, p_msg_buf + sizeof(trans_head_t), head.len);
        if(ret != head.len) {
            return -1;
        }
        total_size += head.len;
        if(total_size == file_len) {
            printf("\nreceive file finish\n");
            close(re_fd);
            return 0;
        }
    }
}

int udp_recv_file(const char *p_ip, const char *p_file_name)
{
    int     ret = 0;
    int    sock = -1;
    fd_set rfds;  
    char   cmd[5] = {0};

    sock = sock_udp(BROAD_PORT, NULL);
    if (sock < 0) {
        return -1;
    }
    struct sockaddr_in dst_addr, remote_addr;
    socklen_t sock_len = sizeof(struct sockaddr_in);
    dst_addr.sin_family      = AF_INET;
    dst_addr.sin_port        = htons(BROAD_PORT);
    dst_addr.sin_addr.s_addr = inet_addr(p_ip);

    printf("send -r commond\n");
    ret = sendto(sock, (void *)CMD_RECV, CMD_LEN, 0, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
    if (ret <= 0) {
        close(sock);
        return -1;
    }
    printf("send file name: %s\n", p_file_name);
    ret = sendto(sock, (void *)p_file_name, strlen(p_file_name), 0, (struct sockaddr*)&dst_addr, sizeof(dst_addr));
    if (ret <= 0) {
        close(sock);
        return -1;
    }
 
    if(recvfrom(sock, (void *)cmd, CMD_LEN, 0, (struct sockaddr*)&remote_addr, &sock_len) != CMD_LEN) {
        printf("Don't recv -S CMD\n");
        return -1;
    }

    if(strncmp(cmd, CMD_SEND, CMD_LEN)) {
        printf("CMD isn't -S\n");
        return -1;
    }

    ret = udp_recv_save_file(sock);
    if (ret != 0) {
        printf("%s fail(line:%d)\n", __func__, __LINE__);
        return -1;
    }

    close(sock);
    return ret;
}
