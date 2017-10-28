#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Sorter.h"

int main(int argc, char **argv) {
	//./sorter -c  movie_title -d thisdir -o thatdir
	//0        1   2           3  4       5  6
	if (argc % 2 != 1) {
		printf("Error, command line missing parameter\n");
		return 0;
	} else if (argc<2) {
		printf("Usage: ./program -c <dirname>\n");
	}
	//boolean flag checks
	//char *columnHeadings = NULL;
	char *directory = NULL; //default search current directory
	char *outputDirectory = NULL; //defaut output to current directory 
	//columnHeadings = argv[2];
	int i;
	for (i=3;i<argc;i+=2) {
		//flag
		if (!strcmp(argv[i],"-d")) {
			directory = argv[i+1];
		} else if (!strcmp(argv[i],"-o")) {
			outputDirectory = argv[i+1];
		} else {
			printf("FLAG BROKEN\n");
		}
	}
	
	printf("\nEnter Forking Area\n");
	printf("Initial PID: %d\nPIDS of all child processes:\n", getpid());
	int totalNumProcesses = parseDir(directory, outputDirectory, argv[2]);
	printf("\nTotal number of processes %d\n", totalNumProcesses);
	printf("\nProcess Completion!\n");
	
	return 0;
}

///Parses CSV and returns a pointer to CSV.
struct csv *parseCSV(FILE *file) {

	//Pointer to CSV file that will be returned.
	struct csv *ret = malloc(sizeof(struct csv));

	//Retrieve header info and populate CSV with values.
	struct headerInfo headerInfo = getHeaderInfo(file);
	ret->columnTypes = headerInfo.types;
	ret->columnNames = headerInfo.columnNames;

	//populate entries and total number of entries
	struct entryInfo entryInfo = getCSVEntries(file, ret->columnTypes);
	ret->entries = entryInfo.entries;
	ret->numEntries = entryInfo.numEntries;
	
	return ret;
}

///Parse first line of CSV and get array of data types for values.
struct headerInfo getHeaderInfo(FILE *file) {

	struct headerInfo ret;

	char **columnNames = malloc(sizeof(char *) * columns);
	enum type *types = malloc(sizeof(enum type) * columns);

	char *currentInput;
	char nextChar;
	int stringPosition, retPosition=0;
	char newlineFound = 0;

	//Ignore leading comma if exists.
	fscanf(file, ",");

	while (!newlineFound) {
		currentInput = malloc(sizeof(char) * maxStringSize);
		stringPosition = 0;
		while (fscanf(file, "%c", &nextChar) > 0) {
			if (nextChar == ',') {
				break;
			} else if (nextChar == '\n' || nextChar == '\r') {
				fscanf(file, "\r");
				fscanf(file, "\n");
				newlineFound = 1;
				break;
			}
			currentInput = addCharacterToString(currentInput, nextChar, stringPosition++);
		}
		//Add null-terminating 0 to end of String.
		currentInput = addCharacterToString(currentInput, '\0', stringPosition);
		types[retPosition] = getTypeFromColumnName(currentInput);
		columnNames[retPosition] = currentInput;

		retPosition++;
	}

	ret.columnNames = columnNames;
	ret.types = types;

	return ret;
}

///Retrieves CSV Entries through fscanf and returns array of entries as well as entry count.
struct entryInfo getCSVEntries(FILE *file, enum type *columnTypes) {

	//Return value: Array of Entry Pointers.
	struct entry **ret = malloc(sizeof(struct entry *) * maxEntries);

	char eofReached = 0, newlineFound = 0, next;
	int scanResult;

	char *currentString = malloc(sizeof(char)*maxStringSize);;
	int stringPosition;

	int quotationMarksFound = 0;
	int nonWhiteSpaceFound = 0;

	int currentEntryPosition = 0;
	int currentValuePosition = 0;
	struct entry *currentEntry;

