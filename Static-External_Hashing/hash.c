#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "BF.h"
#include "hash.h"

#define MAX_RECORDS BLOCK_SIZE/sizeof(Record) - 1

int HT_CreateIndex( char *fileName, char attrType, char* attrName, int attrLength, int buckets) {
	//BF_Init();
	int fileDesc, i, counter=0;
	void* block;
	
	if ((fileDesc = BF_CreateFile(fileName)) < 0) {
		BF_PrintError("HT_CreateIndex, BF_CreateFile");
		return -1;
	}
	if (BF_OpenFile(fileName) < 0) {
		BF_PrintError("HT_CreateIndex, BF_OpenFile");
		return -1;
	}
	for (i=0; i<=buckets; i++) {
		if (BF_AllocateBlock(fileDesc) < 0) {
			BF_PrintError("HT_CreateIndex, BF_AllocateBlock");
			return -1;
		}
		if (BF_ReadBlock(fileDesc, i, &block) < 0) {
			BF_PrintError("HT_CreateIndex, BF_ReadBlock");
			return -1;
		}
		if (i == 0) {
			memcpy(&block[HASH_TYPE_POSITION], STATIC, strlen(STATIC)+1);
			memcpy(&block[ATTR_TYPE_POSITION], &attrType, 1);
			memcpy(&block[ATTR_TYPE_POSITION+1], "\0", 1);
			memcpy(&block[ATTR_NAME_POSITION], attrName, strlen(attrName)+1);
			memcpy(&block[ATTR_LENGTH_POSITION], &attrLength, sizeof(attrLength));
			memcpy(&block[NUM_OF_BUCKETS_POSITION], &buckets, sizeof(buckets));
		}
		memcpy(&block[BLOCK_SIZE - sizeof(Record)], &counter, sizeof(int));
		//memcpy(&block[BLOCK_SIZE - sizeof(Record) + 2*sizeof(int)], "0", 2);
		
		if (BF_WriteBlock(fileDesc, i) < 0) {
			BF_PrintError("HT_CreateIndex, BF_WriteBlock");
			return -1;
		}
	}
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("HT_CreateIndex, BF_CloseFile");
		return -1;
	}
	
    return 0;
}


