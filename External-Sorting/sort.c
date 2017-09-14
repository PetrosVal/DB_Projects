#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "BF.h"
#include "sort.h"

#define MAX_RECORDS (BLOCK_SIZE/sizeof(Record))

int recordcmp(Record *record1, Record *record2, int fieldNo) {
	/*printf("%d %s %s %s\n", record1->id, record1->name, record1->surname, record1->city);
	printf("%d %s %s %s\n", record2->id, record2->name, record2->surname, record2->city);*/
	if (record1->id == 0)
		return 1;
	else if (record2->id == 0)
		return -1;
	else if (fieldNo == 0) {
		if (record1->id == record2->id) {
			return 0;
		}
		else if (record1->id > record2->id) {
			return 1;
		}
		else {
			return -1;
		}
	}
	else if (fieldNo == 1)
		return strcmp(record1->name, record2->name);
	else if (fieldNo == 2)
		return strcmp(record1->surname, record2->surname);
	else
		return strcmp(record1->city, record2->city);
}

int Sorted_CreateFile(char *fileName) {
	int fileDesc;
	void* block;
	Sorted_info sorted_info;
	
	if (BF_CreateFile(fileName) < 0) {
		BF_PrintError("Sorted_CreateFile, BF_CreateFile");
		return -1;
	}
	if ((fileDesc = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Sorted_CreateFile, BF_OpenFile");
		return -1;
	}
	if (BF_AllocateBlock(fileDesc) < 0) {
		BF_PrintError("Sorted_CreateFile, BF_AllocateBlock");
		return -1;
	}
	if (BF_ReadBlock(fileDesc, 0, &block) < 0) {
		BF_PrintError("Sorted_CreateFile, BF_ReadBlock");
		return -1;
	}
	sorted_info.num_of_records = 0;
	strcpy(sorted_info.type, "heapFile");
	
	memcpy(&block[0], &sorted_info, sizeof(sorted_info));
	
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Sorted_CreateFile, BF_CloseFile");
		return -1;
	}
	
    return 0;
}