	//Loop through each line until end of file reached.
	while (!eofReached) {
		newlineFound = 0, stringPosition = 0, quotationMarksFound = 0, nonWhiteSpaceFound = 0;

		//For each line, a new entry will be created (with an array of value pointers).
		currentEntry = malloc(sizeof(struct entry));
		currentEntry -> values = malloc(sizeof(union value) * columns);

		//Loop through each character within a line until line break or end of file reached.
		while (!newlineFound && !eofReached) {
			scanResult = fscanf(file, "%c", &next);
			if (scanResult == EOF) {
				eofReached = 1;
				if (stringPosition == 0) {
					break;
				}
				currentString = addCharacterToString(currentString, '\0', stringPosition);

				setValue(&(currentEntry -> values[currentValuePosition]), currentString, columnTypes[currentValuePosition]);
				currentValuePosition++;

				stringPosition = 0;

				currentString = malloc(sizeof(char)*maxStringSize);
				break;
			}
			if (next == '\r' || next == '\n') {
				fscanf(file, "\r");
				fscanf(file, "\n");
				if (stringPosition == 0) {
					continue;
				}
				currentString = addCharacterToString(currentString, '\0', stringPosition);
				setValue(&(currentEntry -> values[currentValuePosition]), currentString, columnTypes[currentValuePosition]);
				currentValuePosition++;

				stringPosition = 0;


				currentString = malloc(sizeof(char)*maxStringSize);
				newlineFound = 1;
				break;
			} else if (next == '"') {
				//If quotation marks found, ignore any commas until next quotation mark found.
				quotationMarksFound = !quotationMarksFound;
			} else if (next == ',' && !quotationMarksFound) {
				currentString = addCharacterToString(currentString, '\0', stringPosition);
				setValue(&(currentEntry -> values[currentValuePosition]), currentString, columnTypes[currentValuePosition]);
				currentValuePosition++;

				//For now, if the CSV file has too many columns in a single row, ignore the rest of that row.
				//Should be fixed when quotation marks are handled.
				if (currentValuePosition > columns) {
					newlineFound = 1;
				}
				stringPosition = 0;	

				currentString = malloc(sizeof(char)*maxStringSize);
			} else {
				if (nonWhiteSpaceFound || (next != ' ' && next != '\t')) {
					currentString = addCharacterToString(currentString, next, stringPosition++);
					nonWhiteSpaceFound = 1;
				}
			}
		}

		ret = addEntryToArray(ret, currentEntry, currentEntryPosition++);

		currentEntry = malloc(sizeof(struct entry));
		currentEntry -> values = malloc(sizeof(union value) * columns);
		currentValuePosition = 0;
		//printf("Current Entry: %d\n", currentEntryPosition);
	}

	struct entryInfo ei;
	ei.entries = ret;
	ei.numEntries = currentEntryPosition - 1;

	return ei;
}

enum type getTypeFromColumnName(char *name) {
	int i;

	for (i=0;i<stringValuesSize;i++) {
		if (strcmp(stringValues[i], name) == 0) {
			return string;
		}
	}

	for (i=0;i<intValuesSize;i++) {
		if (strcmp(intValues[i], name) == 0) {
			return integer;
		}
	}

	for (i=0;i<doubleValuesSize;i++) {
		if (strcmp(doubleValues[i], name) == 0) {
			return decimal;
		}
	}
	return error;
}

///Debugging method to print values from the CSV.
void printRange(struct csv *csv, int fromRow, int toRow, int columnNumber) {
//NOTE: ROW 2 ON EXCEL IS CONSIDERED ROW 0 HERE SINCE THE FIRST ROW CONTAINS HEADER INFO AND NOT ACTUAL DATA.

	char *columnTypeAsString;

	enum type columnType = csv->columnTypes[columnNumber];
	if (columnType == string) {
		columnTypeAsString = "String";
	} else if (columnType == integer) {
		columnTypeAsString = "Integer";
	} else if (columnType == decimal) {
		columnTypeAsString = "Decimal";
	} else {
		printf("Error: Unknown type printing range.");
		return;
	}

	printf("Printing Rows %d to %d.\n", fromRow, toRow);
	printf("Column Number: %d\n", columnNumber);
	printf("Column Name: %s\n", csv->columnNames[columnNumber]);
	printf("Column Type: %s\n", columnTypeAsString);

	int rowCounter;
	for (rowCounter = fromRow; rowCounter <= toRow; rowCounter++) {
		if (columnType == string) {
			printf("Row %d: %s\n", rowCounter, csv->entries[rowCounter]->values[columnNumber].stringVal);
		} else if (columnType == integer) {
			printf("Row %d: %ld\n", rowCounter, csv->entries[rowCounter]->values[columnNumber].intVal);
		} else if (columnType == decimal) {
			printf("Row %d: %f\n", rowCounter, csv->entries[rowCounter]->values[columnNumber].decimalVal);
		}
		
	}

}

