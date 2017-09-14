typedef struct Record {
	int id;
	char name[15];
	char surname[20];
	char city[25];
} Record;


/* Όπου fileDesc είναι ο αναγνωριστικός 
αριθμός του ανοίγματος του αρχείου, όπως επιστράφηκε από το επίπεδο διαχείρισης μπλοκ. */
typedef struct {
    char type[20];
    int num_of_records;      /* ο αριθμός του τελευταίου block με εγγραφές */
} Sorted_info;


int Sorted_CreateFile(char *fileName);


int Sorted_OpenFile(char *fileName);


int Sorted_CloseFile(int fileDesc);


int Sorted_InsertEntry(int fileDesc, Record record);


int Sorted_SortFile(char *fileName, int fieldNo);


int Sorted_MergeSort(int fileN, int fieldNo);


int Sorted_checkSortedFile(char *fileName, int fieldNo);


void Sorted_GetAllEntries(int fileDesc, int* fieldNo, void *value);