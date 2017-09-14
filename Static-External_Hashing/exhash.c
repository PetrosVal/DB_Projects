#include "BF.h"
#include "exhash.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_RECORDS (BLOCK_SIZE - 2*sizeof(int))/sizeof(Record)

unsigned int key(EH_info* header_info, Record* record) {
    unsigned int key = 0, i;
    if (!strcmp(header_info->attrName, "id"))
        key = record->id;
    else if (!strcmp(header_info->attrName, "name"))
        for (i = 0; i < strlen(record->name); i++)
            key += record->name[i];
    else if (!strcmp(header_info->attrName, "surname"))
        for (i = 0; i < strlen(record->surname); i++)
            key += record->surname[i];
    else if (!strcmp(header_info->attrName, "city"))
        for (i = 0; i < strlen(record->city); i++)
            key += record->city[i];
    else
        return -1;
    return key;
}


int power(int x, int y) {
    int i, x1=1;
    for (i = 0; i < y; i++) {
        x1 *= x;
    }
    return x1;
}


unsigned int hash(unsigned int x, int depth) {
    x = x % power(2, depth);
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
    x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
    x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
    x = ((x >> 16) | (x << 16));
    x = x >> (32 - depth);
    return x;
}


int EH_CreateIndex(char *fileName, char* attrName, char attrType, int attrLength, int depth) {

    int fileDesc, i, counter=0, nbytes;
    long int size, buckets;
    int *local_depth;
    unsigned short block_number;
    void* block;
    unsigned short* array;

    if ((fileDesc = BF_CreateFile(fileName)) < 0) {
        BF_PrintError("EH_CreateIndex, BF_CreateFile");
        return -1;
    }
    if (BF_OpenFile(fileName) < 0) {
        BF_PrintError("EH_CreateIndex, BF_OpenFile");
        return -1;
    }
    if (BF_AllocateBlock(fileDesc) < 0) {
        BF_PrintError("EH_CreateIndex, BF_AllocateBlock");
        return -1;
    }
    if (BF_ReadBlock(fileDesc, 0, &block) < 0) {
        BF_PrintError("EH_CreateIndex, BF_ReadBlock");
        return -1;
    }

    buckets = power(2, depth);
    size = buckets*sizeof(unsigned short);

    if ((array = malloc(size)) == NULL) {
        perror("malloc");
        return -1;
    }

    memcpy(&block[HASH_TYPE_POSITION], EXTENDIBLE, strlen(EXTENDIBLE)+1);
    memcpy(&block[ATTR_TYPE_POSITION], &attrType, 1);
    memcpy(&block[ATTR_TYPE_POSITION+1], "\0", 1);
    memcpy(&block[ATTR_NAME_POSITION], attrName, strlen(attrName)+1);
    memcpy(&block[ATTR_LENGTH_POSITION], &attrLength, sizeof(int));
    memcpy(&block[DEPTH_POSITION], &depth, sizeof(int));
    block_number = buckets + 1;
    memcpy(&block[TABLE_POSITION], &block_number, sizeof(unsigned short));
    if (BF_WriteBlock(fileDesc, 0) < 0) {
        BF_PrintError("EH_CreateIndex, BF_WriteBlock");
        return -1;
    }

    for (i = 0; i < buckets; i++) {
        if (BF_AllocateBlock(fileDesc) < 0) {
            BF_PrintError("EH_CreateIndex, BF_AllocateBlock");
            return -1;
        }
        block_number = BF_GetBlockCounter(fileDesc)-1;
        if (BF_ReadBlock(fileDesc, block_number, &block) < 0) {
            BF_PrintError("EH_CreateIndex, BF_ReadBlock");
            return -1;
        }
        local_depth = &block[LOCAL_DEPTH_POSITION];
        (*local_depth) = depth;
        array[i] = block_number;
        if (BF_WriteBlock(fileDesc, i) < 0) {
            BF_PrintError("HT_CreateIndex, BF_WriteBlock");
            return -1;
        }
    }

    while (size > 0) {
        if (BF_AllocateBlock(fileDesc) < 0) {
            BF_PrintError("EH_CreateIndex, BF_AllocateBlock");
            return -1;
        }
        if ((block_number = BF_GetBlockCounter(fileDesc)-1) < 0) {
            BF_PrintError("EH_CreateIndex, BF_GetBlockCounter");
            return -1;
        }
        memcpy(&block[BLOCK_SIZE - sizeof(unsigned short)], &block_number, sizeof(int));
        if (BF_ReadBlock(fileDesc, block_number, &block) < 0) {
            BF_PrintError("EH_CreateIndex, BF_ReadBlock");
            return -1;
        }
        nbytes = (size < BLOCK_SIZE - sizeof(unsigned short)) ? size : BLOCK_SIZE - sizeof(unsigned short);
        memcpy(&block[0], array, nbytes);
        array += nbytes;
        size -= nbytes;
        if (BF_WriteBlock(fileDesc, block_number) < 0) {
            BF_PrintError("HT_CreateIndex, BF_WriteBlock");
            return -1;
        }
    }

    return 0;
}


