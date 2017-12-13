#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <proto/exec.h>
#include "startup.h"

struct Library 			 *AslBase = NULL;
struct AslIFace 		 *IAsl = NULL;


struct fileHeader
{
	unsigned int	C_off_size;
	unsigned int	C_tk_size;
	unsigned int	C_lib_size;
	unsigned int	C_title_size;
	unsigned short end;
} __attribute__((packed)) ;


 short NumberOfInstruction,NumberOfFunction;


struct command 
{
	short token;
	char command[256];
	char arg[256];
};

void read_name( FILE *fd )
{
	unsigned char byte;
	int n = 0;

	do 
	{
		fread( &byte, 1, 1, fd );
		if (byte != 0xFF) printf("%c",byte & 0x7F);
		n++;
	} while (byte != 0xFF) ;

	printf("- %d\n",n & 1);

	fread( &byte,1,1,fd); printf("%x\n", byte ); 
	fread( &byte,1,1,fd); printf("%x\n", byte); 
}

void Capitalize(char *str)
{
	int n;
	char c,lc;
	int upper = 'A'-'a';

	for (n=0;str[n];n++)
	{
		c = str[n];

		if (n==0)
		{
			str[n] = ((c>='a')  && (c <= 'z')) ? c+upper : c;
		}
		else if (n>0)
		{
			lc = str[n-1];

			if ((lc==' ')||(lc=='!'))
			{
				str[n] = ((c>='a')  && (c <= 'z')) ? c+upper : c;
			}
		}
	}
}

int load_amos_lib( char *name)
{
	FILE *fd;
	unsigned char byte;
	unsigned int offset;
	unsigned short xx;
	struct fileHeader FH;
	struct command cmd;

	char buffer[100];
	int n= 0;
	signed char c;
	int commands;
	bool is_command;
	int a_count;
	int c_count;

	memset( cmd.command, 0, 256 );
	memset( cmd.arg, 0 , 256 );
	
//	fd = fopen("amospro_system:APSystem/AMOSPro_Compact.Lib","r");
//	fd = fopen("AmosPro_system:APSystem/AmosPro_Music.lib","r");
	fd = fopen(name,"r");
	if (fd)
	{
		// skip ELF hunk
		fseek(fd, 0x20, SEEK_SET );

		fread(&FH, sizeof(struct fileHeader), 1,fd);

		printf("FH.C_off_size %d, FH.C_tk_size %d, FH.C_lib_size %d, FH.C_title_size %d\n",
			FH.C_off_size,FH.C_tk_size,FH.C_lib_size,FH.C_title_size);

		fseek(fd, FH.C_off_size + sizeof(struct fileHeader) + 0x20 , SEEK_SET );

		cmd.token = ftell(fd) - FH.C_off_size - 0x20 - 0x16;

		fread(&NumberOfInstruction, sizeof(NumberOfInstruction), 1,fd);

		while ((NumberOfInstruction != 0))
		{

			fread(&NumberOfFunction, sizeof(NumberOfFunction), 1,fd);
			fread( &c,1,1,fd);

			c_count = 0;
			a_count = 0;

			is_command = TRUE;	// we expect command.
			while ((c != -1) && (c != -2))
			{
				if ( (c & 127) > 0)
				{
					if (is_command)
					{
						if (c_count<255) cmd.command[ c_count++ ] = c & 127;
					}
					else
					{
						if (a_count<255) cmd.arg[ a_count++ ] = c & 127;
					}
				}

				if (c & 128)
				{
					 is_command = FALSE;
				}

				fread( &c,1,1,fd);
			}

			// terminate string only if we have new command.
			if (c_count>0) 
			{
				int n = strlen( cmd.command );

				cmd.command[ c_count ] = 0;	// terminate string.
				Capitalize(cmd.command);

				while ((n>0) && (cmd.command[n-1]==' '))
				{
//					printf("*%s*\n", cmd.command);

					cmd.command[n-1]=0;
					n--;
				}

			}

			cmd.arg[ a_count  ] = 0;

			printf("\t{ 0x%04X,%c%s%c },\n",cmd.token, 34, cmd.command[0]=='!' ? cmd.command+1 : cmd.command, 34);

			if (ftell(fd)&1)  	fread( &c,1,1,fd);
			cmd.token = ftell(fd) - FH.C_off_size - 0x20 - 0x16;

			fread(&NumberOfInstruction, sizeof(NumberOfInstruction), 1,fd);
		}

		printf("\nExit at %08X\n",ftell(fd), NumberOfInstruction);

		fclose(fd);
	}
}

bool init()
{
	if ( ! open_lib( "asl.library", 0L , "main", 1, &AslBase, (struct Interface **) &IAsl  ) ) return FALSE;
	return TRUE;
}

void closedown()
{
	if (IAsl) DropInterface((struct Interface*) IAsl); IAsl = 0;
	if (AslBase) CloseLibrary(AslBase); AslBase = 0;
}

int main( int args, char **arg )
{
	FILE *fd;
	char amosid[17];
	unsigned int tokenlength;
	char *filename;

	amosid[16] = 0;	// /0 string.

	if (init())
	{

		filename = get_filename(args,arg);

		if (!filename) filename = asl();

		if (filename)
		{
			load_amos_lib( filename );
			free(filename);
		}
		closedown();
	}

	return 0;
}

