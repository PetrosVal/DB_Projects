#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "exhash.h"
#include "BF.h"

void create_Index(char *fileName, char *attrName, char attrType, int depth) {
    int attrLength = strlen(attrName);
    assert(!EH_CreateIndex(fileName, attrName, attrType, attrLength, depth));
}

EH_info *open_Index(char *fileName) {
    EH_info *info;
    assert((info = EH_OpenIndex(fileName)) != NULL);
    return info;
}

void close_Index(EH_info *info) {
    assert(!EH_CloseIndex(info));
}

void insert_Entries(EH_info *info) {
    FILE *fp;
    Record record;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int count=0;

    if ((fp = fopen("100.csv", "r")) == NULL) {
        perror("fopen");
        return;
    }
    
    while ((read = getline(&line, &len, fp)) != -1) {
        count++;
        sscanf(line, "%d,\"%[^\"]\",\"%[^\"]\",\"%[^\"]\", ", &record.id, record.name, record.surname, record.city);
        //printf("%d %s %s %s\n", record.id, record.name, record.surname, record.city);
        assert(!EH_InsertEntry(info, record));
        if (count > 1000) break;
    }
    fclose(fp);
    free(line);
}

void get_AllEntries(EH_info *info, void *value) {
    assert(EH_GetAllEntries(*info, value) != -1);
}

#define fileName "EH_hashFile"
int main(int argc, char **argv) {
    BF_Init();
    EH_info *info;

    // -- create index
    char attrName[20];
    int depth = 3;
    strcpy(attrName, "city");
    char attrType = 'c';
    // strcpy(attrName, "id");
    // char attrType = 'i';
    create_Index(fileName, attrName, attrType, depth);

    // -- open index
    info = open_Index(fileName);

    // -- insert entries
    insert_Entries(info);

    // -- get all entries
    char value[20];
    strcpy(value, "Keratsini");
    get_AllEntries(info, value);
    // int value = 11903588;
    // get_AllEntries(info, &value);

    // -- close index
    close_Index(info);

    return EXIT_SUCCESS;
}