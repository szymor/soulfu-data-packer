#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEFAULT_FOLDER "sfd"

#define OBFUSCATEA ((unsigned char) 97)
#define OBFUSCATEB ((unsigned char) 11)
#define OBFUSCATEC ((unsigned char) 53)
#define OBFUSCATED ((unsigned char) 37)

enum ErrCode
{
	EC_NONE,
	EC_BADMAGIC,
	EC_FOLDEREXISTS
};

const char sdf_extension[16][4] = {
	"BAD", "TXT", "JPG", "OGG",
	"RGB", "RAW", "SRF", "MUS",
	"DAT", "SRC", "RUN", "PCX",
	"LAN", "DDD", "RDY", "TIL"
};

void help(void);
int pack(void);
int unpack(void);

int main(int argc, char *argv[])
{
	printf("SoulFu data packer\n\n");

	if (1 == argc)
	{
		printf("no arguments provided\n\n");
		help();
		return EC_NONE;
	}
	else if (2 == argc)
	{
		if (!strcmp(argv[1], "-p"))
		{
			return pack();
		}
		else if (!strcmp(argv[1], "-u"))
		{
			return unpack();
		}
	}

	return EC_NONE;
}

void help(void)
{
	static const char contents[] = "Quick help:\n"
		"-p    packs sfd folder into datafile.sdf\n"
		"-u    unpacks datafile.sdf to sfd folder\n";
	printf(contents);
}

int pack(void)
{
}

int unpack(void)
{
	FILE *input = NULL;
	input = fopen("datafile.sdf", "rb");

	char magic[32];
	fseek(input, 16, SEEK_SET);
	fread(magic, 16, 1, input);
	if (!memcmp(magic, "SOULFU DATA FILE", 16))
	{
		printf("Magic string OK.\n");
	}
	else
	{
		printf("Error: invalid magic string.\n");
		return EC_BADMAGIC;
	}

	int filenum = 0;
	fseek(input, 32, SEEK_SET);
	fscanf(input, "%d", &filenum);
	printf("Unpacking %d files...\n", filenum);

	int ret = 0;
#ifdef __MINGW32__
	ret = mkdir(DEFAULT_FOLDER);
#else
	ret = mkdir(DEFAULT_FOLDER, 0644);
#endif
	if (0 != ret)
	{
		printf("Error: output folder exists.\n");
		return EC_FOLDEREXISTS;
	}

	int index_entry_pos = 64;
	for (int i = 0; i < filenum; ++i)
	{
		unsigned char index[16];
		unsigned char filename[16];

		fseek(input, index_entry_pos, SEEK_SET);
		fread(index, 16, 1, input);
		index[0] -= OBFUSCATEA;
		index[1] -= OBFUSCATEA;
		index[2] -= OBFUSCATEA;
		index[3] -= OBFUSCATEA;
		index[4] -= OBFUSCATEB;
		index[5] -= OBFUSCATEB;
		index[6] -= OBFUSCATEB;
		index[7] -= OBFUSCATEB;
		index[8] -= OBFUSCATEC;
		index[9] -= OBFUSCATEC;
		index[10] -= OBFUSCATEC;
		index[11] -= OBFUSCATEC;
		index[12] -= OBFUSCATEC;
		index[13] -= OBFUSCATEC;
		index[14] -= OBFUSCATEC;
		index[15] -= OBFUSCATEC;

		memcpy(filename, index + 8, 8);
		filename[8] = '\0';
		int filetype = *(index + 4) & 15;
		strcat(filename, ".");
		strcat(filename, sdf_extension[filetype]);
		printf("%s\n", filename);

		// save the file
		unsigned char filepath[32];
		sprintf(filepath, "%s/%s", DEFAULT_FOLDER, filename);
		FILE *output = NULL;
		output = fopen(filepath, "wb");

		int offset = index[0] << 24 | index[1] << 16 | index[2] << 8 | index[3];
		fseek(input, 64 + filenum * 16 + 16 + offset, SEEK_SET);

		int filesize = index[5] << 16 | index[6] << 8 | index[7];
		while (filesize--)
		{
			unsigned char data;
			fread(&data, 1, 1, input);
			data -= OBFUSCATED;
			fwrite(&data, 1, 1, output);
		}

		fclose(output);

		index_entry_pos += 16;
	}

	fclose(input);
}
