/**
 * LC-2K Linker
 **/

#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define MAXSIZE 500
#define MAXLINELENGTH 1000
#define MAXFILES 6

typedef struct FileData FileData;
typedef struct SymbolTableEntry SymbolTableEntry;
typedef struct RelocationTableEntry RelocationTableEntry;
typedef struct CombinedFiles CombinedFiles;

struct SymbolTableEntry
{
	char label[7];
	char location; // U T D
	unsigned int offset;
};

struct RelocationTableEntry
{
	unsigned int offset;
	char inst[7];
	char label[7];
	unsigned int file;
};

struct FileData
{
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	unsigned int textStartingLine; // in final executable
	unsigned int dataStartingLine; // in final executable
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry symbolTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
};

struct CombinedFiles
{
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	int text[MAXSIZE * MAXFILES];
	int data[MAXSIZE * MAXFILES];
	SymbolTableEntry symbolTable[MAXSIZE * MAXFILES];
	RelocationTableEntry relocTable[MAXSIZE * MAXFILES];
};

// function declarations for readability:
void readObjectFiles(char *argv[], int argc, FileData files[], int maxFiles, FILE **outFilePtr);
void errorChecks(FileData files[], int fileCount);
int extractOffsetField(int instruction);
int locallyDefinedLabelOffset(FileData files[], int currentFile, int lineNum, int fileCount, FILE *outFilePtr, int labelIndex);
bool isDataSection(int offset, int textSize);
int resolveGlobalLabel(FileData files[], int currentFile, int lineNum, int fileCount, char currentLabel[7], int relocatedCount, FILE *outFilePtr);
void handleLocalLabel(FileData files[], int fileNum, int lineNum, int fileCount, int relocatedCount, FILE *outFilePtr, int *offset);

