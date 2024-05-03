#ifndef EXT2_H 
#define EXT2_H 

#define MAGIC 0xdec0de //Идендификатор фс 
#define SUPER_BLOCK 0 //адрес суперблока  
#define SUPER_BLOCK_SIZE 656 //размер суперблока 
#define ROOT_BLOCK 2 //адрес корневого каталога 
#define DATA_BLOCK_CNT 4096 //число блоков 
#define DATA_BLOCK_SIZE 1024 //размер блока 
#define ROOT_NODE 0 //номер записи корневого каталога 
#define DNODE_SIZE 128 //Размер записи директории 
#define FILE_NODE_BEGIN 3 //номер записи файла 
#define FNODE_CNT 1024 //Число записей файлов 
#define FNODE_SIZE 32 //размер записи файла 
#define FNAME_SIZE 121 //размер имени файла 
#define DIRECT_INDEX_SIZE 6 //размер блока указателей 

typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef struct super_block
{
    int32_t magic_num; //идендификатор фс 
    int32_t free_block_count; //число свободных блоков 
    int32_t free_inode_count; //число свободных инодов 
    int32_t dir_inode_count; //число инодов 
    uint32_t block_map[256]; //карта блоков 
    uint32_t inode_map[32]; //карта инодов 
} sp_block;

typedef struct inode

{
    uint32_t size; //ращмер 
    uint16_t file_type; //тип 
    uint32_t block_point[DIRECT_INDEX_SIZE]; //блоки данных 
} file_node;

typedef struct dir_item //файл в директории 
{
    uint32_t inode_id; //номер иноды 
    uint8_t type; //тип 
    uint16_t valid; //файл существует 
    char name[FNAME_SIZE]; //имя 
} dir_node;

typedef enum file_type //перечисление типов 
{
    DIR_T, //директория 
    FILE_T //файл 
} FileType;

#endif 