void mergesortMovieList(struct csv *csv, int *indexesOfSortBys, enum type *columnTypes, int numberOfSortBys) {
	
	struct entry** entries = csv->entries;
	long low = 0;
	//numEntries includes the labels row (-1), to use Array indicies (-1)
	long high = csv->numEntries-1;
	
	//start mergeSort
	MergeSort(low, high, entries, indexesOfSortBys, columnTypes, numberOfSortBys);
	
}

void MergeSort(long low, long high, struct entry** entries, enum type *columnTypes, int *compareIndexes, int numberOfSortBys){
	//split up array until single blocks are made
	if (low < high){
		//lower array has the "mid" element
		MergeSort(low, ((low+high)/2), entries, compareIndexes, columnTypes, numberOfSortBys);
		MergeSort(((low+high)/2)+1, high, entries, compareIndexes, columnTypes, numberOfSortBys);
		MergeParts(low, high, entries, compareIndexes, columnTypes, numberOfSortBys);
	}
	return;
}

void MergeParts(long low, long high, struct entry** entries, enum type *columnTypes, int *compareIndexes, int numberOfSortBys){
	// (low+high)/2 is part of the lower array
	long  mid = (low+high)/2;
	
	//dynamically create an array of pointers
	struct entry **tempArray1;
	struct entry **tempArray2;
	
	//copy the pointers from entries into tempArray1 and tempArray2
	tempArray1 = malloc(sizeof(struct entry *)*(mid-low+1)); 
	tempArray2 = malloc(sizeof(struct entry *)*(high-mid));
	int i;
	for (i=0; i<mid-low+1; i++){
		tempArray1[i] = entries[low+i];
	}
	for (i=0; i<high-mid; i++){
		tempArray2[i] = entries[mid+1+i];
	}
	
	//check if memory was not properly allocated by malloc
	if (tempArray1==NULL || tempArray2==NULL){
		printf("Error in allocation of memory\n");
		exit(0);
	}
	
	// insertLocation is the location in entries that will be overwritten
	long insertLocation = low;
	//for the first temporary array
	long index1 = low; 
	//for the second temporary array
	long index2 = mid+1; 
	while (index1 <= mid && index2 <= high) { //the lower array gets the middle element
		//compare succeeding elements in tempArray1 vs succeeding elements in tempArray2
		//dereference tempArray(1,2) at an index, dereference and grab values, dereference and grab string, decimal, or float value
		//compareValue returns -1 when element in tempArray1 is smaller and 1 whenelement in tempArray2 is bigger
		if (compareValue((tempArray1[index1-low]), (tempArray2[index2-(mid+1)]), columnTypes, compareIndexes, numberOfSortBys) == -1) {
			//if tempArray1 has the smaller value or they're equal: this makes merge sort stable
			entries[insertLocation] = tempArray1[index1-low];
			index1++;
		} else { //if tempArray2  has the smalller value
			entries[insertLocation] = tempArray2[index2-(mid+1)];
			index2++;
		}
		//insertLocation will never go out of bounds because the loop stops when all the values put back into entries from low to high 
		insertLocation++;
	}
	
	//if tempArray1 still has more entries, put at the end of entries
	while (index1 <= mid) {
		entries[insertLocation] = tempArray1[index1-low];
		index1++;
		insertLocation++;
	}

	//if tempArray2 still has more entries, put at the end of entries
	while (index2 <= high) {
		entries[insertLocation] = tempArray2[index2-(mid+1)];
		index2++;
		insertLocation++;
	}
	
	free(tempArray1);
	free(tempArray2);
	
	return;
} 

int compareValue(struct entry *tempArray1, struct entry *tempArray2, enum type *columnTypes, int *compareIndexes, int numberOfSortBys) {
	//the values could be string, integer, or decimal
	int counter = 0;
	union value *location1;
	union value *location2;
	enum type dataType;
	int temp=0;
	while (counter < numberOfSortBys) {
		location1 = &(tempArray1->values[compareIndexes[counter]]);
		location2 = &(tempArray2->values[compareIndexes[counter]]);
		dataType = columnTypes[compareIndexes[counter]];
		if (dataType == string) {
			temp = strcmp(location1->stringVal,location2->stringVal);
			if (temp < 0) {
				return -1; //first value is smaller or equal
			} else if (temp > 0) {
				return 1; //first value is bigger
			} else {
				counter++;
				continue;
			}
		} else if (dataType == integer) {
			temp = (location1->intVal) - (location2->intVal);
			if (temp < 0) {
				return -1; //first value is smaller or equal
			} else if (temp > 0) {
				return 1; //first value is bigger
			} else {
				counter++;
				continue;
			}
		} else if (dataType == decimal) {
			temp = (location1->decimalVal) - (location2->decimalVal);
			if (temp < 0) {
				return -1; //first value is smaller or equal
			} else if (temp > 0) {
				return 1; //first value is bigger
			} else {
				counter++;
				continue;
			}
		} else {
			printf("Error: compareValue\n");
		}
	}
	return 1; //Both values are exactly the same ==> first value is bigger since mergeSort is stable
}

