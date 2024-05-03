#ifndef DISK_H 
#define DISK_H 

#define DEVICE_BLOCK_SIZE 512 //размер блока в байтах 

int get_disk_size(); //размер диска 4 мбайт 

int open_disk();

int close_disk();

int disk_read_block(unsigned int block_num, char* buf);

int disk_write_block(unsigned int block_num, char* buf);

#endif 
