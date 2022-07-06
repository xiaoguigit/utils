#ifndef __PARSE_LOG_H__
#define __PARSE_LOG_H__


/* LOG MSG */
typedef struct log_msg{
	int format;
	int tag;
	int arg[8];
}LOG_MSG;



void write_log_file(char *buf);
int parse_elf(char *file, char *binFile, uint32_t baseOffset);
void find_by_addr(void *pdata, uint32_t baseOffset);

#endif  /* __PARSE_LOG_H__ */