// int locallyDefinedFillOffset(FileData files[], int currentFile, int lineNum, int fileCount, bool global, bool data);
int main(int argc, char *argv[])
{
	FileData files[MAXFILES];
	FILE *outFilePtr = NULL;

	// Call the new function
	readObjectFiles(argv, argc, files, MAXFILES, &outFilePtr);

	// files hold each file 0 - N
	// file object has all tables
	int fileCount = argc - 2;

	errorChecks(files, fileCount);

	int stackAddress = 0;
	for (int i = 0; i < fileCount; i++)
	{
		stackAddress += (files[i].textSize + files[i].dataSize);
	}

	// printForDebug(files, fileCount);
	for (int fileNum = 0; fileNum < fileCount; fileNum++)
	{
		int relocatedCount = 0;
		// text section
		for (int lineNum = 0; lineNum < files[fileNum].textSize; lineNum++)
		{
			if (((strcmp(files[fileNum].relocTable[relocatedCount].inst, "sw") == 0) || (strcmp(files[fileNum].relocTable[relocatedCount].inst, "lw") == 0)) && (files[fileNum].relocTable[relocatedCount].offset == lineNum))
			{
				int offsetAddition = 0;

				char currentLabel[7];
				strcpy(currentLabel, files[fileNum].relocTable[relocatedCount].label);
				// stack address
				if (strcmp("Stack", currentLabel) == 0)
				{
					offsetAddition += stackAddress;
				}
				// local address
				else if (isupper(currentLabel[0]) == 0)
				{
					handleLocalLabel(files, fileNum, lineNum, fileCount, relocatedCount, outFilePtr, &offsetAddition);
				}
				// global address
				else
				{
					offsetAddition += resolveGlobalLabel(files, fileNum, lineNum, fileCount, currentLabel, relocatedCount, outFilePtr);
				}

				files[fileNum].text[lineNum] += offsetAddition;
				relocatedCount++;
			} // if statment
			// print all
			fprintf(outFilePtr, "%d\n", files[fileNum].text[lineNum]);
		} // text loop
	}

	// data loop
	for (int fileNum = 0; fileNum < fileCount; fileNum++)
	{
		for (int lineNum = 0; lineNum < files[fileNum].dataSize; lineNum++)
		{
			bool foundRelocEntry = false;
			int offsetAddition = 0;
			int relocatedCount = 0;
			char currentLabel[7];
			for (int relocIndex = 0; relocIndex < files[fileNum].relocationTableSize; relocIndex++)
			{
				if (files[fileNum].relocTable[relocIndex].offset == lineNum && strcmp(".fill", files[fileNum].relocTable[relocIndex].inst) == 0)
				{
					// Match found, process this relocation entry
					foundRelocEntry = true;
					relocatedCount = relocIndex;
					strcpy(currentLabel, files[fileNum].relocTable[relocIndex].label);
					break;
				}
			}

			if (foundRelocEntry)
			{
				if (strcmp("Stack", currentLabel) == 0)
				{
					offsetAddition += stackAddress;
				}
				// local address
				else if (isupper(currentLabel[0]) == 0)
				{
					handleLocalLabel(files, fileNum, lineNum, fileCount, relocatedCount, outFilePtr, &offsetAddition);
				}
				// global address
				else
				{
					offsetAddition += resolveGlobalLabel(files, fileNum, lineNum, fileCount, currentLabel, relocatedCount, outFilePtr);
				}
				files[fileNum].data[lineNum] += offsetAddition;
			}

			// Print or process the data line
			fprintf(outFilePtr, "%d\n", files[fileNum].data[lineNum]);
		} // data loop
	}
	// file loop
	return 0;
}
// main
int resolveGlobalLabel(FileData files[], int currentFile, int lineNum, int fileCount, char currentLabel[7], int relocatedCount, FILE *outFilePtr)
{
	int offsetAddition = 0;
	bool localLabel = false;
	for (int i = 0; i < files[currentFile].symbolTableSize; i++)
	{
		if (strcmp(files[currentFile].symbolTable[i].label, currentLabel) == 0)
		{
			switch (files[currentFile].symbolTable[i].location)
			{
			case 'U':
				break;
			case 'T':
				localLabel = true;
				break;
			case 'D':
				localLabel = true;
				break;
				printf("DEBUG: error, symbol type not recognized");
				exit(1);
			}
		}
	}

	// local & locally defined-global labels need to be offset by start of current text file
	if (localLabel)
	{
		handleLocalLabel(files, currentFile, lineNum, fileCount, relocatedCount, outFilePtr, &offsetAddition);
	}
	// undef. global labels need to be offset by start of text theyre defined in
	else
	{ // undef global
		int globalLabelFile = 0;
		int globalLabelFileIndex = 0;
		// initialize as P for debugging :))
		char globalLabelType = 'P';
		for (int i = 0; i < fileCount; i++)
		{
			for (int j = 0; j < files[i].symbolTableSize; j++)
			{
				if ((strcmp(currentLabel, files[i].symbolTable[j].label) == 0) && (files[i].symbolTable[j].location != 'U'))
				{
					globalLabelFile = i;
					globalLabelFileIndex = j;
					globalLabelType = files[i].symbolTable[j].location;
				}
			}
		}
		if (globalLabelType == 'P' || globalLabelType == 'U')
		{
			printf("DEBUG: error, global label not found");
			exit(1);
		}
		// text section
		if (globalLabelType == 'T')
		{
			// add line of file that label is defined in
			for (int i = 0; i < globalLabelFile; i++) {
				offsetAddition += files[i].textSize;
			}
			offsetAddition += files[globalLabelFile].symbolTable[globalLabelFileIndex].offset;
		}
		else
		{
			for (int i = 0; i < fileCount; i++)
			{
				offsetAddition += files[i].textSize;
			}
			for (int i = 0; i < globalLabelFile; i++)
			{
				offsetAddition += files[i].dataSize;
			}
			offsetAddition += files[globalLabelFile].symbolTable[globalLabelFileIndex].offset;

		}
	}
	return offsetAddition;
}
// int globalLabelOffset
void errorChecks(FileData files[], int fileCount)
{
	char uniqueLabels[MAXSIZE][7];
	char definedGlobalLabels[MAXSIZE][7];
	int definedCount = 0;
	int uniqueCount = 0;
	// go through each file
	for (int fileNum = 0; fileNum < fileCount; fileNum++)
	{
		// go through symbol table for this file.
		for (int label = 0; label < files[fileNum].symbolTableSize; label++)
		{
			bool found = false;
			for (int i = 0; i < definedCount; i++)
			{
				if (strcmp(files[fileNum].symbolTable[label].label, definedGlobalLabels[i]) == 0)
				{
					found = true;
					// if entered, so label is the same. Have to make sure it's undefined
					if (files[fileNum].symbolTable[label].location != 'U')
					{
						printf("Error, duplicate def. of global label: %s\n", files[fileNum].symbolTable[label].label);
						exit(1);
					}
				}
			}
			if (strcmp(files[fileNum].symbolTable[label].label, "Stack") == 0)
			{
				if (files[fileNum].symbolTable[label].location != 'U')
				{
					printf("Error, Stack defined by object file: %s\n", files[fileNum].symbolTable[label].label);
					exit(1);
				}
			}
			// match not found, so not defined yet.
			if (!found)
			{
				if (files[fileNum].symbolTable[label].location != 'U')
				{
					strcpy(definedGlobalLabels[definedCount++], files[fileNum].symbolTable[label].label);
				}
				strcpy(uniqueLabels[uniqueCount++], files[fileNum].symbolTable[label].label);
			}
		}
	}

	for (int i = 0; i < uniqueCount; i++)
	{
		bool found = false;
		for (int j = 0; j < definedCount; j++)
		{

			if (strcmp(uniqueLabels[i], definedGlobalLabels[j]) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found && (strcmp("Stack", uniqueLabels[i]) != 0))
		{
			printf("Error, undefined global label: %s\n", uniqueLabels[i]);
			exit(1);
		}
	}
} // for 1

void readObjectFiles(char *argv[], int argc, FileData files[], int maxFiles, FILE **outFilePtr)
{
	char *outFileString;
	FILE *inFilePtr;
	unsigned int i, j;

	if (argc <= 2)
	{
		printf("error: usage: %s <obj file> ... <output-exe-file>\n", argv[0]);
		exit(1);
	}

	outFileString = argv[argc - 1];
	*outFilePtr = fopen(outFileString, "w");
	if (*outFilePtr == NULL)
	{
		printf("error in opening %s\n", outFileString);
		exit(1);
	}

	for (i = 0; i < argc - 2; i++)
	{
		char *inFileString = argv[i + 1];
		inFilePtr = fopen(inFileString, "r");
		// debug:
		// printf("opening %s\n", inFileString);

		if (inFilePtr == NULL)
		{
			printf("error in opening %s\n", inFileString);
			exit(1);
		}

		char line[MAXLINELENGTH];
		unsigned int textSize, dataSize, symbolTableSize, relocationTableSize;

		fgets(line, MAXSIZE, inFilePtr);
		sscanf(line, "%d %d %d %d", &textSize, &dataSize, &symbolTableSize, &relocationTableSize);

		// Initialize fields of files[i]
		files[i].textSize = textSize;
		files[i].dataSize = dataSize;
		files[i].symbolTableSize = symbolTableSize;
		files[i].relocationTableSize = relocationTableSize;

		// read in text section
		int instr;
		for (j = 0; j < textSize; j++)
		{
			fgets(line, MAXLINELENGTH, inFilePtr);
			instr = strtol(line, NULL, 0);
			files[i].text[j] = instr;
		}

		// read in data section
		int data;
		for (j = 0; j < dataSize; j++)
		{
			fgets(line, MAXLINELENGTH, inFilePtr);
			data = strtol(line, NULL, 0);
			files[i].data[j] = data;
		}

		// read in the symbol table
		char label[7];
		char type;
		unsigned int addr;
		for (j = 0; j < symbolTableSize; j++)
		{
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%s %c %d",
				   label, &type, &addr);
			files[i].symbolTable[j].offset = addr;
			strcpy(files[i].symbolTable[j].label, label);
			files[i].symbolTable[j].location = type;
		}

		// read in relocation table
		char opcode[7];
		for (j = 0; j < relocationTableSize; j++)
		{
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%d %s %s",
				   &addr, opcode, label);
			files[i].relocTable[j].offset = addr;
			strcpy(files[i].relocTable[j].inst, opcode);
			strcpy(files[i].relocTable[j].label, label);
			files[i].relocTable[j].file = i;
		}
		fclose(inFilePtr);
		// end reading files
	}
}

