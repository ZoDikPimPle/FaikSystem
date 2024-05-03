#define _CRT_SECURE_NO_WARNINGS // Это подавляет предупреждение о безопасности CRT
#include <stdio.h>
#include <string.h>
#include "disk.h"
#include "ext2.h"
#include "util.h"
#include "sh.h"

sp_block sp;
file_node fn[FNODE_CNT];
dir_node dn;

char disk_buf[DEVICE_BLOCK_SIZE];
char buf[FNAME_SIZE];

void format() // форматируем
{
    printf("Внимание! Форматирование!!! Не выключайте компьютер!!!\n");
    for (int i = 0; i < DATA_BLOCK_CNT; i++) // число блоков данных
    {
        memset(disk_buf, 0, DEVICE_BLOCK_SIZE);
        store(disk_buf, i, DEVICE_BLOCK_SIZE);
    }

    read((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE); // читаем в область суперблока
    sp.magic_num = MAGIC; // записываем в структуру магию
    sp.free_block_count = DATA_BLOCK_CNT - 3 - FNODE_SIZE * FNODE_CNT / DATA_BLOCK_SIZE; // записываем в суперблок число свободных ячеек
    sp.free_inode_count = FNODE_CNT - 1; // записываем в суперблок число свободных инодов
    sp.dir_inode_count = 1; // записываем число инодов
    for (int i = 0; i < 3 + FNODE_SIZE * FNODE_CNT / DATA_BLOCK_SIZE; i++)
    {
        map_set(sp.block_map, i); // задаем битовую карту блоков
    }
    // map_set(sp.inode_map, ROOT_NODE); // задаем битовую карту для корневого каталога
    store((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE); // запиываем корневой каталог на диск

    fn[ROOT_NODE].size = 0; // создаем корневой каталог
    fn[ROOT_NODE].file_type = DIR_T; // тип каталог
    // fn[ROOT_NODE].link = 0;
    fn[ROOT_NODE].block_point[0] = ROOT_BLOCK;
    inode_set((char*)&fn[ROOT_NODE], ROOT_NODE); // задаем иноду
    store((char*)fn, FILE_NODE_BEGIN, FNODE_SIZE * FNODE_CNT);
    // записываем на диск
    printf("Форматирование завершено\n");
}

void boot()
{
    char c;
    read((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE); // читаем супер блок из начала
    if (sp.magic_num == MAGIC) // читаем магию
    {
        printf("Найдена файловая система\n");
        while (1)
        {
            printf("Отформатировать? (y/n)");
            c = getchar();
            if (c == 'y')
            {
                format();
                break;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        printf("Файловая система не найдена в файле или повреждена\n");
        printf("Будет произведено форматирование\n");
        format();
    }
}

int main(int argc, char const* argv[])
{
    SetConsoleOutputCP(1251);
    open_disk();
    boot();
    shell();
    close_disk();
    return 0;
}
