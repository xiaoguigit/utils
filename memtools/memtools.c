#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

typedef long long int u64;
static void *gpBaseaddr;            // 内存映射基地址，虚拟地址
static uint32_t gpPhyAddrBase;            // 内存映射基地址按页取整
static uint32_t gpPhyOffset;            // 内存映射基地址偏移
static uint32_t gMapSize;


void printHexArray(unsigned char *data, int dataLen)
{
    int i;
    char array[2048 * 3] = {0};
    char *arrayPtr = array;

    if (dataLen >= 2048) {
        return;
    }

    for (i=0; i<dataLen; i++) {
        sprintf(arrayPtr, "%02X ", data[i]);
        arrayPtr += 3;
    }

    *arrayPtr = '\0';
    printf("%s\n", array);
}



/*
 * 设备初始化，获取映射地址
 */
int32_t devmemInit(uint32_t phyAddr, uint32_t size)
{
    int32_t fd;
    uint32_t pageSize = 4096;
    uint32_t pageMask = pageSize - 1;

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        return -1;
    }

    gpPhyAddrBase = phyAddr & (~pageMask);
    gpPhyOffset = abs(phyAddr-gpPhyAddrBase);
    size += gpPhyOffset;
    if ((size % pageSize) != 0) {
        gMapSize = ((size / pageSize) + 1) * pageSize;
    } else {
        gMapSize = size;
    }

    gpBaseaddr = mmap(0, gMapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phyAddr & (~pageMask));
    close(fd);
    if (gpBaseaddr == MAP_FAILED || gpBaseaddr == NULL) {
        return -2;
    }
    
    return 0;
}


void devmemUninit(void)
{
    if(gpBaseaddr){
        munmap(gpBaseaddr, gMapSize);
    }
}


/* 从复制数据到内存
 * offset:      基于基地址的偏移值
 * size  :      复制的大小
 * buf   :      缓冲区指针
 */
int copyToMem(int offset, int size, char *buf)
{
    if(offset + size > gMapSize){
        printf("Out of range.\n");
        return -1;
    }
    if(buf){
        memcpy((void*)gpBaseaddr + gpPhyOffset + offset, buf, size);
        return size;
    }

    return 0;
}

/* 从内存复制数据
 * offset:      基于基地址的偏移值
 * size  :      复制的大小
 * buf   :      缓冲区指针
 */
int copyFromMem(int offset, int size, char *buf)
{
    if(offset + size > gMapSize){
        printf("Out of range.\n");
        return -1;
    }
    if(buf){
        memcpy(buf, (void*)gpBaseaddr + gpPhyOffset + offset, size);
        return size;
    }
    return 0;
}

/* 直接访问内存获取数据 */
void *getDataFromOffset(int offset)
{
    return (void *)gpBaseaddr + offset;
}

void writeFile(char *filename, int offset, int size)
{
    if(offset + size > gMapSize){
        printf("Out of range.\n");
        return;
    }
    FILE *fd = fopen(filename, "wb");
    if(fd == NULL) {
        return ;
    }
    fwrite((char*)gpBaseaddr + gpPhyOffset + offset, sizeof(unsigned char), size, fd);
    sync();
    fclose(fd);
}

void usage(char *name)
{
    printf("\n");
    printf("\t -m: Specify the physical memory address.\n");
    printf("\t -o: Specify the physical address offset.\n");
    printf("\t -s: Specify the physical address size.\n");
    printf("\t -f: Specify the output file.\n");
    printf("\n");
}

/*
* memdump -m 0x40000000 -s 0x320000 -o mem.bin
*/
int main(int argc, char **argv)
{
    int ch;
    int ret;
    uint32_t phyAddr;
    uint32_t size;
    uint32_t offset = 0;
    char filename[128] = {0};
    unsigned flag = 0;

    while ((ch = getopt(argc, argv, "hm:o:s:f:")) != -1) {
        switch (ch) 
        {
                case 'h':
                        usage(argv[0]);
                        return 0;
                case 'm':
                        if(strstr(optarg, "0x")){
                            phyAddr = (uint32_t)strtol(optarg, NULL, 16);
                        }else{
                            phyAddr = (uint32_t)strtol(optarg, NULL, 10);
                        }
                        flag |= 1 << 0;
                        break;
                case 's':
                        if(strstr(optarg, "0x")){
                            size = (uint32_t)strtol(optarg, NULL, 16);
                        }else{
                            size = (uint32_t)strtol(optarg, NULL, 10);
                        }
                        flag |= 1 << 1;
                        break;
                case 'o':
                        if(strstr(optarg, "0x")){
                            offset = (uint32_t)strtol(optarg, NULL, 16);
                        }else{
                            offset = (uint32_t)strtol(optarg, NULL, 10);
                        }
                        flag |= 1 << 2;
                        break;
                case 'f':
                        strcpy(filename, optarg);
                        flag |= 1 << 3;
                        break;
        }
    }

    if((flag & 0xf) != 0xB && (flag & 0xf) != 0xf){
        usage(argv[0]);
        return 0;
    }

    ret = devmemInit(phyAddr, size);
    if(ret){
        return ret;
    }

    writeFile(filename, offset, size);

    devmemUninit();
    return 0;

}
