#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "hash.h"
#include "BF.h"

void create_Index(char *fileName, char *attrName, char attrType, int buckets) {
    int attrLength = strlen(attrName);
    assert(!HT_CreateIndex(fileName, attrType, attrName, attrLength, buckets));
}

HT_info *open_Index(char *fileName) {
    HT_info *info;
    assert((info = HT_OpenIndex(fileName)) != NULL);
    return info;
}

void close_Index(HT_info *info) {
    assert(!HT_CloseIndex(info));
}

void insert_Entries(HT_info *info) {
    FILE *fp;
    Record record;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

	if ((fp = fopen("100000.csv", "r")) == NULL) {
		perror("fopen");
		return;
	}
	
    while ((read = getline(&line, &len, fp)) != -1) {
		sscanf(line, "%d,\"%[^\"]\",\"%[^\"]\",\"%[^\"]\", ", &record.id, record.name, record.surname, record.city);
		//printf("%d %s %s %s\n", record.id, record.name, record.surname, record.city);
		assert(!HT_InsertEntry(*info, record));
	}
	fclose(fp);
	free(line);
}

void get_AllEntries(HT_info *info, void *value) {
    assert(HT_GetAllEntries(*info, value) != -1);
}

#define fileName "HT_hashFile"
int main(int argc, char **argv) {
    BF_Init();
    HT_info *info;

    // -- create index
    char attrName[20];
    int buckets = 128;
    strcpy(attrName, "city");
    char attrType = 'c';
    // strcpy(attrName, "id");
    // char attrType = 'i';
    create_Index(fileName, attrName, attrType, buckets);

    // -- open index
    info = open_Index(fileName);

    // -- insert entries
    insert_Entries(info);

    close_Index(info);
    info = open_Index(fileName);

    // -- get all entries
    char value[20];
    strcpy(value, "Keratsini");
    get_AllEntries(info, value);
    // int value = 864646;
    // get_AllEntries(info, value);

    // -- close index
    close_Index(info);
    
    HashStatistics(fileName);

    return EXIT_SUCCESS;
}
