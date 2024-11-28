/*Wario Land 2/3 (GB/GBC) (Kozue Ishikawa) to MIDI converter*/
/*Also works with other games*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * mid;
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
unsigned static char* midData;
unsigned static char* ctrlMidData;

long midLength;

const char MagicBytesA[6] = { 0x69, 0x60, 0x29, 0x09, 0x29, 0x01 };
const char MagicBytesB[6] = { 0x69, 0x60, 0x29, 0x29, 0x29, 0x01 };

const char NoteLen[0x31] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
0x1C, 0x1E, 0x20, 0x24, 0x28, 0x2A, 0x2C, 0x30, 0x34, 0x36, 0x38, 0x3C, 0x40, 0x42, 0x44,
0x48, 0x4C, 0x4E, 0x50, 0x54, 0x58, 0x5A, 0x5C, 0x60 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
unsigned int WriteNoteEvent(unsigned static char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst);
int WriteDeltaTime(unsigned static char* buffer, unsigned int pos, unsigned int value);
void song2mid(int songNum, long ptr, int numChan);

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

unsigned int WriteNoteEvent(unsigned static char* buffer, unsigned int pos, unsigned int note, int length, int delay, int firstNote, int curChan, int inst)
{
	int deltaValue;
	deltaValue = WriteDeltaTime(buffer, pos, delay);
	pos += deltaValue;

	if (firstNote == 1)
	{
		if (curChan != 3)
		{
			Write8B(&buffer[pos], 0xC0 | curChan);
		}
		else
		{
			Write8B(&buffer[pos], 0xC9);
		}

		Write8B(&buffer[pos + 1], inst);
		Write8B(&buffer[pos + 2], 0);

		if (curChan != 3)
		{
			Write8B(&buffer[pos + 3], 0x90 | curChan);
		}
		else
		{
			Write8B(&buffer[pos + 3], 0x99);
		}

		pos += 4;
	}

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 100);
	pos++;

	deltaValue = WriteDeltaTime(buffer, pos, length);
	pos += deltaValue;

	Write8B(&buffer[pos], note);
	pos++;
	Write8B(&buffer[pos], 0);
	pos++;

	return pos;

}

int WriteDeltaTime(unsigned static char* buffer, unsigned int pos, unsigned int value)
{
	unsigned char valSize;
	unsigned char* valData;
	unsigned int tempLen;
	unsigned int curPos;

	valSize = 0;
	tempLen = value;

	while (tempLen != 0)
	{
		tempLen >>= 7;
		valSize++;
	}

	valData = &buffer[pos];
	curPos = valSize;
	tempLen = value;

	while (tempLen != 0)
	{
		curPos--;
		valData[curPos] = 128 | (tempLen & 127);
		tempLen >>= 7;
	}

	valData[valSize - 1] &= 127;

	pos += valSize;

	if (value == 0)
	{
		valSize = 1;
	}
	return valSize;
}

int main(int args, char* argv[])
{
	printf("Wario Land 2/3 (GB/GBC) to MIDI converter\n");
	printf("Also works with other games\n");
	if (args != 3)
	{
		printf("Usage: WL22MID <rom> <bank>\n");
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
						song2mid(songNum, songPtr, numChan);
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
							song2mid(songNum, songPtr, numChan);
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

void song2mid(int songNum, long ptr, int numChan)
{
	static const char* TRK_NAMES[4] = { "Square 1", "Square 2", "Wave", "Noise" };
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
	int trackCnt = 4;
	int ticks = 120;
	int transpose = 0;
	int tempo = 150;
	int k = 0;
	unsigned int midPos = 0;
	unsigned int ctrlMidPos = 0;
	long midTrackBase = 0;
	long ctrlMidTrackBase = 0;
	int valSize = 0;
	long trackSize = 0;
	int rest = 0;
	int tempByte = 0;
	int curDelay = 0;
	int ctrlDelay = 0;
	long tempPos = 0;
	int holdNote = 0;
	long startPos = 0;
	int inMacro = 0;
	int firstNote = 1;
	int playedNote = 0;


	midPos = 0;
	ctrlMidPos = 0;

	midLength = 0x10000;
	midData = (unsigned char*)malloc(midLength);

	ctrlMidData = (unsigned char*)malloc(midLength);

	for (j = 0; j < midLength; j++)
	{
		midData[j] = 0;
		ctrlMidData[j] = 0;
	}

	sprintf(outfile, "song%d.mid", songNum);
	if ((mid = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.mid!\n", songNum);
		exit(2);
	}
	else
	{
		/*Write MIDI header with "MThd"*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D546864);
		WriteBE32(&ctrlMidData[ctrlMidPos + 4], 0x00000006);
		ctrlMidPos += 8;

		WriteBE16(&ctrlMidData[ctrlMidPos], 0x0001);
		WriteBE16(&ctrlMidData[ctrlMidPos + 2], trackCnt + 1);
		WriteBE16(&ctrlMidData[ctrlMidPos + 4], ticks);
		ctrlMidPos += 6;
		/*Write initial MIDI information for "control" track*/
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x4D54726B);
		ctrlMidPos += 8;
		ctrlMidTrackBase = ctrlMidPos;

		/*Set channel name (blank)*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE16(&ctrlMidData[ctrlMidPos], 0xFF03);
		Write8B(&ctrlMidData[ctrlMidPos + 2], 0);
		ctrlMidPos += 2;

		/*Set initial tempo*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF5103);
		ctrlMidPos += 4;

		WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
		ctrlMidPos += 3;

		/*Set time signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5804);
		ctrlMidPos += 3;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0x04021808);
		ctrlMidPos += 4;

		/*Set key signature*/
		WriteDeltaTime(ctrlMidData, ctrlMidPos, 0);
		ctrlMidPos++;
		WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5902);
		ctrlMidPos += 4;

		/*Get initial information*/
		seqPos = ptr - bankAmt;
		numChan = exRomData[seqPos];
		numPat = exRomData[seqPos + 1] + 1;
		seqPos += 2;

		for (curTrack = 0; curTrack < 4; curTrack++)
		{
			chanPtrs[curTrack] = 0;
		}

		/*Get the pointers to each channel! (only the first "pattern" is relevant for conversion)*/
		for (curTrack = 0; curTrack < numChan; curTrack++)
		{
			chanPtrs[curTrack] = ReadLE16(&exRomData[seqPos]);
			seqPos += 2;
		}

		/*Now process each channel pattern...*/
		for (curTrack = 0; curTrack < 4; curTrack++)
		{
			/*Add track header*/
			/*Write MIDI chunk header with "MTrk"*/
			WriteBE32(&midData[midPos], 0x4D54726B);
			midPos += 8;
			midTrackBase = midPos;

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);

			curNote = 0;
			curNoteLen = 0;
			curDelay = 0;
			seqPos = chanPtrs[curTrack] - bankAmt;
			seqEnd = 0;
			inMacro = 0;
			firstNote = 1;
			transpose = 0;
			playedNote = 0;

			if (chanPtrs[curTrack] == 0)
			{
				seqEnd = 1;
			}

			if (songNum == 258 && curTrack == 2)
			{
				seqEnd = 0;
			}
			while (seqEnd == 0)
			{

				command[0] = exRomData[seqPos];
				command[1] = exRomData[seqPos + 1];
				command[2] = exRomData[seqPos + 2];
				command[3] = exRomData[seqPos + 3];

				/*Set envelope*/
				if (command[0] < 0x24)
				{
					playedNote = 1;
					seqPos++;
				}

				/*Set note*/
				else if (command[0] >= 0x24 && command[0] < 0x80)
				{
					curNote = command[0] + transpose;
					playedNote = 1;
					seqPos++;
				}

				/*Play note at specified length*/
				else if (command[0] >= 0x80 && command[0] <= 0xB0)
				{
					curNoteLen = NoteLen[command[0] - 0x80] * 5;
					if (playedNote == 0)
					{
						curDelay += curNoteLen;
						ctrlDelay += curNoteLen;
					}
					else
					{
						tempPos = WriteNoteEvent(midData, midPos, curNote, curNoteLen, curDelay, firstNote, curTrack, curInst);
						firstNote = 0;
						holdNote = 0;
						midPos = tempPos;
						curDelay = 0;
						ctrlDelay += curNoteLen;
						playedNote = 0;
					}
					seqPos++;
				}

				/*Stop track*/
				else if (command[0] == 0xB1)
				{
					seqEnd = 1;
				}

				/*Go to loop point*/
				else if (command[0] == 0xB2)
				{
					seqEnd = 1;
				}

				/*Jump to position*/
				else if (command[0] == 0xB3)
				{
					jumpPos = ReadLE16(&exRomData[seqPos + 1]) - bankAmt;
					jumpRet = seqPos += 3;
					seqPos = jumpPos;
					inMacro = 1;
				}

				/*Return*/
				else if (command[0] == 0xB4)
				{
					if (inMacro == 1)
					{
						seqPos = jumpRet;
						inMacro = 0;
					}
					else
					{
						seqPos++;
					}
				}

				/*Set tempo*/
				else if (command[0] == 0xBC)
				{
					tempo = command[1] * 2;
					ctrlMidPos++;
					valSize = WriteDeltaTime(ctrlMidData, ctrlMidPos, ctrlDelay);
					ctrlDelay = 0;
					ctrlMidPos += valSize;
					WriteBE24(&ctrlMidData[ctrlMidPos], 0xFF5103);
					ctrlMidPos += 3;
					WriteBE24(&ctrlMidData[ctrlMidPos], 60000000 / tempo);
					ctrlMidPos += 2;
					seqPos += 2;
				}

				/*Set transpose*/
				else if (command[0] == 0xBD)
				{
					transpose = (signed char)command[1];
					seqPos += 2;
				}

				/*Set instrument*/
				else if (command[0] == 0xBE)
				{
					if (curInst != command[1])
					{
						curInst = command[1];
						if (curInst >= 0x80)
						{
							curInst = 0;
						}
						firstNote = 1;
					}

					seqPos += 2;
				}

				/*Set parameters*/
				else if (command[0] == 0xBF)
				{
					seqPos += 2;
				}

				/*Effect*/
				else if (command[0] >= 0xC0 && command[0] < 0xCF)
				{
					seqPos += 2;
				}

				/*Effect*/
				else if (command[0] == 0xCF)
				{
					seqPos++;
				}

				/*Change note size*/
				else if (command[0] >= 0xD0)
				{
					playedNote = 1;
					seqPos++;
				}

				/*Unknown command*/
				else
				{
					seqPos++;
				}
			}

			/*End of track*/
			WriteBE32(&midData[midPos], 0xFF2F00);
			midPos += 4;

			/*Calculate MIDI channel size*/
			trackSize = midPos - midTrackBase;
			WriteBE16(&midData[midTrackBase - 2], trackSize);
		}

		/*End of control track*/
		ctrlMidPos++;
		WriteBE32(&ctrlMidData[ctrlMidPos], 0xFF2F00);
		ctrlMidPos += 4;

		/*Calculate MIDI channel size*/
		trackSize = ctrlMidPos - ctrlMidTrackBase;
		WriteBE16(&ctrlMidData[ctrlMidTrackBase - 2], trackSize);

		sprintf(outfile, "song%d.mid", songNum);
		fwrite(ctrlMidData, ctrlMidPos, 1, mid);
		fwrite(midData, midPos, 1, mid);
		fclose(mid);
	}
}