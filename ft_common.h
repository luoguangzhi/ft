#ifndef _FT_COMMON_H
#define _FT_COMMON_H

#include <stdio.h>
/*****************************************************************************/
#define KEY_NAME_GEN      (0)
#define KEY_SHR_PATH_GEN  (1)

#define BROAD_PORT        (10086)
#define TRANS_PORT        (10010)

#define CMD_LEN           (2)

#define CMD_GET_IP        "-I"
#define CMD_SEND          "-S"
#define CMD_RECV          "-R"

#define FT_MAGIC_NUM      (0x55)
/******************************************************************************/

typedef struct trans_head {
    unsigned int magic;
    char         p_name[128];
    unsigned int blk_num;
    unsigned int blk_index;
    unsigned int file_len;
    unsigned int len;
} trans_head_t;

/*****************************************************************************/

int get_dst_addr(char *p_name, char *p_ip, size_t len);

char *cfg_value_get(int gen, char *p_value);

int cfg_value_set(int gen, const char *p_value);

int send_file(const char *p_ip, const char *p_file_path);

int recv_file(const char *p_ip, const char *p_file_path);

#endif /* _FT_COMMON_H */