EH_info* EH_OpenIndex(char *fileName) {
    int fileDesc, table_block, i, nbytes;
    long int size;
    unsigned short block_number;
    unsigned short* table;
    void* block;
    char* str;
    EH_info* info;

    if ((fileDesc = BF_OpenFile(fileName)) < 0) {
        BF_PrintError("EH_CreateIndex, BF_OpenFile");
        return NULL;
    }

    if (BF_ReadBlock(fileDesc, HEADER_BLOCK, &block) < 0) {
        BF_PrintError("EH_CreateIndex, BF_ReadBlock");
        return -1;
    }

    info = malloc(sizeof(EH_info));
    if (info == NULL) {
        perror("malloc");
        return NULL;
    }
    info->attrName = malloc(32*sizeof(char));
    if (info->attrName == NULL) {
        perror("malloc");
        free(info);
        return NULL;
    }

    str = &block[HASH_TYPE_POSITION];
    if (strcmp(str, EXTENDIBLE)) {
        free(info->attrName);
        free(info);
        return NULL;
    }

    info->fileDesc = fileDesc;

    str = &block[ATTR_TYPE_POSITION];
    info->attrType = str[0];
    if (info->attrType != CHAR && info->attrType != INT) {
        free(info->attrName);
        free(info);
        return NULL;
    }

    str = &block[ATTR_NAME_POSITION];
    strcpy(info->attrName, str);
    

    info->attrLength = *((int*)&block[ATTR_LENGTH_POSITION]);
    if (strlen(info->attrName) != info->attrLength) {
        free(info->attrName);
        free(info);
        return NULL;
    }

    info->depth = *((int*)&block[DEPTH_POSITION]);
    if (info->depth <= 0) {
        free(info->attrName);
        free(info);
        return NULL;
    }

    table_block = *((int*)&block[TABLE_POSITION]);
    //printf("table_block: %d\n", table_block);
    if (table_block <= 0) {
        free(info->attrName);
        free(info);
        return NULL;
    }
    if (BF_ReadBlock(fileDesc, table_block, &block) < 0) {
        BF_PrintError("EH_CreateIndex, BF_ReadBlock");
        return -1;
    }
    size = sizeof(unsigned short) * power(2, info->depth);
    
    if ((info->table = malloc(size)) == NULL) {
        free(info->attrName);
        free(info);
        return NULL;
    }
    table = info->table;

    while (size > 0) {
        nbytes = (size < BLOCK_SIZE - sizeof(unsigned short)) ? size : BLOCK_SIZE - sizeof(unsigned short);
        memcpy(table, &block[0], nbytes);
        block_number = *((unsigned short*)&block[BLOCK_SIZE - sizeof(unsigned short)]);
        if (BF_ReadBlock(fileDesc, block_number, &block) < 0) {
            BF_PrintError("EH_CreateIndex, BF_ReadBlock");
            return -1;
        }
        table += nbytes;
        size -= nbytes;
    }


    return info;
}



int EH_CloseIndex(EH_info* header_info) {

    if (BF_CloseFile(header_info->fileDesc) < 0) {
        BF_PrintError("EH_CloseIndex, BF_CloseFile");
        return -1;
    }
    free(header_info->attrName);
    free(header_info->table);
    free(header_info);
    header_info = NULL;
    
    return 0;
}

