#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include "ft_common.h"


char *get_gateway()  
{  
    FILE *fp;  
    char buf[512];  
    char cmd[128];  
    char *tmp;  
    
    char *gateway = (char *)malloc(GATEWAY_LEN);
    if (gateway == NULL)
    {
        return NULL;
    }
    memset(gateway, 0x00, GATEWAY_LEN);

    fp = popen(ROUTECMD, "r");  
    if(NULL == fp)  
    {  
        perror("popen error");  
        return NULL;  
    }  
    while(fgets(buf, sizeof(buf), fp) != NULL)  
    {  
        tmp = buf;  
        while(*tmp && isspace(*tmp))
        { 
            ++tmp;  
        }
        if(strncmp(tmp, "default", strlen("default")) == 0) 
        { 
            break;
        }
    }  
    sscanf(buf, "%s %s %s", tmp, tmp, gateway);         
    printf("default gateway:%s\n", gateway);  
    pclose(fp);  
    return gateway;    
}

int get_host_ip(unsigned int gateway, char **ip)
{
    int sfd, intr;
    struct ifreq buf[16];
    struct ifconf ifc;
    unsigned int tmp_ip = 0;
    char *ip_addr = NULL;
    unsigned int net_mask = 0;
    

    sfd = socket (AF_INET, SOCK_DGRAM, 0); 
    if (sfd < 0)
    {
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (ioctl(sfd, SIOCGIFCONF, (char *)&ifc))
    {
        return -1;
    }

    intr = ifc.ifc_len / sizeof(struct ifreq);
    while (intr > 0)
    {
        ioctl(sfd, SIOCGIFNETMASK, (char *)(&buf[intr])); 
        printf("net mask %s\n", inet_ntoa(((struct sockaddr_in*)(&buf[intr].ifr_netmask))->sin_addr));
        net_mask = ((struct sockaddr_in*)(&buf[intr].ifr_netmask))->sin_addr.s_addr;

        ioctl(sfd, SIOCGIFADDR, (char *)(&buf[intr]));
        printf("ip addr %s\n", inet_ntoa(((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr));
        tmp_ip = ((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr.s_addr;
        ip_addr = inet_ntoa(((struct sockaddr_in*)(&buf[intr].ifr_addr))->sin_addr);
        if ((tmp_ip & net_mask) == (gateway & net_mask))
        {
            *ip = ip_addr;
            break;
        }
        intr--;
    }
    return 0;
}


