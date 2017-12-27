#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <proto/exec.h>
#include "startup.h"
#include <proto/amosextension.h>

#define debug 0

extern struct Library 		*AslBase ;
extern struct AslIFace 		*IAsl ;

enum 
{
	e_amos_example,
	e_c_example,
	e_c_header,
	e_os4_xml_interface,
	e_debug
};

const char *filetypes[]={".lib",NULL};

bool is_correct_file(char *name)
{
	const char **type;
	int l = strlen(name);

	for(type=filetypes;*type;type++)
	{
		if (l>strlen(*type))
		{
			if (strcasecmp(name+l-strlen(*type),*type)==0) return true;
		}
	}
	return false;
}

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

void StripSpaces( char *str)
{
	char *src;
	char *out = str;

	for (src=str;*src;src++)
	{
		if (*src != ' ') *out++=*src;
	}
	*out = 0;
}

void DollarToStr(char *str)
{
	int n = strlen(str);

	if (n>0)
	{
		if (str[n-1] == '$') sprintf(str+n-1,"STR");
	}
}

void make_amos_example(struct TokenInfo &cmd, char *output)
{
	char *ptr;
	const char *ret;
	const char *aptr;
	bool rv;
	char tname[100];

//	- First character:
//		The first character defines the TYPE on instruction:
//			I--> instruction
//			0--> function that returns a integer
//			1--> function that returns a float
//			2--> function that returns a string
//			V--> reserved variable. In that case, you must
//				state the type int-float-string
//	- If your instruction does not need parameters, then you stop
//	- Your instruction needs parameters, now comes the param list
//			Type,TypetType,Type...
//		Type of the parameter (0 1 2)
//		Comma or "t" for TO

			ret = "";
			aptr = cmd.args ? cmd.args : "";
			switch (*aptr)
			{
				case 'I': ret = ""; break;
				case '0': ret = "n="; break;
				case '1': ret = "f#="; break;
				case '2': ret = "s$="; break;
			}

			rv = ret[0] != 0;
			if (*aptr) aptr++;

			if (*aptr == 0) rv = false;
			
			if (cmd.command)
			{
				sprintf(tname, "%s", cmd.command[0]=='!' ? cmd.command+1 : cmd.command);
			}
			else
			{
				sprintf(tname,"");	// nothing sorry
			}

			Capitalize(tname);

			sprintf(output," %s%s%s", 
					ret, 
					tname,
					 rv ? "(" : " ");

			ptr = output + strlen(output);
			aptr = cmd.args ? cmd.args : "";

			if (*aptr) aptr++;

			while (*aptr)
			{
				ret = "";
				switch (*aptr)
				{
					case 'I': ret = ""; break;
					case '0': ret = "n"; break;
					case '1': ret = "f#"; break;
					case '2': ret = "s$"; break;
					case ',': ret= ","; break;
					case 't': ret=" To "; break;
				}

				sprintf(ptr,"%s",ret);				// add string at ptr
				ptr = output + strlen(output);		// next place to add string
				aptr++;
			}

			sprintf(ptr,"%s", rv ? ")" : " ");
}


void make_c_header(struct TokenInfo &cmd, char *output)
{
	char *ptr;
	char tname[100];

	char *_output;

	sprintf(tname, "%s", cmd.command[0]=='!' ? cmd.command+1 : cmd.command);

	Capitalize(tname);
	StripSpaces(tname);
	DollarToStr(tname);

	_output = output;
	sprintf(_output,"#define Token%s%s%s 0x%04X\n",tname, (cmd.args ? "_" : ""), (cmd.args? cmd.args : ""), cmd.token );
	_output += strlen(_output);
	sprintf(_output,"#define Inst%s%s%s %d",tname, (cmd.args ? "_" : ""), (cmd.args? cmd.args : ""), cmd.NumberOfInstruction );

	for (ptr = output; *ptr; ptr++)
	{
		if (*ptr=='.') *ptr='_';
		if (*ptr==',') *ptr='_';
		if (*ptr=='|') *ptr='_';
	}
}