int Sorted_OpenFile(char *fileName) {
	int fileDesc;
	void* block;
	Sorted_info* sorted_info;
	
    if ((fileDesc = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Sorted_OpenFile, BF_OpenFile");
		return -1;
	}
	if (BF_ReadBlock(fileDesc, 0, &block) < 0) {
		BF_PrintError("Sorted_OpenFile, BF_ReadBlock");
		return -1;
	}
	
	sorted_info = &block[0];
	//if (strcmp(sorted_info->type, "heapFile") && strcmp(sorted_info->type, "sortedHeapFile")) {
	//	perror("Sorted_OpenFile, Incorrect type");
	//	return -1;
	//}
	
    return fileDesc;
} 


int Sorted_CloseFile(int fileDesc) {
	if (BF_WriteBlock(fileDesc, 0) < 0) {
		BF_PrintError("Sorted_CloseFile, BF_WriteBlock");
		return -1;
	}
    if (BF_CloseFile(fileDesc) < 0) {
    	BF_PrintError("Sorted_CloseFile, BF_CloseFile");
    	return -1;
    }
    
    return 0;
}


int Sorted_InsertEntry(int fileDesc, Record record) {
	int block_number;
	void* block;
	Sorted_info* sorted_info;
	
	if (BF_ReadBlock(fileDesc, 0, &block) < 0) {
		BF_PrintError("Sorted_InsertEntry, BF_ReadBlock");
		return -1;
	}
	sorted_info = &block[0];
	block_number = sorted_info->num_of_records / MAX_RECORDS + 1;
	if (block_number == BF_GetBlockCounter(fileDesc)) {
		if (BF_AllocateBlock(fileDesc) < 0) {
			BF_PrintError("Sorted_InsertEntry, BF_AllocateBlock");
			return -1;
		}
	}
	if (BF_ReadBlock(fileDesc, block_number, &block) < 0) {
		BF_PrintError("Sorted_InsertEntry, BF_ReadBlock");
		return -1;
	}
	memcpy(&block[(sorted_info->num_of_records % MAX_RECORDS) * sizeof(Record)], &record, sizeof(Record));
	(sorted_info->num_of_records)++;
	if (BF_WriteBlock(fileDesc, block_number) < 0) {
		BF_PrintError("Sorted_InsertEntry, BF_WriteBlock");
		return -1;
	}

	return 0;
}


int Sorted_SortFile(char *fileName, int fieldNo) {
    int fileDesc, new_fileDesc, block_number, pass, i, j, num_of_files, blocks_in_file;
	void *block, *newblock;
	Record *record1, *record2;
	Record temp_record;
	Sorted_info sorted_info;
	char str[50];

	if ((fileDesc = Sorted_OpenFile(fileName)) < 0) {
		BF_PrintError("Sorted_SortFile, Sorted_OpenFile");
		return -1;
	}

	if (BF_ReadBlock(fileDesc, 0, &block) < 0) {
		BF_PrintError("Sorted_SortFile, BF_ReadBlock");
		return -1;
	}

	blocks_in_file = BF_GetBlockCounter(fileDesc);
	for (block_number = 1; block_number < blocks_in_file; block_number++) {
		
		if (BF_ReadBlock(fileDesc, block_number, &block) < 0) {
			BF_PrintError("Sorted_SortFile, BF_ReadBlock");
			return -1;
		}

		sprintf(str, "temp%d", block_number - 1);
		if (BF_CreateFile(str) < 0) {
			BF_PrintError("Sorted_SortFile, BF_CreateFile");
			return -1;
		}
		if ((new_fileDesc = BF_OpenFile(str)) < 0) {
			BF_PrintError("Sorted_SortFile, BF_OpenFile");
			return -1;
		}
		if (BF_AllocateBlock(new_fileDesc) < 0) {
			BF_PrintError("Sorted_SortFile, BF_AllocateBlock");
			return -1;
		}
		if (BF_ReadBlock(new_fileDesc, 0, &newblock) < 0) {
			BF_PrintError("Sorted_SortFile, BF_ReadBlock");
			return -1;
		}
		memcpy(newblock, block, BLOCK_SIZE);

		for (pass = 0; pass < MAX_RECORDS; pass++) {
			for (i = 0; i < MAX_RECORDS - 1 - pass; i++) {
				record1 = &newblock[i * sizeof(Record)];
				record2 = &newblock[(i + 1) * sizeof(Record)];
				if (recordcmp(record1, record2, fieldNo) > 0) {
					temp_record = *record1;
					memcpy(record1, record2, sizeof(Record));
					memcpy(record2, &temp_record, sizeof(Record));
				}
			}
		}
		if (BF_WriteBlock(new_fileDesc, 0) < 0) {
			BF_PrintError("Sorted_SortFile, BF_WriteBlock");
			return -1;
		}
		if (BF_CloseFile(new_fileDesc) < 0) {
	    	BF_PrintError("Sorted_SortFile, BF_CloseFile");
	    	return -1;
	    }
	}
	if (BF_CloseFile(fileDesc) < 0) {
    	BF_PrintError("Sorted_SortFile, BF_CloseFile");
    	return -1;
    }

    num_of_files = block_number - 1;
    i = 0;
    while (1) {
    	if (i >= num_of_files) {
    		num_of_files = (num_of_files + 1) / 2;
    		i = 0;
    	}
    	if (num_of_files == 1)
    		break;
    	if (Sorted_MergeSort(i, fieldNo) < 0) {
    		printf("Sorted_SortFile, Sorted_MergeSort");
    		return -1;
    	}
    	i += 2;
    }
    
    sprintf(str, "%sSorted%d", fileName, fieldNo);
    if (BF_CreateFile(str) < 0) {
		BF_PrintError("Sorted_SortFile, BF_CreateFile");
		return -1;
	}
	if ((new_fileDesc = BF_OpenFile(str)) < 0) {
		BF_PrintError("Sorted_SortFile, BF_OpenFile");
		return -1;
	}
	if ((fileDesc = BF_OpenFile("temp0")) < 0) {
		BF_PrintError("Sorted_SortFile, BF_OpenFile");
		return -1;
	}
	if (BF_AllocateBlock(new_fileDesc) < 0) {	
		BF_PrintError("Sorted_SortFile, BF_AllocateBlock");
		return -1;
	}
	if (BF_ReadBlock(new_fileDesc, 0, &newblock) < 0) {
		BF_PrintError("Sorted_SortFile, BF_ReadBlock");
		return -1;
	}
	sorted_info.num_of_records = 0;
	strcpy(sorted_info.type, "sortedHeapFile");
	memcpy(newblock, &sorted_info, sizeof(sorted_info));
	for (i = 0; i < BF_GetBlockCounter(fileDesc); i++) {
		if (BF_AllocateBlock(new_fileDesc) < 0) {	
			BF_PrintError("Sorted_SortFile, BF_AllocateBlock");
			return -1;
		}
    	if (BF_ReadBlock(fileDesc, i, &block) < 0) {
			BF_PrintError("Sorted_SortFile, BF_ReadBlock");
			return -1;
		}
		if (BF_ReadBlock(new_fileDesc, i + 1, &newblock) < 0) {
			BF_PrintError("Sorted_SortFile, BF_ReadBlock");
			return -1;
		}
		memcpy(newblock, block, BLOCK_SIZE);
		if (BF_WriteBlock(new_fileDesc, i + 1) < 0) {
			BF_PrintError("Sorted_SortFile, BF_WriteBlock");
			return -1;
		}
    }
    if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Sorted_SortFile, BF_CloseFile");
		return -1;
	}
	//if (BF_CloseFile(new_fileDesc) < 0) {
	//	BF_PrintError("Sorted_SortFile, BF_CloseFile");
	//	return -1;
	//}
    unlink("temp0");

    //Sorted_GetAllEntries(new_fileDesc, NULL, 1);

	return 0;
}