void printSortedColumn(struct csv *csv, int compareIndex) {
	struct entry** entries = csv->entries;
	long size = csv->numEntries;
	int i;
	for (i=0; i<size; i++){
		printf("%s\n", (entries[i]->values[compareIndex]).stringVal);
	}
	return;
}

void printCSV(struct csv *csv, FILE *file) {
	struct entry** entries = csv->entries;
	long size = csv->numEntries;
	int i;
	int j;
	for (i=0;i<columns;i++) {
		if (i>0) {
			fprintf(file, ",");
		}
		fprintf(file, "%s", csv->columnNames[i]);
	}
	fprintf(file, "\n");
	
	for (i=0; i<size; i++){
		for (j=0; j<columns; j++) {
			if (j>0) {
				fprintf(file, ",");
			}
			enum type columnType = csv->columnTypes[j];
			if (columnType == string) {
				fprintf(file, "\"%s\"", entries[i]->values[j].stringVal);
			} else if (columnType == integer) {
				fprintf(file, "%ld", entries[i]->values[j].intVal);
			} else if (columnType == decimal) {
				fprintf(file, "%f", entries[i]->values[j].decimalVal);
			}
		}
		fprintf(file, "\n");
	}
	return;
}

///Frees CSV struct pointer for future usage.
void freeCSV(struct csv *csv) {
	int i;

	//Free Column Types (Array of Enums)
	free(csv->columnTypes);

	//Free Each Individual Column Name (Dynamically Allocated String)
	for (i=0;i<columns;i++) {
		free(csv->columnNames[i]);
	}

	//Free Column Names (Array of Dynamically Allocated Strings)
	free(csv->columnNames);

	//Free Each Individual CSV Entry (Array of Value Structs)
	for (i=0;i<maxEntries;i++) {
		free(csv->entries[i]);
	}

	//Free CSV Entries (Array of Entry Pointers)
	free(csv->entries);

	//Free CSV (Dynamically Allocated Struct)
	free(csv);
}

char *addCharacterToString(char *string, char next, int position){
	if (position >= maxStringSize) {
		maxStringSize *= 10;
		string = realloc(string, sizeof(char) * maxStringSize);
	}
	string[position] = next;

	return string;
}

struct entry **addEntryToArray(struct entry **array, struct entry *entry, int position) {
	if (position >= maxEntries) {
		maxEntries *= 10;
		array = realloc(array, sizeof(struct entry) * maxEntries);
	}
	array[position] = entry;

	return array;
}

void setValue(union value *location, char *value, enum type dataType) {
	if (dataType == string) {
		location->stringVal = value;
	} else if (dataType == integer) {
		location->intVal = atoi(value);
	} else if (dataType == decimal) {
		location->decimalVal = atof(value);
	} else {
		printf("Error: Unknown column type for value: %s.\n", value);
	}
}

int parseDir(char *inputDir, char *outputDir, char *sortBy){
	
	struct dirent * pDirent;
	DIR *dir = NULL;
	if (inputDir == NULL) {
		printf("Case1\n");
		dir = opendir(".");
	} else {
		printf("Case2\n");
		dir = opendir(inputDir);
	}
	
	if (dir == NULL) {
		printf("Cannot open directory: %s\n", inputDir);
		exit(0);
	} else {
		printf("Directory opened with name: %s\n", dir);
	}
	
	int numChildProcesses = 0;
	int totalNumProcesses = 1;
	
	int limitChildren = 0;
	while (((pDirent = readdir(dir)) != NULL) && limitChildren < 300) {
		if ((isCSV(pDirent->d_name)==1) && (pDirent->d_type == DT_REG)) {
			printf("Regular CSV with name: %s\n", pDirent->d_name);
			/*if (fork()==0){
				printf("CHILD1PID: %d\n", getpid());
				exit(sortFile(inputDir, outputDir, pDirent->d_name, sortBy));
			} else {
				numChildProcesses++;
			}*/
			
		} else if (pDirent->d_type == DT_DIR) {
			char *subDir = (char *)malloc((strlen(inputDir)+strlen(pDirent->d_name))* sizeof(char));
			//memcpy(subDir, inputDir, strlen(inputDir));
			strcat(subDir, inputDir);
			strcat(subDir, pDirent->d_name);
			printf("Regular directory with name: %s\n", subDir);
			/*if (fork()==0){
				printf("CHILD2PID: %d", getpid());
				int retVal = parseDir(subDir, outputDir, sortBy);
				free(subDir);
				exit(retVal);
			} else {
				numChildProcesses++;
				free(subDir);
			}*/
			
		}
		if (limitChildren == 299) {
			printf("\n\n\n\nPREVENT FORK BOMB\n\n\n");
			break;
		}
		limitChildren++;
	}
	
	int i;
	int pid = 0;
	int status = 0;
	for (i=0;i<numChildProcesses-1;i++) {
		pid = wait(&status);
		printf("PID was returned to Parent: %d\n", pid);
		totalNumProcesses += status;
	}
	return totalNumProcesses;
}

