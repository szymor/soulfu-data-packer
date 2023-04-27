#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#define DEFAULT_DIRECTORY "sfd"
#define DEFAULT_SDF_FILENAME "datafile.sdf"

#define OBFUSCATEA ((unsigned char) 97)
#define OBFUSCATEB ((unsigned char) 11)
#define OBFUSCATEC ((unsigned char) 53)
#define OBFUSCATED ((unsigned char) 37)

enum ErrCode
{
	EC_NONE,
	EC_BADMAGIC,
	EC_DIREXISTS,
	EC_NODIR
};

struct IndexEntry
{
	unsigned int pos;
	unsigned int size;
	unsigned char path[32];
	unsigned char name[16];
	unsigned char ext[8];
	unsigned char type;
	struct IndexEntry *next;
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

void write_big_endian(unsigned char dst[4], unsigned int n)
{
	dst[0] = (n >> 24) & 0xff;
	dst[1] = (n >> 16) & 0xff;
	dst[2] = (n >> 8) & 0xff;
	dst[3] = (n >> 0) & 0xff;
}

unsigned char get_type_from_ext(char *ext)
{
	for (unsigned char i = 0; i < 16; ++i)
	{
		if (!strcmp(sdf_extension[i], ext))
			return i;
	}
	return 0;	// 0 means unused file
}

int pack(void)
{
	DIR *dirp = opendir(DEFAULT_DIRECTORY);
	if (NULL == dirp)
	{
		printf("Error: data directory not found.\n");
		return EC_NODIR;
	}

	// count files
	int filenum = 0;
	struct IndexEntry *index = NULL;
	struct IndexEntry **ientry = &index;
	struct dirent *entry = NULL;
	while (entry = readdir(dirp))
	{
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
		{
			*ientry = (struct IndexEntry *)malloc(sizeof(struct IndexEntry));
			struct IndexEntry *curr = *ientry;
			curr->next = NULL;
			ientry = &curr->next;
			sprintf(curr->path, "%s/%s", DEFAULT_DIRECTORY, entry->d_name);
			sscanf(entry->d_name, "%[^.\r\n].%s", curr->name, curr->ext);
			curr->type = get_type_from_ext(curr->ext);
			if (0 == curr->type)
				continue;

			++filenum;
		}
	}
	printf("%d files found.\n", filenum);
	closedir(dirp);

	FILE *output = NULL;
	output = fopen("output.sdf", "wb");

	// file header
	fwrite("----------------", 1, 16, output);
	fwrite("SOULFU DATA FILE", 1, 16, output);
	int written = fprintf(output, "%d files", filenum);
	fwrite("                ", 1, 16 - written, output);
	fwrite("----------------", 1, 16, output);

	// index
	unsigned int pos = 0;
	ientry = &index;
	while (*ientry)
	{
		if (0 == (*ientry)->type)
			continue;

		FILE *input = fopen((*ientry)->path, "rb");
		fseek(input, 0, SEEK_END);
		(*ientry)->size = ftell(input);
		fseek(input, 0, SEEK_SET);
		fclose(input);

		unsigned char buff[16];
		write_big_endian(buff, pos);
		(*ientry)->pos = pos;
		pos += (*ientry)->size;
		write_big_endian(buff + 4, (*ientry)->size);
		buff[4] = (*ientry)->type;		// overwrite
		strncpy(buff + 8, (*ientry)->name, 8);

		buff[0] += OBFUSCATEA;
		buff[1] += OBFUSCATEA;
		buff[2] += OBFUSCATEA;
		buff[3] += OBFUSCATEA;
		buff[4] += OBFUSCATEB;
		buff[5] += OBFUSCATEB;
		buff[6] += OBFUSCATEB;
		buff[7] += OBFUSCATEB;
		buff[8] += OBFUSCATEC;
		buff[9] += OBFUSCATEC;
		buff[10] += OBFUSCATEC;
		buff[11] += OBFUSCATEC;
		buff[12] += OBFUSCATEC;
		buff[13] += OBFUSCATEC;
		buff[14] += OBFUSCATEC;
		buff[15] += OBFUSCATEC;

		fwrite(buff, 1, 16, output);

		ientry = &((*ientry)->next);
	}

	// trailing ----
	fwrite("----------------", 1, 16, output);

	// save filedata
	ientry = &index;
	while (*ientry)
	{
		if (0 == (*ientry)->type)
			continue;

		FILE *input = fopen((*ientry)->path, "rb");
		unsigned char data;
		for (int i = 0; i < (*ientry)->size; ++i)
		{
			fread(&data, 1, 1, input);
			data += OBFUSCATED;
			fwrite(&data, 1, 1, output);
		}
		fclose(input);

		ientry = &((*ientry)->next);
	}

	fclose(output);
}

int unpack(void)
{
	FILE *input = NULL;
	input = fopen(DEFAULT_SDF_FILENAME, "rb");

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
	ret = mkdir(DEFAULT_DIRECTORY);
#else
	ret = mkdir(DEFAULT_DIRECTORY, 0644);
#endif
	if (0 != ret)
	{
		printf("Error: output folder exists.\n");
		return EC_DIREXISTS;
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
		sprintf(filepath, "%s/%s", DEFAULT_DIRECTORY, filename);
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
