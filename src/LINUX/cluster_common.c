#include "cluster_common.h"

#ifdef CLU_MANEGE_DEBUG
void debug_dump_packet(unsigned char *ptr, unsigned short len)
{
    int i = 0;

    printf("Dump packet content, len=%d\n", len);
    while(i < len)
    {
        printf("%02x ", ptr[i]);
        if((i&0xF)==0x7)
            printf(" ");
        if((i&0xF)==0xF)
            printf("\n");
        i++;
    }
    printf("\n\n");
}

void option_debug(unsigned short type, unsigned short len, unsigned char *ptr)
{
    int i = 0;

    printf("Dump Option, type=%d len=%d\n", type, len);
    printf("value:\n", type, len);
    while(i < len - 4)
    {
        printf("%02x ", ptr[i]);
        if((i&0xF)==0x7)
            printf(" ");
        if((i&0xF)==0xF)
            printf("\n");
        i++;
    }
    printf("\n");
}

#else
void debug_dump_packet(unsigned char *ptr, unsigned short len)
{
}

void option_debug(unsigned short type, unsigned short len, unsigned char *ptr)
{
}

#endif

unsigned short get_checksum(unsigned char *ptr, unsigned short len)
{
    int i;
    unsigned int csum=0;

    for(i=0; i<len-1; i+=2)
    {
        csum += (ptr[i]<<8);
        csum += ptr[i+1];
    }
    if(i == (len-1))
        csum += ptr[i];

    csum=(csum>>16)+(csum&0xFFFF);  
    csum+=(csum >> 16);  
    return (unsigned short)(~csum);
}

