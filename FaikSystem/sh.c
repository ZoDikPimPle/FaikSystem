#define _CRT_SECURE_NO_WARNINGS // Это подавляет предупреждение о безопасности CRT
#include <stdio.h>
#include <string.h>
#include "ext2.h"
#include "disk.h"
#include "util.h"
#include "sh.h"

extern sp_block sp;
extern file_node fn[FNODE_CNT];
extern dir_node dn;

extern char disk_buf[DEVICE_BLOCK_SIZE];
extern char buf[FNAME_SIZE];

char* ls_cmd = "ls";
char* mkdir_cmd = "mkdir";
char* touch_cmd = "touch";
char* cp_cmd = "cp";
char* shutdown_cmd = "qq";
char* superblock = "superblock";
char* rf = "rf";
char* wf = "wf";

int curdir = ROOT_NODE;
char dirname[FNAME_SIZE];

int ls() {
    int dir, q, r;

    uint32_t* blocks;
    dir = ROOT_NODE; // начинаем просмотр от корневого каталога
    if (getchar() != '\n') { // Если строка каталога не пуста
        scanf("%s", buf); // считываем строку
        char* head = buf;
        char* tail = buf;
        while (strchr(head, '/') != NULL) { // если находим признак директории
            while (*head != '/') // проспатриваем до слеша
                head++;
            *head++ = 0;
            dir = lookup(tail, DIR_T, fn[dir].block_point); // ищем директорию в каталоге
            if (dir == -1) {
                printf("Не найдено директории с именем %s\n", tail); // если директория не найдена, выходим
                return -1;
            }
            tail = head;
        }
        dir = lookup(head, DIR_T, fn[dir].block_point); // продолжаем поиск директорий
        if (dir == -1) {
            printf("Не найдено директории с именем %s\n", tail);
            return -1;
        }
    }
    inode_get((char*)&fn[dir], dir);
    blocks = fn[dir].block_point; // берем номера кластера кластеров
    q = fn[dir].size / (DEVICE_BLOCK_SIZE / DNODE_SIZE);
    r = fn[dir].size % (DEVICE_BLOCK_SIZE / DNODE_SIZE);
    for (int i = 0; i <= q; i++) {
        read(disk_buf, blocks[i], DEVICE_BLOCK_SIZE); // читаем последовательно блоки данных
        for (int j = 0; j < DEVICE_BLOCK_SIZE / DNODE_SIZE; j++) {
            memcpy((char*)&dn, disk_buf + j * DNODE_SIZE, DNODE_SIZE); // читаем структуру директории
            if (i == q && j == r) {
                if (i != 0 || j != 0) {
                    printf("\n");
                }
                break;
            }
            printf("%s ", dn.name);
            if (dn.type) {
                printf("файл ");
            }
            else {
                printf("директория ");
            }
            printf("\n");
        }
    }
}