void make_c_example(struct TokenInfo &cmd, char *output)
{
	char *ptr;
	const char *ret;
	const char *aptr;
	bool rv;
	char tname[100];

//	- First character:
//		The first character defines the TYPE on instruction:
//			I--> instruction
//			0--> function that returns a integer
//			1--> function that returns a float
//			2--> function that returns a string
//			V--> reserved variable. In that case, you must
//				state the type int-float-string
//	- If your instruction does not need parameters, then you stop
//	- Your instruction needs parameters, now comes the param list
//			Type,TypetType,Type...
//		Type of the parameter (0 1 2)
//		Comma or "t" for TO

			ret = "";
			aptr = cmd.args ? cmd.args : "";
			switch (*aptr)
			{
				case 'I': ret = "void "; break;
				case '0': ret = "int "; break;
				case '1': ret = "float "; break;
				case '2': ret = "std::string "; break;
			}
			rv = ret[0] != 0;
			if (*aptr) aptr++;
			if (*aptr == 0) rv = false;

			sprintf(tname, "%s", cmd.command[0]=='!' ? cmd.command+1 : cmd.command);

			Capitalize(tname);
			StripSpaces(tname);

			sprintf(output," %s%s%s", 
					ret, 
					tname,
					 rv ? "(" : "(");

			ptr = output + strlen(output);
			aptr = cmd.args ? cmd.args : "";
			if (*aptr) aptr++;

			while (*aptr)
			{
				ret = "";
				switch (*aptr)
				{
					case 'I': ret = ""; break;
					case '0': ret = "int"; break;
					case '1': ret = "float"; break;
					case '2': ret = "std::string"; break;
					case ',': ret= ","; break;
					case 't': ret=","; break;
				}
				sprintf(ptr,"%s",ret);
				ptr = output + strlen(output);
				aptr++;
			}

			sprintf(ptr,"%s", rv ? ")" : ")");
}


extern bool init();
extern void closedown();

void print_help()
{
	printf("\nAmos Extension to AscII\n");
	printf("\n");
	printf("\t--amos\t\tAmos example\n");
	printf("\t--c++\t\tc++ example\n");
	printf("\t--c-header\te_c_header\n");
	printf("\t--c-list\tc list format\n");
	printf("\t--interface\tAmigaOS4 interface (XML file)\n");
	printf("\n");
}

int main( int args, char **arg )
{
	FILE *fd;
	char *filename;
	struct extension *ext;
	struct ExtensionDescriptor *ed;
	int output_type = e_amos_example;
	char formated_text[1000];

	if (args>1)
	{
		int n;

		for (n=1; n<args; n++)
		{
			if (strcasecmp(arg[n],"--amos")==0) { output_type = e_amos_example; break; }
			if (strcasecmp(arg[n],"--c++")==0) { output_type = e_c_example; break; }
			if (strcasecmp(arg[n],"--c-header")==0) { output_type = e_c_header; break; }
			if (strcasecmp(arg[n],"--help")==0) { print_help(); return 0; }
		}
	}

	if (init())
	{
		filename = get_filename(args,arg);

		if (!filename) filename = asl();

		if (filename)
		{
			if (is_correct_file(filename))
			{
				ext = OpenExtension( filename );

				if (ext)
				{

					switch (output_type)
					{
						case e_c_header:
							printf("// token list, + offset to 680x0 assembler\n\n");
					}
					

					for ( ed = FirstExtensionItem( ext ); ed ; ed = NextExtensionItem( ed ))
					{
#if (debug)
						printf("NumberOfInstruction %d , NumberOfFunction %d\n",	ed -> tokenInfo.NumberOfInstruction,
										ed -> tokenInfo.NumberOfFunction	);
#endif
						if ((ed -> tokenInfo.args != NULL ) || (ed -> tokenInfo.command !=NULL ))
						{

#if (debug)
						printf("output_type %d\n", output_type);
#endif

							switch (output_type)
							{
								case e_amos_example:

#if (debug)
						printf("ed -> tokenInfo %08x\n", ed -> tokenInfo);
#endif

									make_amos_example(ed -> tokenInfo, formated_text);
									printf("%s\n", formated_text);
									break;

								case e_c_example:
									make_c_example(ed -> tokenInfo, formated_text);
									printf("%s\n", formated_text);
									break;

								case e_c_header:
									make_c_header(ed -> tokenInfo, formated_text);
									printf("%s\n",formated_text);
									break;

								case e_os4_xml_interface:
									break;

								case e_debug:
									break;
							}
						}
						else
						{
						}						

#if (debug)
						printf("%s: %s\n", 
							(ed -> tokenInfo.command) ? ed -> tokenInfo.command : "NULL",
							(ed -> tokenInfo.args) ? ed -> tokenInfo.args : "NULL" ) ;
#endif

					}
					CloseExtension( ext );
				}
			}
			else printf("not correct file\n");

			free(filename);
		}

		closedown();
	}

	return 0;
}

