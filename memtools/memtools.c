#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

// #define MEM_DEBUG(...)  printf("[MEMTOOLS] "__VA_ARGS__)
#define MEM_DEBUG(...)  

typedef long long int u64;
static void *gpBaseaddr;            // 内存映射基地址，虚拟地址
static uint32_t gpPhyAddrBase;            // 内存映射基地址按页取整
static uint32_t gpPhyOffset;            // 内存映射基地址偏移
static uint32_t gMapSize;
static uint32_t phyAddr;


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
    MEM_DEBUG("physical base addr: %x\n", gpPhyAddrBase);
    MEM_DEBUG("physical offset   : %x\n", gpPhyOffset);

    gpBaseaddr = mmap(0, gMapSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, phyAddr & (~pageMask));
    close(fd);
    if (gpBaseaddr == MAP_FAILED || gpBaseaddr == NULL) {
        return -2;
    }
    MEM_DEBUG("map size          : %x\n", gMapSize);
    MEM_DEBUG("physical map addr : %x\n", gpBaseaddr);
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
        printf("copy to addr      : %x\n", gpBaseaddr + gpPhyOffset + offset);
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
        printf("copy from addr    : %x\n", gpBaseaddr + gpPhyOffset + offset);
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

int writeFile(char *filename, int offset, int size)
{
    if(offset + size > gMapSize){
        printf("Out of range.\n");
        return -0x11;
    }
    FILE *fd = fopen(filename, "wb");
    if(fd == NULL) {
        return -0x12;
    }
    MEM_DEBUG("copy from addr    : %x\n", gpBaseaddr + gpPhyOffset + offset);
    MEM_DEBUG("copy size         : %x\n", size);
    int ret = fwrite((char*)gpBaseaddr + gpPhyOffset + offset, sizeof(unsigned char), size, fd);
    if(ret != size){
        printf("Error: %s\n", strerror(errno));
    }else{
        MEM_DEBUG("Read 0x%x bytes from 0x%x done\n", size, phyAddr + gpPhyOffset + offset);
    }
    sync();
    fclose(fd);
    return 0;
}

int writeMem(char *filename, int offset, int size)
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("File %s not found !!!\n", filename);
        return -0x22;
    }
        
    fseek(file, 0, SEEK_END);
    int fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    int writeMemLen = 0;
    if(gMapSize - offset < fileLen){
        writeMemLen = gMapSize;
    }else{
        writeMemLen = fileLen;
    }

    char *fileStream = (char *)malloc(writeMemLen + 1);
    if (fileStream == NULL) {
        fclose(file);
        return -0x23;
    }else{
        memset(fileStream, 0, writeMemLen + 1);
    }

    int ret = fread(fileStream, 1, writeMemLen, file);
    if (ret < writeMemLen) {
        fclose(file);
        printf("File read bytes error !!!\n");
        return -0x24;
    }
    fclose(file);
    MEM_DEBUG("copy to addr      : %x\n", gpBaseaddr + gpPhyOffset + offset);
    MEM_DEBUG("copy size         : %x\n", writeMemLen);
    memcpy((void*)gpBaseaddr + gpPhyOffset + offset, fileStream, writeMemLen);
    MEM_DEBUG("Write 0x%x bytes to 0x%x done\n", writeMemLen, phyAddr + gpPhyOffset + offset);
    return 0;
}


void initMem(int offset, int size, char value)
{
    MEM_DEBUG("memset %x to addr: %x\n", value, gpBaseaddr + gpPhyOffset + offset);
    memset((void*)gpBaseaddr + gpPhyOffset + offset, value, size);
}


void usage(char *name)
{
    printf("\n");
    printf("\t -m: Specify the physical memory address.\n");
    printf("\t -o: Specify the physical address offset.\n");
    printf("\t -s: Specify the physical address size.\n");
    printf("\t -f: Specify the output/input file.\n");
    printf("\t -d: For dump memory.\n");
    printf("\t -w: For write memory.\n");
    printf("\t -i: Memset value for memory.\n");
    printf("\n");
}

/*
* memdump -m 0x40000000 -s 0x320000 -o mem.bin
*/
int main(int argc, char **argv)
{
    int ch;
    int ret;
    uint32_t size;
    uint32_t offset = 0;
    char filename[128] = {0};
    unsigned flag = 0;
    char value;

    while ((ch = getopt(argc, argv, "hdwm:o:s:f:i:")) != -1) {
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
                case 'd':
                        flag |= 1 << 4;
                        break;
                case 'w':
                        flag |= 1 << 5;
                        break;
                case 'i':
                        if(strstr(optarg, "0x")){
                            value = (uint32_t)strtol(optarg, NULL, 16);
                        }else{
                            value = (uint32_t)strtol(optarg, NULL, 10);
                        }
                        flag |= 1 << 6;
                        break;
        }
    }

    if((flag & 0xff) != 0x1B && (flag & 0xff) != 0x1f 
        && (flag & 0xff) != 0x2f && (flag & 0xff) != 0x2B && (flag & 0x40) != 0x40){
        usage(argv[0]);
        return 0;
    }

    ret = devmemInit(phyAddr, size);
    if(ret){
        return ret;
    }

    if((flag & 0x40) == 0x40){
        initMem(offset, size, value);
        devmemUninit();
        return 0;
    }

    if((flag & 0xf0) == 0x10){
        ret = writeFile(filename, offset, size);
    }else if((flag & 0xf0) == 0x20){
        writeMem(filename, offset, size);
    }

    devmemUninit();
    return 0;

}