int mkdir() {
    int fn_num, db_num, dir;
    if (getchar() != '\n') {
        do {
            scanf("%s", buf);
            char* head = buf;
            char* tail = buf;
            dir = ROOT_NODE;
            while (strchr(head, '/') != NULL) {
                while (*head != '/')
                    head++;
                *head++ = 0;
                inode_get((char*)&fn[dir], dir);
                dir = lookup(tail, DIR_T, fn[dir].block_point);
                if (dir == -1) {
                    printf("Не найдено директории с именем %s\n", tail);
                    return -1;
                }
                tail = head;
            }
            read((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            db_num = DATA_BLOCK_CNT - sp.free_block_count;
            fn_num = FNODE_CNT - sp.free_inode_count;
            sp.free_block_count--;
            sp.free_inode_count--;
            store((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);

            dn.inode_id = fn_num;
            dn.type = DIR_T;
            dn.valid = 1;
            memset(dn.name, 0, FNAME_SIZE);
            memcpy(dn.name, head, strlen(head));

            inode_get((char*)&fn[dir], dir);

            fn[dir].size++;
            dir_set((char*)&dn, fn[dir].block_point);
            inode_set((char*)&fn[dir], dir);

            fn[fn_num].size = 0;
            fn[fn_num].file_type = DIR_T;
            fn[fn_num].block_point[0] = db_num;
            inode_set((char*)&fn[fn_num], fn_num);

            read((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            sp.dir_inode_count++;
            map_set(sp.inode_map, fn_num);
            map_set(sp.block_map, db_num);
            store((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
        } while (getchar() != '\n');
    }
    else {
        printf("Используйте: mkdir <dir>\n");
    }
}

int touch() {
    int fn_num, db_num, dir;
    if (getchar() != '\n') { // проверяем если строка пуста
        do {
            scanf("%s", buf);
            char* head = buf;
            char* tail = buf;
            dir = ROOT_NODE;
            while (strchr(head, '/') != NULL) {
                while (*head != '/') // пока не встретился признак каталога
                    head++;
                *head++ = 0;
                inode_get((char*)&fn[dir], dir);
                dir = lookup(tail, DIR_T, fn[dir].block_point); // проверяем наличие директории
                if (dir == -1) {
                    printf("Нет директории с именем %s\n", tail);
                    return -1;
                }
                tail = head;
            }
            read((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE); // читаем супер блока
            db_num = DATA_BLOCK_CNT - sp.free_block_count;
            fn_num = FNODE_CNT - sp.free_inode_count;


            dn.inode_id = fn_num;
            dn.valid = 1;
            dn.type = FILE_T;
            memset(dn.name, 0, FNAME_SIZE);
            memcpy(dn.name, head, strlen(head));

            inode_get((char*)&fn[dir], dir);
            fn[dir].size++;
            dir_set((char*)&dn, fn[dir].block_point);
            inode_set((char*)&fn[dir], dir);

            fn[fn_num].size = 0;
            fn[fn_num].file_type = FILE_T;
            fn[fn_num].block_point[0] = db_num;
            inode_set((char*)&fn[fn_num], fn_num);

            sp.free_block_count--;
            sp.free_inode_count--;
            sp.dir_inode_count++;
            map_set(sp.inode_map, fn_num);
            map_set(sp.block_map, db_num);
            store((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
        } while (getchar() != '\n');
    }
    else {
        printf("Используйте: touch <file>\n");
    }
}

int readf() {
    int dir, q, r;
    uint32_t* blocks;
    dir = ROOT_NODE; // начинаем просмотр от корневого каталога
    // inode_get((char *)&fn[dir], dir);
    if (getchar() != '\n') { // Если строка каталога не пуста
        scanf("%s", buf); // считываем строку
        char* head = buf;
        char* tail = buf;
        while (strchr(head, '/') != NULL) { // если находим признак директории
            while (*head != '/') // проспатриваем до слеша
                head++;
            *head++ = 0;
            dir = lookup(tail, DIR_T, fn[dir].block_point); // ищем директорию в каталоге
            if (dir == -1) {

                printf("Не найдено директории с именем %s\n", tail); // если директория не найдена выходим
                return -1;
            }
            tail = head;
        }
        dir = lookup(head, FILE_T, fn[dir].block_point); // продолжаем поиск директорий
        if (dir == -1) {
            printf("Не найдено файла с именем %s\n", tail);
            return -1;
        }
    }
    inode_get((char*)&fn[dir], dir);

    blocks = fn[dir].block_point; // берем номера кластера кластеров
    q = fn[dir].size / (DEVICE_BLOCK_SIZE / DNODE_SIZE);
    r = fn[dir].size % (DEVICE_BLOCK_SIZE / DNODE_SIZE);
    // for (int i = 0; i <= q; i++)
    // {
    read(disk_buf, blocks[0], DEVICE_BLOCK_SIZE); // читаем последовательно блоки данных
    // for (int j = 0; j < DEVICE_BLOCK_SIZE / DNODE_SIZE; j++)
    // {
    printf("%s \n", disk_buf);
    // }
    // }
}

int wrf() {
    int dir, q, r;
    uint32_t* blocks;
    dir = ROOT_NODE; // начинаем просмотр от корневого каталога
    // inode_get((char *)&fn[dir], dir);
    if (getchar() != '\n') { // Если строка каталога не пуста
        scanf("%s", buf); // считываем строку
        char* head = buf;
        char* tail = buf;
        while (strchr(head, '/') != NULL) { // если находим признак директории
            while (*head != '/') // проспатриваем до слеша
                head++;
            *head++ = 0;
            dir = lookup(tail, DIR_T, fn[dir].block_point); // ищем директорию в каталоге
            if (dir == -1) {

                printf("Не найдено директории с именем %s\n", tail); // если директория не найдена выходим
                return -1;
            }
            tail = head;
        }
        dir = lookup(head, FILE_T, fn[dir].block_point); // продолжаем поиск директорий
        if (dir == -1) {
            printf("Не найдено файла с именем %s\n", tail);
            return -1;
        }
    }
    inode_get((char*)&fn[dir], dir);
    char bufstr[DEVICE_BLOCK_SIZE]; // Инициализация переменной "bufstr"
    scanf("%s", bufstr); // Считывание строки в "bufstr"
    fn[dir].size = fn[dir].size + strlen(bufstr);
    blocks = fn[dir].block_point; // берем номера кластера кластеров
    q = fn[dir].size / (DEVICE_BLOCK_SIZE / DNODE_SIZE);
    r = fn[dir].size % (DEVICE_BLOCK_SIZE / DNODE_SIZE);
    for (int i = 0; i <= q; i++) {
        store(bufstr, blocks[0], strlen(bufstr));
    }
    inode_set((char*)&fn[dir], dir);
}


int cp() {
    int src, dst, dir;
    int fn_num, db_num;
    if (getchar() != '\n') {
        scanf("%s", buf);
        char* head = buf;
        char* tail = buf;
        dir = ROOT_NODE;
        while (strchr(head, '/') != NULL) {
            while (*head != '/')
                head++;
            *head++ = 0;
            inode_get((char*)&fn[dir], dir);
            dir = lookup(tail, DIR_T, fn[dir].block_point);
            if (dir == -1) {
                printf("Нет директории с именем %s\n", tail);
                return -1;
            }
            tail = head;

        }
        inode_get((char*)&fn[dir], dir);
        src = lookup(head, FILE_T, fn[dir].block_point);
        if (src == -1) {
            printf("Нет директории с именем %s\n", head);
            return -1;
        }
        else {
            if (getchar() != '\n') {
                scanf("%s", buf);
                head = buf;
                tail = buf;
                dir = ROOT_NODE;
                while (strchr(head, '/') != NULL) {
                    while (*head != '/')
                        head++;
                    *head++ = 0;
                    inode_get((char*)&fn[dir], dir);
                    dir = lookup(tail, DIR_T, fn[dir].block_point);
                    if (dir == -1) {
                        printf("Нет директории с именем %s\n", tail);
                        return -1;
                    }
                    tail = head;
                }
                read((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
                fn_num = FNODE_CNT - sp.free_inode_count;

                memset((char*)&dn, 0, sizeof(dn));
                dn.inode_id = fn_num;
                dn.valid = 1;
                dn.type = FILE_T;
                memcpy(dn.name, head, strlen(head));

                inode_get((char*)&fn[dir], dir);
                fn[dir].size++;
                dir_set((char*)&dn, fn[dir].block_point);
                inode_set((char*)&fn[dir], dir);

                fn[fn_num].size = 0;
                fn[fn_num].file_type = FILE_T;
                memcpy(fn[fn_num].block_point, fn[src].block_point, DIRECT_INDEX_SIZE);
                inode_set((char*)&fn[fn_num], fn_num);

                sp.free_inode_count--;
                sp.dir_inode_count++;

                map_set(sp.inode_map, fn_num);
                store((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE);
            }
            else {
                printf("Исполтзуйте: cp <src> <dst>\n");
            }
        }
    }
    else {
        printf("Исполтзуйте: cp <src> <dst>\n");
    }
}

void readsb() {
    FILE* fp;
    sp_block sb;
    if ((fp = fopen("disk", "r")) != NULL) {
        fread(&sb, sizeof(sb), 1, fp);
        printf("magic_num %d \n", sb.magic_num);
        printf("free_block_count %d \n", sb.free_block_count);
        printf("free_inode_count %d \n", sb.free_inode_count);
        printf("dir_inode_count %d \n", sb.dir_inode_count);
        fclose(fp);
    }
}

void shell() {
    while (1) {
        memset(buf, 0, sizeof(buf));
        fflush(stdin);
        printf(">>>");
        if (scanf("%s", buf) != EOF) {
            if (strcmp(buf, ls_cmd) == 0) {
                ls();
            }
            else if (strcmp(buf, mkdir_cmd) == 0) {
                mkdir();
            }
            else if (strcmp(buf, touch_cmd) == 0) {
                touch();

            }
            else if (strcmp(buf, cp_cmd) == 0) {
                cp();
            }
            else if (strcmp(buf, shutdown_cmd) == 0) {
                break;
            }
            else if (strcmp(buf, superblock) == 0) {
                readsb();
            }
            else if (strcmp(buf, rf) == 0) {
                readf();
            }
            else if (strcmp(buf, wf) == 0) {
                wrf();
            }
            else {
                printf("Неизвестная команда: %s\n\n", buf);
                printf("Используйте: mkdir <dir> для создания папки \n");
                printf("Используйте: ls <dir> для просмотра содержимого директории \n");
                printf("Используйте: touch <dir> для создания файла\n");
                printf("Исполтзуйте: cp <src> <dst> для копирования файла \n");
                printf("Используйте: superblock для просмотра состояния суперблока\n");
                printf("Используйте: qq для выхода\n");
            }
        }
    }
}