int EH_InsertEntry(EH_info *header_info, Record record) {
    int block_number, newblock_number, i, flag, temp_counter, j, to_change;
    long int buckets, size;
    unsigned int hash_value, key_value;
    unsigned short *table;
    void *block, *block1;
    void *newblock;
    int *counter, *newcounter, *local_depth, *local_depth1, *counter1;
    Record* r;
    Record* temp_array;

    /*printf("AAAA\n");
    for (j = 1; j < BF_GetBlockCounter(header_info->fileDesc); j++)
    {
        if (j == 2) continue;
        BF_ReadBlock(header_info->fileDesc, j, &block);
        local_depth = &block[LOCAL_DEPTH_POSITION];
        counter = &block[COUNTER_POSITION];
        printf("block_number: %d local_depth: %d counter: %d\n", j, *local_depth, *counter);
    }
    getchar();*/
    
    //printf("block counter: %d\n", BF_GetBlockCounter(header_info->fileDesc));
    //printf("depth: %d\n", header_info->depth);
    key_value = key(header_info, &record);
    //printf("key: %d\n", key_value);
    hash_value = hash(key_value, header_info->depth);
    //printf("hash: %d\n", hash_value);
    block_number = header_info->table[hash_value];
    //printf("block: %d\n", block_number);
    if (BF_ReadBlock(header_info->fileDesc, block_number, &block) < 0) {
        BF_PrintError("EH_CreateIndex, BF_ReadBlock");
        return -1;
    }

    counter = &block[COUNTER_POSITION];
    //printf("counter: %d\n\n", *counter);
    //EH_GetAllEntries(*header_info, "A");
    if (*counter == MAX_RECORDS) {
        //puts("counter == MAX_RECORDS");

        flag = 0;
        for (i = 0; i < MAX_RECORDS; i++) {
            r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
            if (key(header_info, r) != key_value) {
                flag = 1;
                break;
            }
        }
        if (!flag) {
            printf("not enough space for:\n");
            printf("%d %s %s %s\n", record.id, record.name, record.surname, record.city);
            return 0;
        }

        local_depth = &block[LOCAL_DEPTH_POSITION];
        if ((*local_depth == header_info->depth)) {
            //puts("\tlocal depth == depth");

            buckets = power(2, header_info->depth);
            size = buckets*sizeof(unsigned short);
            //printf("2*size = %ld\n", 2*size);
            if ((table = malloc(2*size)) == NULL) {
                perror("malloc");
                return -1;
            }

            /*printf("table before:\n");
            for (i = 0; i < power(2, header_info->depth); i++)
                printf("[%d]=%d,", i, header_info->table[i]);
            printf("\n\n");*/

            for (i = 0; i < buckets; i++) {
                table[2*i] = header_info->table[i];
                table[2*i+1] = header_info->table[i];
            }
            (header_info->depth)++;

            /*printf("table after:\n");
            for (i = 0; i < power(2, header_info->depth); i++)
                printf("[%d]=%d,", i, table[i]);
            printf("\n\n");*/

            free(header_info->table);
            header_info->table = table;
            if (EH_InsertEntry(header_info, record) == -1)
                return -1;
        }
        else if (*local_depth < header_info->depth) {
            /*puts("\tlocal depth < depth");
            printf("\nbefore:\nblock %d\n", block_number);
            for (i = 0; i < *counter; i++) {
                r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
                //if (!strcmp(r->city, value))
                    printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
            }
            printf("\n");
            printf("\t+\n\n%d %s %s %s\n\n\t=\n\nbreak bucket:\n\n", record.id, record.name, record.surname, record.city);*/

            /*for (j = 1; j < BF_GetBlockCounter(header_info->fileDesc); j++)
            {
                if (j == 9) continue;
                block1 = BF_ReadBlok[j];
                local_depth1 = &block1[LOCAL_DEPTH_POSITION];
                counter1 = &block1[COUNTER_POSITION];
                printf("block_number: %d local_depth: %d counter: %d\n", j, *local_depth1, *counter1);
            }*/

            if (BF_AllocateBlock(header_info->fileDesc) < 0) {
                BF_PrintError("EH_CreateIndex, BF_AllocateBlock");
                return -1;
            }
            if ((newblock_number = BF_GetBlockCounter(header_info->fileDesc)-1) < 0) {
                BF_PrintError("EH_CreateIndex, BF_GetBlockCounter");
                return -1;
            }
            if (BF_ReadBlock(header_info->fileDesc, newblock_number, &newblock) < 0) {
                BF_PrintError("EH_CreateIndex, BF_ReadBlock");
                return -1;
            }
            while (hash_value != -1 && header_info->table[hash_value] == block_number)
                hash_value--;
            hash_value++;
            
            int to_change = hash_value;
            while (header_info->table[hash_value] == block_number)
                hash_value++;
            to_change = hash_value - to_change;
            to_change >>= 1;
            (*local_depth)++;

            /*printf("\n");
            for (i = 0; i < power(2, header_info->depth); i++)
                printf("[%d]=%d,", i, header_info->table[i]);
            printf("\n");

            printf("block number: %d\n", block_number);
            printf("power(2, %d - %d) = %d\n", header_info->depth, *local_depth, power(2, header_info->depth - *local_depth));*/
            for (i = 0; i < to_change; i++) {
                //printf("newblock_number: %d\n", newblock_number);
                //printf("local_depth: %d\n", *local_depth);
                //printf("hash_value: %d\n", hash_value);
                hash_value--;
                /*printf("hash_value: %d\n", hash_value);
                if (hash_value <= 0) {
                    printf("hash_value <= 0!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    EH_GetAllEntries(*header_info, "A");
                }*/
                header_info->table[hash_value] = newblock_number;
            }
            memcpy(&newblock[LOCAL_DEPTH_POSITION], local_depth, sizeof(int));
            newcounter = &newblock[COUNTER_POSITION];
            (*newcounter) = 0;
            
            if ((temp_array = malloc((*counter)*sizeof(Record))) == NULL) {
                perror("malloc");
                return -1;
            }
            for (i = 0; i < *counter; i++) { //memcpy όλα τα records
                r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
                temp_array[i] = *r;
            }
            memset(&block[FIRST_RECORD_POSITION], 0, (*counter)*sizeof(Record));
            temp_counter = *counter;
            (*counter) = 0;
            for (i = 0; i < temp_counter; i++) {
                //if (header_info->depth == 7) getchar();
                r = &temp_array[i];
                //printf("putting %d %s %s %s in:\n", r->id, r->name, r->surname, r->city);
                key_value = key(header_info, r);
                if (header_info->table[hash(key_value, header_info->depth)] == newblock_number) {
                    memcpy(&newblock[FIRST_RECORD_POSITION + (*newcounter)*sizeof(Record)], r, sizeof(Record));
                    (*newcounter)++;
                    /*printf("\nblock %d, counter: %d\n", newblock_number, *newcounter);
                    for (j = 0; j < *newcounter; j++) {
                        r = &newblock[FIRST_RECORD_POSITION + j*sizeof(Record)];
                        printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
                    }*/
                }
                else {
                    memcpy(&block[FIRST_RECORD_POSITION + (*counter)*sizeof(Record)], r, sizeof(Record));
                    (*counter)++;
                    /*printf("\nblock %d, counter: %d\n", block_number, *counter);
                    for (j = 0; j < *counter; j++) {
                        r = &block[FIRST_RECORD_POSITION + j*sizeof(Record)];
                        printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
                    }*/
                }
            }

            /*printf("\nblock %d\n", block_number);
            for (i = 0; i < *counter; i++) {
                r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
                printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
            }
            printf("\n");
            printf("\nblock %d\n", newblock_number);
            for (i = 0; i < *newcounter; i++) {
                r = &newblock[FIRST_RECORD_POSITION + i*sizeof(Record)];
                printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
            }
            printf("\n");*/
            if (BF_WriteBlock(header_info->fileDesc, block_number) < 0) {
                BF_PrintError("HT_CreateIndex, BF_WriteBlock");
                return -1;
            }
            if (BF_WriteBlock(header_info->fileDesc, newblock_number) < 0) {
                BF_PrintError("HT_CreateIndex, BF_WriteBlock");
                return -1;
            }

            free(temp_array);
            temp_array = NULL;

            if (EH_InsertEntry(header_info, record) == -1)
                return -1;
        }
    }
    else {
        //puts("counter < MAX_RECORDS\n");
        /*printf("before:\nblock %d\n", block_number);
        for (i = 0; i < *counter; i++) {
            r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
            printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
        }
        printf("\t+\n\n%d %s %s %s\n\n\t=\n", record.id, record.name, record.surname, record.city);*/
        memcpy(&block[FIRST_RECORD_POSITION + (*counter)*sizeof(Record)], &record, sizeof(Record));
        (*counter)++;
        /*printf("after:\nblock %d\n", block_number);
        for (i = 0; i < *counter; i++) {
            r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
            printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
        }*/
        if (BF_WriteBlock(header_info->fileDesc, block_number) < 0) {
            BF_PrintError("HT_CreateIndex, BF_WriteBlock");
            return -1;
        }
    }


    return 0;
}