int isCSV(char *fname){
	int i = 0;
	while (fname[i]!='\0') {
		i++;
	}
	if (fname[i-4]=='.' && fname[i-3]=='c' && fname[i-2]=='s' && fname[i-1]=='v') {
		return 1;
	}
	
	return 0;
}

int sortFile(char *inputDir, char *outputDir, char *fileName, char *sortBy){
	
	
	FILE *in = fopen(fileName, "r");
	
	char* outputFilename = calloc(1, (strlen(fileName) + strlen("-sorted-") + strlen(sortBy) + 1) * sizeof(char));
	strcat(outputFilename, fileName);
	strcat(outputFilename, "-sorted-");
	strcat(outputFilename, sortBy);
	strcat(outputFilename, ".csv");
	
	
	FILE *out;
	if (outputDir != NULL) {
		strcat(outputDir, "/");
		strcat(outputDir, outputFilename);
		out = fopen(outputDir, "w");
	} else {
		out = fopen(outputFilename, "w");
	}
	//struct csv takes in the whole csv file
	struct csv *csv = parseCSV(in);
	
	
	//char *sortBy = argv[2];
	//!!code changed to handle query that has mutliple sort by values, comma separated
	//array of strings
	char **columnNames = csv->columnNames;
	
	//find the indexes of the desired field to sort by; color = 0, director_name = 1 ...
	int numberOfSortBys = 1;
	int i; 
	char *query = sortBy;
	for (i=0; query[i]!='\0'; i++) {
		if (query[i] == ',') {
			numberOfSortBys += 1;
		}
	}
	
	//all the sortBy values separated
	char **arrayOfSortBys = (char **)malloc(numberOfSortBys * sizeof(char *));
	int counter = 0;
	
	//parse out the different sortBy values
	char *temp = query;
	for (i=0; query[i]!='\0'; i++) {
		if (query[i] == ',') {
			char *sortVal = (char *) malloc((query-temp+1) * sizeof(char));
			memcpy(sortVal, temp, (query-temp));
			sortVal[query-temp] = '\0';
			arrayOfSortBys[counter] = sortVal;
			counter++;
			temp=query+1;
		}
	}
	//for the last value after the last comma
	char *sortVal = (char *) malloc((query-temp+1) * sizeof(char));
	memcpy(sortVal, temp, (query-temp));
	sortVal[query-temp] = '\0';
	arrayOfSortBys[counter] = sortVal;
	
	
	int *indexesOfSortBys = (int *) malloc(numberOfSortBys * sizeof(int));
	counter = 0;
	for (i=0; i<numberOfSortBys; i++) {
		for (i=0; i < columns; i++) {
			if (strcmp(columnNames[i], arrayOfSortBys[counter])==0) {
				indexesOfSortBys[counter] = i;
				counter++;
			}
			//check if header is found
			if (i == columns) {
				printf("Error, could not find query in column names\n");
				exit(0);
			}
		}
	}
	
	//free the parsed character array of query
	for (i=0; i<numberOfSortBys; i++) {
		free(arrayOfSortBys[i]);
	}
	free(arrayOfSortBys);
	//==================
	
	//sorts csv by sortBy
	mergesortMovieList(csv, indexesOfSortBys, csv->columnTypes, numberOfSortBys);
	
	free(indexesOfSortBys);
	
	//prints out the whole csv in sorted order
	printCSV(csv, out);
	
	freeCSV(csv);
	
	return 0;
}