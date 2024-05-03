#define _CRT_SECURE_NO_WARNINGS // Это подавляет предупреждение о безопасности CRT#include <stdio.h>
#include <string.h>
#include "ext2.h"
#include "disk.h"
#include "util.h"

extern sp_block sp;
extern dir_node dn;

extern char disk_buf[DEVICE_BLOCK_SIZE];

void map_set(uint32_t* map, uint32_t num) {
    int q, r;
    q = num / 32;
    r = num % 32;
    map[q] |= 1 << (31 - r); //записывем в карту биты занятости блоков
}

void map_reset(uint32_t* map, uint32_t num) {
    int q, r;
    q = num / 32;
    r = num % 32;
    map[q] &= ~(1 << (31 - r)); //очищаем бит занятости блока
}

int map_test(uint32_t* map, uint32_t num) {
    int q, r;
    q = num / 32;
    r = num % 32;
    return map[q] >> (31 - r) & 1; //возврат занятости блока
}

void store(char* buffer, uint32_t num, uint32_t len) {
    if (len <= DEVICE_BLOCK_SIZE) {
        disk_write_block(num, buffer); //записываем блоки на диск если длина меньше размера блока
    }
    else {
        for (int i = 0; i <= len / DEVICE_BLOCK_SIZE; i++) {
            disk_write_block(num + i, buffer + i * DEVICE_BLOCK_SIZE); //если больше, то записываем последовательно следующий блок со смещением в 512
        }
    }
}

void read(char* buffer, uint32_t num, uint32_t len) { //читаем блок в буффер откуда прочитать и сколько
    if (len <= DEVICE_BLOCK_SIZE) { //если рамер читаемого меньше размера блока
        disk_read_block(num, disk_buf); //то читаем блок с диска
        memcpy(buffer, disk_buf, len); //сохраяняем в буффер
    }
    else {
        for (int i = 0; i <= len / DEVICE_BLOCK_SIZE; i++) { //читаем блоки
            disk_read_block(num + i, disk_buf); //читаем блок с диска
            if (i == len / DEVICE_BLOCK_SIZE) { //если длина меньше блока
                memcpy(buffer + i * DEVICE_BLOCK_SIZE, disk_buf, len - i * DEVICE_BLOCK_SIZE); //записываем в буффер блоки начиная с нужной позиции
            }
            else {
                memcpy(buffer + i * DEVICE_BLOCK_SIZE, disk_buf, DEVICE_BLOCK_SIZE); //записываем в буффер последний блок
            }
        }
    }
}

void inode_get(char* buffer, uint32_t num) {
    uint32_t q, r;
    q = num / (DATA_BLOCK_SIZE / FNODE_SIZE) + 3; //номер блока
    r = num % (DATA_BLOCK_SIZE / FNODE_SIZE); //смещение внутри блока
    read(disk_buf, q, DEVICE_BLOCK_SIZE); //читаем блок с диска
    memcpy(buffer, disk_buf + r * FNODE_SIZE, FNODE_SIZE); //копируем в буффер иноду
}

void inode_set(char* buffer, uint32_t num) {
    uint32_t q, r;
    q = num / (DATA_BLOCK_SIZE / FNODE_SIZE) + 3; //номер блока инода
    r = num % (DATA_BLOCK_SIZE / FNODE_SIZE); //номер блока данных
    read(disk_buf, q, DEVICE_BLOCK_SIZE); //читаем блок с диска
    memcpy(disk_buf + r * FNODE_SIZE, buffer, FNODE_SIZE); //копируем в дисковый буффер из буффера
    store(disk_buf, q, DEVICE_BLOCK_SIZE); //записываем данные на диск
}

int lookup(char* buffer, int type, uint32_t* blocks) { //поиск нужного блока
    for (int i = 0; i < DIRECT_INDEX_SIZE; i++) { //проходим по директории
        if (blocks[i] == 0) { //Если блоки файла или директории пусты, выходим
            return -1;
        }
        read(disk_buf, blocks[i], DEVICE_BLOCK_SIZE); //читаем из буфера
        for (int j = 0; j < DEVICE_BLOCK_SIZE / DNODE_SIZE; j++) {
            memcpy((char*)&dn, disk_buf + j * DNODE_SIZE, DNODE_SIZE);
            if (memcmp(dn.name, buffer, strlen(buffer)) == 0 && dn.type == type && dn.valid == 1) { //если блок соответствует нужному типу и занят, то возврящаем иноду
                return dn.inode_id;
            }
        }
        if (i == DIRECT_INDEX_SIZE - 1) {
            return -1;
        }
    }
}

int dir_set(char* buffer, uint32_t* blocks) {
    dir_node dnode;
    for (int i = 0; i < DIRECT_INDEX_SIZE; i++) {
        if (blocks[i] == 0) {
            read((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE); //читаем суперблок
            blocks[i] = DATA_BLOCK_CNT - sp.free_block_count; //уменьшаем число свободных блоков
            sp.free_block_count--;
            map_set(sp.block_map, blocks[i]); //задаем новую карту занятости
            store((char*)&sp, SUPER_BLOCK, SUPER_BLOCK_SIZE); //записываем новый суперблок
        }
        read(disk_buf, blocks[i], DEVICE_BLOCK_SIZE); //читаем блоки директории
        for (int j = 0; j < DEVICE_BLOCK_SIZE / DNODE_SIZE; j++) {
            memcpy((char*)&dnode, disk_buf + j * DNODE_SIZE, DNODE_SIZE); //читаем структуру информации о директории
            if (dnode.valid == 0) { //если занятость 0
                memcpy(disk_buf + j * DNODE_SIZE, buffer, DNODE_SIZE); //записываем структуру
                store(disk_buf, blocks[i], DEVICE_BLOCK_SIZE); //записываем на диск
                return 1;
            }
        }
        if (i == DIRECT_INDEX_SIZE - 1) {
            printf("Недостаточно места\n");
        }
    }
}