int extractOffsetField(int instruction)
{
	// Mask out all but the offset field
	// where offsetField is the 16 least significant bits
	int offsetField = instruction & 0xFFFF; // Mask to keep only the last 16 bits

	// Check if the offset field is negative, sign extension
	if (offsetField & 0x8000)
	{							   // If the most significant bit of the offset is 1 (negative number)
		offsetField |= 0xFFFF0000; // Extend the sign to the entire 32-bit integer
	}

	return offsetField;
}

bool isDataSection(int offset, int textSize)
{
	return offset >= textSize;
}

void handleLocalLabel(FileData files[], int fileNum, int lineNum, int fileCount, int relocatedCount, FILE *outFilePtr, int *offset)
{
	char *currentLabel = files[fileNum].relocTable[relocatedCount].label;
	// Initial addition based on the label's relative location
	int localOffset = locallyDefinedLabelOffset(files, fileNum, lineNum, fileCount, outFilePtr, relocatedCount);

	int offsetAddition = localOffset;

	int baseOffSet = 0;

	if (strcmp(".fill", files[fileNum].relocTable[relocatedCount].inst) == 0)
	{
		baseOffSet = extractOffsetField(files[fileNum].data[lineNum]);
	}
	else
	{
		baseOffSet = extractOffsetField(files[fileNum].text[lineNum]);
	}

	// global, find location from def in local table
	if (islower(currentLabel[0]) == 0)
	{
		for (int i = 0; i < files[fileNum].symbolTableSize; i++)
		{
			if (strcmp(currentLabel, files[fileNum].symbolTable[i].label) == 0)
			{
				offsetAddition += files[fileNum].symbolTable[i].offset;
				// delete offset from file, replace with symbol table offset^
				offsetAddition -= baseOffSet;
			}
		}
	}
	
	// local, 2 cases
	else
	{
		if (((strcmp(".fill", files[fileNum].relocTable[relocatedCount].inst) == 0))
		&& (isDataSection(baseOffSet, files[fileNum].textSize))) {
			offsetAddition -= files[fileNum].textSize;
		}
		// if its greater than or equal to size, its in data section, else leave the same
		else if (((strcmp("lw", files[fileNum].relocTable[relocatedCount].inst) == 0) 
		|| (strcmp("sw", files[fileNum].relocTable[relocatedCount].inst) == 0)) 
		&& (!isDataSection(baseOffSet, files[fileNum].textSize)))
		{
			offsetAddition -= baseOffSet;
			// Extracting and adjusting the base offset in the instruction
		}
		else if ((isDataSection(baseOffSet, files[fileNum].textSize)) 
		&& ((strcmp("lw", files[fileNum].relocTable[relocatedCount].inst) == 0) 
		|| (strcmp("sw", files[fileNum].relocTable[relocatedCount].inst) == 0)))
		{
			//add the relative data label location
			offsetAddition -= baseOffSet;
			offsetAddition += baseOffSet - files[fileNum].textSize;
			// Extracting and adjusting the base offset in the instruction
		}
	}
	// Debugging the total offset addition for this label
	(*offset) += offsetAddition;
}

int locallyDefinedLabelOffset(FileData files[], int currentFile, int lineNum, int fileCount, FILE *outFilePtr, int labelIndex)
{
	int offsetAddition = 0;

	bool data = false;
	int offsetBase = 0;
	if (strcmp(".fill", files[currentFile].relocTable[labelIndex].inst) == 0)
	{
		offsetBase = extractOffsetField(files[currentFile].data[lineNum]);
		data = isDataSection(files[currentFile].data[lineNum], files[currentFile].textSize);
	}
	else
	{
		offsetBase = extractOffsetField(files[currentFile].text[lineNum]);
	}
	data = isDataSection(offsetBase, files[currentFile].textSize);
	// in data section
	if (data)
	{
		for (int i = 0; i < fileCount; i++)
		{
			offsetAddition += files[i].textSize;
		}
		for (int i = 0; i < currentFile; i++)
		{
			offsetAddition += files[i].dataSize;
		}
	}
	else
	{
		for (int i = 0; i < currentFile; i++)
		{
			offsetAddition += files[i].textSize;
		}
	}
	return offsetAddition;
}