HT_info* HT_OpenIndex(char *fileName) {
	int fileDesc;
	void* block;
	char* str;
	HT_info* info;
	
    if ((fileDesc = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("HT_CreateIndex, BF_OpenFile");
		return NULL;
	}
	if (BF_ReadBlock(fileDesc, 0, &block) < 0) {
		BF_PrintError("HT_CreateIndex, BF_ReadBlock");
		return NULL;
	}
	
	info = malloc(sizeof(HT_info));
	if (info == NULL)
		return NULL;
	info->attrName = malloc(32*sizeof(char));
	if (info->attrName == NULL) {
		free(info);
		return NULL;
	}
	
	str = &block[HASH_TYPE_POSITION];
	if (strcmp(str, STATIC)) {
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
	
	info->numBuckets = *((int*)&block[NUM_OF_BUCKETS_POSITION]);
	if (info->numBuckets <= 0) {
		free(info->attrName);
		free(info);
		return NULL;
	}
	
    return info;
} 


int HT_CloseIndex( HT_info* header_info ) {
    if (BF_CloseFile(header_info->fileDesc) < 0) {
    	BF_PrintError("HT_CloseIndex, BF_CloseFile");
    	return -1;
    }
    free(header_info->attrName);
    free(header_info);
    header_info = NULL;
    
    return 0;
}

int HT_InsertEntry(HT_info header_info, Record record) {
	int key=0, block_number, newblock_number, i, *counter;
	void* block;
	void* newblock;
	Record* r;
	
    if (!strcmp(header_info.attrName, "id"))
    	key = record.id;
    else if (!strcmp(header_info.attrName, "name"))
    	for (i=0; i<strlen(record.name); i++)
    		key += record.name[i];
    else if (!strcmp(header_info.attrName, "surname"))
    	for (i=0; i<strlen(record.surname); i++)
    		key += record.surname[i];
    else if (!strcmp(header_info.attrName, "city"))
    	for (i=0; i<strlen(record.city); i++)
    		key += record.city[i];
	else
		return -1;
	
	block_number = key % header_info.numBuckets + 1;
	if (BF_ReadBlock(header_info.fileDesc, block_number, &block) < 0) {
		BF_PrintError("HT_InsertEntry, BF_ReadBlock");
		return -1;
	}
	
	while (*((int*)&block[BLOCK_SIZE - sizeof(Record) + sizeof(int)])) {
		block_number = *((int*)&block[BLOCK_SIZE - sizeof(Record) + sizeof(int)]);
		//printf("block number: %d\n", block_number);
		if (BF_ReadBlock(header_info.fileDesc, block_number, &block) < 0) {
			BF_PrintError("HT_InsertEntry, BF_ReadBlock");
			return -1;
		}
	}
	
	counter = &block[BLOCK_SIZE - sizeof(Record)];
	if (*counter == MAX_RECORDS) {
		if (BF_AllocateBlock(header_info.fileDesc) < 0) {
			BF_PrintError("HT_InsertEntry, BF_AllocateBlock");
			//printf("%d blocks\n", BF_GetBlockCounter(header_info.fileDesc));
			return -1;
		}
		newblock_number = BF_GetBlockCounter(header_info.fileDesc) - 1;
		if (BF_ReadBlock(header_info.fileDesc, newblock_number, &newblock) < 0) {
			BF_PrintError("HT_InsertEntry, BF_ReadBlock");
			return -1;
		}
		memcpy(&block[BLOCK_SIZE - sizeof(Record) + sizeof(int)], &newblock_number, sizeof(int));
		memcpy(&newblock[0], &record, sizeof(record));
		counter = &newblock[BLOCK_SIZE - sizeof(Record)];
		(*counter) = 0;
		if (BF_WriteBlock(header_info.fileDesc, newblock_number) < 0) {
			BF_PrintError("HT_InsertEntry, BF_WriteBlock");
			return -1;
		}
	}
	else {
		memcpy(&block[(*counter)*sizeof(Record)], &record, sizeof(record));
		(*counter)++;
	}

	if (BF_WriteBlock(header_info.fileDesc, block_number) < 0) {
		BF_PrintError("HT_InsertEntry, BF_WriteBlock");
		return -1;
	}
	
	return 0;
}



int HT_GetAllEntries(HT_info header_info, void *value) {
    int key=0, block_number, i, *counter;
	void* block;
	char* str;
	Record* r;
	
	str = value;
    if (!strcmp(header_info.attrName, "id"))
    	key = value;
    else if (!strcmp(header_info.attrName, "name"))
    	for (i=0; i<strlen(str); i++)
    		key += str[i];
    else if (!strcmp(header_info.attrName, "surname"))
    	for (i=0; i<strlen(str); i++)
    		key += str[i];
    else if (!strcmp(header_info.attrName, "city"))
    	for (i=0; i<strlen(str); i++)
    		key += str[i];
	else
		return -1;
	
	block_number = key % header_info.numBuckets + 1;
	if (BF_ReadBlock(header_info.fileDesc, block_number, &block) < 0) {
		BF_PrintError("HT_GetAllEntries, BF_ReadBlock");
		return -1;
	}
	
	do {
		counter = &block[BLOCK_SIZE - sizeof(Record)];
		if (!strcmp(header_info.attrName, "id")) {
			for (i=0; i<*counter; i++) {
				r = &block[i*sizeof(Record)];
				if (r->id == value)
					printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
			}
		}
		else if (!strcmp(header_info.attrName, "name")) {
			for (i=0; i<*counter; i++) {
				r = &block[i*sizeof(Record)];
				if (!strcmp(r->name, value))
					printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
			}
		}
		else if (!strcmp(header_info.attrName, "surname")) {
			for (i=0; i<*counter; i++) {
				r = &block[i*sizeof(Record)];
				if (!strcmp(r->surname, value))
					printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
			}
		}
		else if (!strcmp(header_info.attrName, "city")) {
			for (i=0; i<*counter; i++) {
				r = &block[i*sizeof(Record)];
				if (!strcmp(r->city, value))
					printf("%d %s %s %s\n", r->id, r->name, r->surname, r->city);
			}
		}
		block_number = *((int*)&block[BLOCK_SIZE - sizeof(Record) + sizeof(int)]);
		if (BF_ReadBlock(header_info.fileDesc, block_number, &block) < 0) {
			BF_PrintError("HT_GetAllEntries, BF_ReadBlock");
			return -1;
		}
	} while (*counter == MAX_RECORDS);

    return 0;
}


int HashStatistics(char* filename) {
    int fileDesc, block_counter, i, block_number, totalrecs, recs, minrecs, maxrecs, overflow_blocks, buckets_with_overflow;
    int *counter;
	void* block;
	//char* str;
	HT_info* info;
	
	info = HT_OpenIndex(filename);
	block_counter = BF_GetBlockCounter(info->fileDesc) - 1; //minus header block
	printf("block counter = %d\n", block_counter);
	
    if ((fileDesc = BF_OpenFile(filename)) < 0) {
		BF_PrintError("HT_CreateIndex, BF_OpenFile");
		return NULL;
	}
	totalrecs = 0;
	minrecs = MAX_RECORDS;
	maxrecs = -1;
	buckets_with_overflow = 0;
	for (i=1; i<=info->numBuckets; i++) {
		recs = 0;
		if (BF_ReadBlock(fileDesc, i, &block) < 0) {
			BF_PrintError("HT_CreateIndex, BF_ReadBlock");
			return NULL;
		}
		overflow_blocks = -1;
		do {
			overflow_blocks++;
			counter = &block[BLOCK_SIZE - sizeof(Record)];
			recs += *counter;
			block_number = *((int*)&block[BLOCK_SIZE - sizeof(Record) + sizeof(int)]);
			if (BF_ReadBlock(info->fileDesc, block_number, &block) < 0) {
				BF_PrintError("HT_GetAllEntries, BF_ReadBlock");
				return -1;
			}
		} while (*counter == MAX_RECORDS);
		if (overflow_blocks > 0) {
			printf("bucket %d has %d overflow block", i, overflow_blocks);
			if (overflow_blocks > 1)
				printf("s");
			printf("\n");
			buckets_with_overflow++;
		}
		totalrecs += recs;
		if (recs < minrecs)
			minrecs = recs;
		if (recs > maxrecs)
			maxrecs = recs;
	}
	printf("buckets with overflow: %d\n", buckets_with_overflow);
	printf("minimum records per bucket: %d\n", minrecs);
	printf("maximum records per bucket: %d\n", maxrecs);
	printf("average number of records per bucket: %.2f\n", (float)totalrecs/info->numBuckets);
	printf("average number of blocks per bucket: %.2f\n", (float)block_counter/info->numBuckets);
    
    return 0;
}