int Sorted_MergeSort(int fileN, int fieldNo) {
	int fileDesc1, fileDesc2, blocks_in_file1, blocks_in_file2, i, j, k, out, block_number1, block_number2, out_block_number, counter=1;
	void *block1, *block2, *out_block;
	char str1[50], str2[50];
	Record *record1, *record2;

	sprintf(str1, "temp%d", fileN);
	if ((fileDesc1 = BF_OpenFile(str1)) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_OpenFile");
		return -1;
	}
	sprintf(str2, "temp%d", fileN + 1);
	if ((fileDesc2 = BF_OpenFile(str2)) < 0) {
		sprintf(str2, "temp%d", fileN / 2);
		if (BF_CreateFile(str2) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_CreateFile");
			return -1;
		}
		if ((fileDesc2 = BF_OpenFile(str2)) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_OpenFile");
			return -1;
		}
		for (i = 0; i < BF_GetBlockCounter(fileDesc1); i++) {
			if (BF_AllocateBlock(fileDesc2) < 0) {	
				BF_PrintError("Sorted_MergeSort, BF_AllocateBlock");
				return -1;
			}
	    	if (BF_ReadBlock(fileDesc1, i, &block1) < 0) {
				BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
				return -1;
			}
			if (BF_ReadBlock(fileDesc2, i, &block2) < 0) {
				BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
				return -1;
			}
			memcpy(&block2[0], block1, BLOCK_SIZE);
			if (BF_WriteBlock(fileDesc2, i) < 0) {
				BF_PrintError("Sorted_MergeSort, BF_WriteBlock");
				return -1;
			}
	    }
	    if (BF_CloseFile(fileDesc1) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_CloseFile");
			return -1;
		}
		if (BF_CloseFile(fileDesc2) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_CloseFile");
			return -1;
		}
	    unlink(str1);
	    
		return 0;
	}

	if (fileN == 0) {
		if (BF_CreateFile("out") < 0) {
			BF_PrintError("Sorted_MergeSort, BF_CreateFile");
			return -1;
		}
		if ((out = BF_OpenFile("out")) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_OpenFile");
			return -1;
		}
	}
	else {
		sprintf(str1, "temp%d", fileN / 2);
		if (BF_CreateFile(str1) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_CreateFile");
			return -1;
		}
		if ((out = BF_OpenFile(str1)) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_OpenFile");
			return -1;
		}
	}
	if (BF_AllocateBlock(out) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_AllocateBlock");
		return -1;
	}

	blocks_in_file1 = BF_GetBlockCounter(fileDesc1);
	blocks_in_file2 = BF_GetBlockCounter(fileDesc2);

	block_number1 = 0;
	block_number2 = 0;
	out_block_number = 0;
	i = 0;
	j = 0;
	k = 0;
	if (BF_ReadBlock(fileDesc1, 0, &block1) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
		return -1;
	}
	if (BF_ReadBlock(fileDesc2, 0, &block2) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
		return -1;
	}
	if (BF_ReadBlock(out, 0, &out_block) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
		return -1;
	}

	while(1) {
		if (i == MAX_RECORDS) {
			i = 0;
			block_number1++;
			if (block_number1 < blocks_in_file1) {
				if (BF_ReadBlock(fileDesc1, block_number1, &block1) < 0) {
					BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
					return -1;
				}
			}
		}
		if (j == MAX_RECORDS) {
			j = 0;
			block_number2++;
			if (block_number2 < blocks_in_file2) {
				if (BF_ReadBlock(fileDesc2, block_number2, &block2) < 0) {
					BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
					return -1;
				}
			}
		}
		if (k == MAX_RECORDS) {
			k = 0;
			if (BF_WriteBlock(out, out_block_number) < 0) {
				BF_PrintError("Sorted_MergeSort, BF_WriteBlock");
				return -1;
			}
			out_block_number++;
			if (out_block_number < blocks_in_file1 + blocks_in_file2) {
				if (BF_AllocateBlock(out) < 0) {
					BF_PrintError("Sorted_MergeSort, BF_AllocateBlock");
					return -1;
				}
				if (BF_ReadBlock(out, out_block_number, &out_block) < 0) {
					BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
					return -1;
				}
			}
			else
				break;
		}

		record1 = &block1[i * sizeof(Record)];
		record2 = &block2[j * sizeof(Record)];
		if (record1->id == 0 && record2->id == 0)
			break;
		else if ((block_number1 < blocks_in_file1) && (block_number2 < blocks_in_file2)) {
			if (recordcmp(record1, record2, fieldNo) <= 0) {
				memcpy(&out_block[k * sizeof(Record)], record1, sizeof(Record));
				i++;
			}
			else {
				memcpy(&out_block[k * sizeof(Record)], record2, sizeof(Record));
				j++;
			}
		}
		else if (block_number1 < blocks_in_file1) {
			memcpy(&out_block[k * sizeof(Record)], record1, sizeof(Record));
			i++;
		}
		else if (block_number2 < blocks_in_file2) {
			memcpy(&out_block[k * sizeof(Record)], record2, sizeof(Record));
			j++;
		}
		else
			break;
		k++;
	}

	if (BF_CloseFile(fileDesc1) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_CloseFile");
		return -1;
	}
	if (BF_CloseFile(fileDesc2) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_CloseFile");
		return -1;
	}
	
	sprintf(str1, "temp%d", fileN);
	unlink(str1);
	unlink(str2);

	sprintf(str1, "temp%d", fileN / 2);
	if (fileN > 0) {
		if (BF_CloseFile(out) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_CloseFile");
			return -1;
		}
		return 0;
	}

	if (BF_CreateFile(str1) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_CreateFile");
		return -1;
	}
	if ((fileDesc1 = BF_OpenFile(str1)) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_OpenFile");
		return -1;
	}
	for (i = 0; i < BF_GetBlockCounter(out); i++) {
		if (BF_AllocateBlock(fileDesc1) < 0) {	
			BF_PrintError("Sorted_MergeSort, BF_AllocateBlock");
			return -1;
		}
    	if (BF_ReadBlock(out, i, &out_block) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
			return -1;
		}
		if (BF_ReadBlock(fileDesc1, i, &block1) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_ReadBlock");
			return -1;
		}
		memcpy(&block1[0], out_block, BLOCK_SIZE);
		if (BF_WriteBlock(fileDesc1, i) < 0) {
			BF_PrintError("Sorted_MergeSort, BF_WriteBlock");
			return -1;
		}
    }
    if (BF_CloseFile(out) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_CloseFile");
		return -1;
	}
	if (BF_CloseFile(fileDesc1) < 0) {
		BF_PrintError("Sorted_MergeSort, BF_CloseFile");
		return -1;
	}
    unlink("out");
	return 0;
}


