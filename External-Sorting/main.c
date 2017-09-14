#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BF.h"
#include "sort.h"
#define fileName "heapFile"

void insert_entries(int fd) {
    FILE *fp;
    Record record;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int counter=0;

	if ((fp = fopen("10000.csv", "r")) == NULL) {
		perror("fopen");
		return;
	}
	
    while ((read = getline(&line, &len, fp)) != -1) {
    	//if ((counter++) == 200) break;
		sscanf(line, "%d,\"%[^\"]\",\"%[^\"]\",\"%[^\"]\", ", &record.id, record.name, record.surname, record.city);
		assert(!Sorted_InsertEntry(fd, record));
	}
	
	fclose(fp);
	free(line);
}


int main(int argc, char **argv) {
    
    int fd;
    int fieldNo;
   
    BF_Init();
    
    //create heap file
    if(Sorted_CreateFile(fileName) == -1  )
        printf("Error creating file!\n");
    
    fd = Sorted_OpenFile(fileName);
    if( fd == -1  )
        printf("Error opening file!\n");
    insert_entries(fd);
    
    Sorted_CloseFile(fd);
    fd = Sorted_OpenFile(fileName);

    //sort heap file using 2-way merge-sort
    if(Sorted_SortFile(fileName,0) == -1  )
        printf("Error sorting file!\n");
    
    //if(Sorted_checkSortedFile("heapFileSorted0",0) == -1  )
    //    printf("Error sorting file!\n");
    
    //get all entries with value
    //char value[20];
    //strcpy(value, "Keratsini");
    fieldNo = 0;
    int value = 11903588;
    
    fd = Sorted_OpenFile("heapFileSorted0");
    if( fd == -1  )
        printf("Error opening file!\n");
    
    Sorted_GetAllEntries(fd,&fieldNo,&value);
    
    
    return EXIT_SUCCESS;
}