int EH_GetAllEntries(EH_info header_info, void *value) {
    int key_value=0, block_number, i, *counter;
    unsigned int hash_value;
    void* block;
    char* str;
    Record* r;

    printf("\n-----------------------------------\n");
    
    str = value;
    if (!strcmp(header_info.attrName, "id"))
        key_value = value;
    else if (!strcmp(header_info.attrName, "name"))
        for (i = 0; i < strlen(str); i++)
            key_value += str[i];
    else if (!strcmp(header_info.attrName, "surname"))
        for (i = 0; i < strlen(str); i++)
            key_value += str[i];
    else if (!strcmp(header_info.attrName, "city"))
        for (i = 0; i < strlen(str); i++)
            key_value += str[i];
    else
        return -1;
    
    hash_value = hash(key_value, header_info.depth);
    block_number = header_info.table[hash_value];
    if (BF_ReadBlock(header_info.fileDesc, block_number, &block) < 0) {
        BF_PrintError("EH_CreateIndex, BF_ReadBlock");
        return -1;
    }
    
    counter = &block[COUNTER_POSITION];
    if (!strcmp(header_info.attrName, "id")) {
        for (i = 0; i < *counter; i++) {
            r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
            if (r->id == value)
                printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
        }
    }
    else if (!strcmp(header_info.attrName, "name")) {
        for (i = 0; i < *counter; i++) {
            r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
            if (!strcmp(r->name, value))
                printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
        }
    }
    else if (!strcmp(header_info.attrName, "surname")) {
        for (i = 0; i < *counter; i++) {
            r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
            if (!strcmp(r->surname, value))
                printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
        }
    }
    else if (!strcmp(header_info.attrName, "city")) {
        for (i = 0; i < *counter; i++) {
            r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
            if (!strcmp(r->city, value))
                printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
        }
    }
    /*else if (!strcmp(header_info.attrName, "city")) {
        //int j;
        //for (j = 0; j < power(2, header_info.depth); j++)
        //{
            //if (block_number == header_info.table[hash_value]) continue;
            //printf("block_number: %d\n", block_number);
            //block_number = header_info.table[j];
            //BF_ReadBlock(header_info.fileDesc, header_info.table[j], &block);
            //counter = &block[COUNTER_POSITION];
            for (i = 0; i < *counter; i++) {
                r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
                //if (!strcmp(r->city, value))
                    printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
            }
            printf("\n");
        //}
    }*/
    /*else if (!strcmp(header_info.attrName, "city")) {
        int j;
        for (j = 1; j < BF_GetBlockCounter(header_info.fileDesc); j++)
        {
            if (j == 9) continue;
            printf("block_number: %d\n", j);
            block = BF_ReadBlok[j];
            counter = &block[COUNTER_POSITION];
            for (i = 0; i < *counter; i++) {
                r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
                printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
            }
            printf("\n");
        }
    }*/
    //printf("depth = %d\n", header_info.depth);
    /*else if (!strcmp(header_info.attrName, "city")) {
        printf("block_number: 1\n");
        BF_ReadBlock(header_info.fileDesc, 1, &block);
        counter = &block[COUNTER_POSITION];
        for (i = 0; i < *counter; i++) {
            r = &block[FIRST_RECORD_POSITION + i*sizeof(Record)];
            printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
        }
        printf("\n");
    }*/

    printf("\n-----------------------------------\n");

    return 0;
}


int HashStatistics(char* filename) {
    /* Add your code here */

    return -1;
   
}