int Sorted_checkSortedFile(char *fileName, int fieldNo) {
	int fileDesc, block_number, i, j, blocks_in_file;
	void* block;
	Record *record1, *record2;

	if ((fileDesc = BF_OpenFile(fileName)) < 0) {
		BF_PrintError("Sorted_checkSortedFile, BF_OpenFile");
		return -1;
	}

	blocks_in_file = BF_GetBlockCounter(fileDesc);
	j = 1;
	for (block_number = 1; block_number < blocks_in_file; block_number++) {
		if (BF_ReadBlock(fileDesc, block_number, &block) < 0) {
			BF_PrintError("Sorted_checkSortedFile, BF_ReadBlock");
			return -1;
		}
		if (block_number > 1) {
			record2 = &block[0];
			if (recordcmp(record1, record2, fieldNo) > 0)
				return -1;
		}
		for (i = 0; i < MAX_RECORDS; i++) {
			record1 = &block[i * sizeof(Record)];
			if (i == MAX_RECORDS - 1 || record1->id == 0)
				break;
			record2 = &block[(i + 1) * sizeof(Record)];
			if (recordcmp(record1, record2, fieldNo) > 0) {
				printf("%d\n", record1->id);
				return -1;
			}
		}
	}
	if (BF_CloseFile(fileDesc) < 0) {
		BF_PrintError("Sorted_checkSortedFile, BF_CloseFile");
		return -1;
	}
	return 0;
}


