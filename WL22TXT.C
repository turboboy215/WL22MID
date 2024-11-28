/*Wario Land 2/3 (GB/GBC) (Kozue Ishikawa) to MIDI converter*/
/*Also works with other games*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
long bank;
long masterBank;
long offset;
long tablePtrLoc;
long tableOffset;
int i, j;
char outfile[1000000];
int songNum;
long seqPtrs[4];
long songPtr;
int chanMask;
int numChan;
long bankAmt;
int foundTable = 0;
long firstPtr = 0;
int songTranspose = 0;
int version = 1;
int curInst = 0;

unsigned static char* romData;
unsigned static char* exRomData;

const char MagicBytesA[6] = { 0x69, 0x60, 0x29, 0x09, 0x29, 0x01 };
const char MagicBytesB[6] = { 0x69, 0x60, 0x29, 0x29, 0x29, 0x01 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptr, int numChan);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

int main(int args, char* argv[])
{
	printf("Wario Land 2/3 (GB/GBC) to TXT converter\n");
	printf("Also works with other games\n");
	if (args != 3)
	{
		printf("Usage: WL22TXT <rom> <bank>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			masterBank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				bankAmt = bankSize;
			}
			else
			{
				bankAmt = 0;
			}
			fseek(rom, ((masterBank - 1) * bankSize), SEEK_SET);
			romData = (unsigned char*)malloc(bankSize);
			fread(romData, 1, bankSize, rom);

			/*Try to search the bank for song table loader - earlier version*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesA, 6)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 6;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
				}
			}

			/*Try to search the bank for song table loader - later version*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesB, 6)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 6;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
					version = 2;
				}
			}

			if (foundTable == 1)
			{
				if (version == 1)
				{
					i = tableOffset - bankAmt + 6;
					songNum = 1;
					while (ReadLE16(&romData[i]) < (bankSize * 2) && ReadLE16(&romData[i]) > bankSize)
					{
						songPtr = ReadLE16(&romData[i]);
						bank = romData[i + 2] + 1;
						numChan = romData[i + 5];
						printf("Song %i: 0x%04X (bank %01X), # of channels: %01X\n", songNum, songPtr, bank, numChan);
						fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
						exRomData = (unsigned char*)malloc(bankSize);
						fread(exRomData, 1, bankSize, rom);
						song2txt(songNum, songPtr, numChan);
						i += 6;
						songNum++;
					}
				}

				else if (version == 2)
				{
					i = tableOffset - bankAmt + 8;
					songNum = 1;
					while (ReadLE16(&romData[i]) < (bankSize * 2) && ReadLE16(&romData[i]) > bankSize)
					{
						songPtr = ReadLE16(&romData[i]);
						bank = romData[i + 2] + 1;
						numChan = romData[i + 6];
						if (numChan <= 4)
						{
							printf("Song %i: 0x%04X (bank %01X), # of channels: %01X\n", songNum, songPtr, bank, numChan);
							fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
							exRomData = (unsigned char*)malloc(bankSize);
							fread(exRomData, 1, bankSize, rom);
							song2txt(songNum, songPtr, numChan);
							i += 8;
							songNum++;
						}
						else
						{
							break;
						}

					}
				}
			}

			else
			{
				printf("ERROR: Magic bytes not found!\n");
				exit(1);
			}

			printf("The operation was successfully completed!\n");
			exit(0);
		}
	}
}

void song2txt(int songNum, long ptr, int numChan)
{
	int seqPos = 0;
	int seqEnd = 0;
	int numPat = 0;
	long chanPtrs[4] = { 0, 0, 0, 0 };
	int curNote = 0;
	int curNoteLen = 0;
	long jumpPos = 0;
	long jumpRet = 0;
	unsigned char command[4];
	int curTrack = 0;
	int transpose = 0;
	int tempo = 0;
	sprintf(outfile, "song%i.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%i.txt!\n", songNum);
		exit(2);
	}
	else
	{
		/*Get initial information*/
		seqPos = ptr - bankAmt;
		numChan = exRomData[seqPos];
		fprintf(txt, "Number of channels: %i\n", numChan);
		numPat = exRomData[seqPos + 1] + 1;
		fprintf(txt, "Number of patterns: %i\n", numPat);
		seqPos += 2;

		/*Get the pointers to each channel! (only the first "pattern" is relevant for conversion)*/
		for (curTrack = 0; curTrack < numChan; curTrack++)
		{
			chanPtrs[curTrack] = ReadLE16(&exRomData[seqPos]);
			fprintf(txt, "Channel %i pattern: 0x%04X\n", (curTrack + 1), chanPtrs[curTrack]);
			seqPos += 2;
		}

		fprintf(txt, "\n");

		/*Now process each channel pattern...*/
		for (curTrack = 0; curTrack < numChan; curTrack++)
		{
			seqPos = chanPtrs[curTrack] - bankAmt;
			seqEnd = 0;
			fprintf(txt, "Channel %i:\n", (curTrack + 1));

			while (seqEnd == 0)
			{
				command[0] = exRomData[seqPos];
				command[1] = exRomData[seqPos + 1];
				command[2] = exRomData[seqPos + 2];
				command[3] = exRomData[seqPos + 3];

				if (command[0] < 0x24)
				{
					fprintf(txt, "Set envelope: %01X\n", command[0]);
					seqPos++;
				}

				else if (command[0] >= 0x24 && command[0] < 0x80)
				{
					curNote = command[0];
					fprintf(txt, "Set note: %01X\n", curNote);
					seqPos++;
				}

				else if (command[0] >= 0x80 && command[0] <= 0xB0)
				{
					curNoteLen = command[0];
					fprintf(txt, "Play note: length %01X\n", curNoteLen);
					seqPos++;
				}

				else if (command[0] == 0xB1)
				{
					fprintf(txt, "Stop track\n\n");
					seqEnd = 1;
				}

				else if (command[0] == 0xB2)
				{
					jumpPos = ReadLE16(&exRomData[seqPos + 1]);
					fprintf(txt, "Go to loop point: %04X\n\n", jumpPos);
					seqEnd = 1;
				}

				else if (command[0] == 0xB3)
				{
					jumpPos = ReadLE16(&exRomData[seqPos + 1]);
					fprintf(txt, "Jump to position: 0x%04X\n", jumpPos);
					seqPos += 3;
				}

				else if (command[0] == 0xB4)
				{
					fprintf(txt, "Return\n");
					seqPos++;
				}

				else if (command[0] == 0xBC)
				{
					tempo = command[1];
					fprintf(txt, "Set tempo: %01X\n", tempo);
					seqPos += 2;
				}

				else if (command[0] == 0xBD)
				{
					transpose = (signed char)command[1];
					fprintf(txt, "Set transpose: %01X\n", transpose);
					seqPos += 2;
				}

				else if (command[0] == 0xBE)
				{
					curInst = command[1];
					fprintf(txt, "Set instrument: %01X\n", curInst);
					seqPos += 2;
				}

				else if (command[0] == 0xBF)
				{
					fprintf(txt, "Set parameters: %01X\n", command[1]);
					seqPos += 2;
				}

				else if (command[0] >= 0xC0 && command[0] < 0xCF)
				{
					fprintf(txt, "Effect: %01X, %01X\n", command[0], command[1]);
					seqPos += 2;
				}

				else if (command[0] == 0xCF)
				{
					fprintf(txt, "Effect: %01X\n", command[0]);
					seqPos++;
				}

				else if (command[0] >= 0xD0)
				{
					fprintf(txt, "Change note size: %01X\n", command[0]);
					seqPos++;
				}

				else
				{
					fprintf(txt, "Unknown command: %01X\n", command[0]);
					seqPos++;
				}
			}
		}

		fclose(txt);
	}
}