void Sorted_GetAllEntries(int fileDesc, int* fieldNo, void *value) {
    int block_number, i, j, blocks_in_file, low, high, blocks_read, i1, i2, previous_block_number, next_block_number;
	void* block;
	Record *record1, *record2;

	blocks_in_file = BF_GetBlockCounter(fileDesc);
	j = 1;
	blocks_read = 0;
	if (fieldNo == NULL) {
		for (block_number = 1; block_number < blocks_in_file; block_number++) {
			if (BF_ReadBlock(fileDesc, block_number, &block) < 0) {
				BF_PrintError("Sorted_GetAllEntries, BF_ReadBlock");
				return -1;
			}
			blocks_read++;
			for (i = 0; i < MAX_RECORDS; i++) {
				record1 = &block[i * sizeof(Record)];
				if (record1->id == 0)
					break;
				printf("%d: %d %s %s %s\n", j++, record1->id, record1->name, record1->surname, record1->city);
			}
		}
	}
	else {
		record2 = malloc(sizeof(Record));
		if (*fieldNo == 0)
			record2->id = *(int*)value;
		else if (*fieldNo == 1)
			strcpy(record2->name, value);
		else if (*fieldNo == 2)
			strcpy(record2->surname, value);
		else
			strcpy(record2->city, value);

		low = 1;
		high = blocks_in_file - 1;

		while (low < high) {
			block_number = (low + high) / 2;
			if (BF_ReadBlock(fileDesc, block_number, &block) < 0) {
				BF_PrintError("Sorted_GetAllEntries, BF_ReadBlock");
				return -1;
			}
			blocks_read++;
			record1 = &block[0];

			i = 0;
			if (recordcmp(record1, record2, *fieldNo) > 0) {
				high = block_number;
				continue;
			}
			while (i < MAX_RECORDS - 1 && recordcmp(record1, record2, *fieldNo) < 0) {
				i++;
				record1 = &block[i * sizeof(Record)];
			}
			if (recordcmp(record1, record2, *fieldNo) == 0) {
				previous_block_number = block_number;
				next_block_number = block_number;
				i1 = i;
				i2 = i;
				do {
					printf("%d: %d %s %s %s\n", j++, record1->id, record1->name, record1->surname, record1->city);
					i1++;
					if (i1 == MAX_RECORDS) {
						next_block_number++;
						if (BF_ReadBlock(fileDesc, next_block_number, &block) < 0) {
							BF_PrintError("Sorted_GetAllEntries, BF_ReadBlock");
							return -1;
						}
						blocks_read++;
						i1 = 0;
					}
					record1 = &block[i1 * sizeof(Record)];
				} while (recordcmp(record1, record2, *fieldNo) == 0);
				while (recordcmp(record1, record2, *fieldNo) == 0) {
					i2--;
					if (i2 < 0) {
						previous_block_number--;
						if (BF_ReadBlock(fileDesc, previous_block_number, &block) < 0) {
							BF_PrintError("Sorted_GetAllEntries, BF_ReadBlock");
							return -1;
						}
						blocks_read++;
						i2 = MAX_RECORDS - 1;
					}
					record1 = &block[i2 * sizeof(Record)];
					printf("%d: %d %s %s %s\n", j++, record1->id, record1->name, record1->surname, record1->city);
				}
				break;
			}
			low = block_number;
			continue;
		}
	}
	printf("blocks read: %d\n", blocks_read);
}