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
	e_c_list,
	e_c_list_kitty,
	e_os4_xml_interface,
	e_debug
};

const char *filetypes[]={".lib",NULL};

char *funcPrefix = NULL;

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
	char sym;

	for (src=str;*src;src++)
	{
		sym=*src;
		if (sym == '|') sym = '_';
		if (sym != ' ') *out++=sym;
	}
	*out = 0;
}

void DollarToStr(char *str)
{
	int n = strlen(str);

	if (n>0)
	{
		if (str[n-1] == '$') sprintf(str+n-1,"STR");
		if (str[n-1] == '#') sprintf(str+n-1,"File");
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

void make_c_list(struct TokenInfo &cmd, char *output)
{
	char *ptr;
	char tname[100];

	char *_output;

	sprintf(tname, "%s", cmd.command[0]=='!' ? cmd.command+1 : cmd.command);

	Capitalize(tname);

	_output = output;


	sprintf(_output,"\t\t{0x%04X,\"%s\",\"%s\",%d,%d},",
				cmd.token,
				tname,
				(cmd.args? cmd.args : ""),
				cmd.NumberOfInstruction, 
				cmd.NumberOfFunction);

}

void make_c_list_kitty(struct TokenInfo &cmd, char *output)
{
	char *ptr;
	char tname[100];
	char *funcName;

	char *_output;

	sprintf(tname, "%s", cmd.command[0]=='!' ? cmd.command+1 : cmd.command);

	Capitalize(tname);

	funcName = strdup(tname);
	DollarToStr(funcName);
	StripSpaces(funcName);

	_output = output;


	sprintf(_output,"\t\t{0x%04X,\"%s\",%d,%s%s},",
				cmd.token,
				tname,
				0,
				 funcPrefix,
				 funcName ? funcName : "<funcName is NULL>");


	if (funcName)
	{
		free(funcName);
		funcName;
	}
}



void make_xml_interface(struct TokenInfo &cmd,  char *output)
{
	char *ptr;
	char tname[100];
	const char *ret = "";
	const char *type = "";
	const char *args = "";
	const char *c;
	bool arg_list = false;

	char *_output;

	sprintf(tname, "%s", cmd.command[0]=='!' ? cmd.command+1 : cmd.command);

	Capitalize(tname);
	StripSpaces(tname);
	DollarToStr(tname);

	for (ptr = tname; *ptr; ptr++)
	{
		if (*ptr==':') *ptr='_';
		if (*ptr==',') *ptr='_';
		if (*ptr=='.') *ptr='_';
		if (*ptr=='|') *ptr='_';
	}

	if (strlen(tname)==0)	// exit if no name
	{
		*output =0;
		return;
	}

	args = (cmd.args? cmd.args : "");
	c = args;

	switch (*c)
	{
		case 'I': ret = "void"; break;
		case '0': ret = "int"; break;
		case '1': ret = "float"; break;
		case '2': ret = "std::string"; break;
	}
	if (*c) c++;

	if (*c) arg_list = true;

	_output = output;
	sprintf(_output,"\t\t<method name=\"%s\" result=\"%s\"%s>\n",tname, ret, (arg_list ? "" : "/") );
	_output += strlen(_output);

	for ( ; *c ; c++)
	{
		switch (*c)
		{
			case 'I': ret = "void"; type = "void"; break;
			case '0': ret = "int"; type = "num"; break;
			case '1': ret = "float"; type = "num"; break;
			case '2': ret = "std::string"; type="str"; break;
			default: ret = ""; type = ""; break;			// filters out "," and "t" symbols
		}

		if (*ret)	// not empty str
		{
			sprintf(_output,"\t\t\t<arg name=\"%s%d\" result=\"%s\"/>\n",type,(c-args), ret );
			_output += strlen(_output);
		}
	}

	if (arg_list) sprintf(_output,"\t\t</method>\n" );
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
			DollarToStr(tname);

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
	printf("Copyright LiveForIt Software / Kjetil Hvalstrand\n");
	printf("Software Licence MIT\n\n");
	printf("This command designed to extract information and generate useful files that can be use\n");
	printf("different ways in C/C++ projects, general idea is this files can be used in interpreter\nconvention programs, or as docs if have lost the docs\n");
	printf("\n");
	printf("--amos\n\n");
	printf("\tShow the command in Amos fomrat\n\n");
	printf("--c++\n\n");
	printf("\tShow the command in c++ fomrat\n\n");
	printf("--c-header\n\n");
	printf("\tShow list #defines\n\n");
	printf("--c-list\n\n");
	printf("\tshow list in struct data format.\n\n");
	printf("--interface\n\n");
	printf("\tAmigaOS4 Interface XML file, that can be used to generate AmigaOS library skeleton library from, using idltool.\n");
	printf("\n");
	printf("Orginal source code can be found here:\n");
	printf("https://github.com/khval/amosExtension2Ascii\n");
	printf("\n");
}

void print_xml_interface_header(const char *name)
{
	printf("<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
	printf("<!DOCTYPE library SYSTEM \"library.dtd\">\n");
	printf("<!-- autogenerated by AmosExtension2AscII -->\n");
	printf("<library name=\"%s\" basename=\"%sBase\" openname=\"%s.library\">\n", name, name, name);

	printf("\t<include>exec/types.h</include>\n");
	printf("\t<include>exec/ports.h</include>\n");
	printf("\t<include>dos/dos.h</include>\n");
	printf("\t<include>libraries/%s.h</include>\n", name);
	printf("\t<include>utility/tagitem.h</include>\n");
	printf("\t<interface name=\"main\" version=\"1.0\" struct=\"%sIFace\" prefix=\"_%s_\" asmprefix=\"I%s\" global=\"I%s\">\n", name, name, name, name);
}

void print_xml_interface_foot()
{
	printf("\t</interface>\n");
	printf("</library>\n");
}

void print_c_list_header(const char *name)
{
	printf("struct Data %s[]={\n", name);
}


const char *prefixList[] = 
{
	"_",
	"AMOSPro",
	".lib",
	NULL
};

void remove_words(char *name,const char **list)
{
	const char **i;
	char *src;
	char *dest = name;
	bool found;
	int sym;

	for (src = name;*src;src++)
	{
		found = false;

		for (i=list;*i;i++)
		{
			if (strncasecmp( src, *i , strlen(*i) ) == 0 )
			{
				src += strlen(*i);
				src--;
				found = true;
				continue;
			}
		}

		if (found) continue;

		sym = *src;

		if ((sym>='A')&&(sym<='Z')) sym=sym-'A'+'a';

		*dest = sym;
		dest++;
	}
	*dest = 0;

	if (strcmp(name,"") == 0)
	{
		sprintf(name,"cmd");
	}
}

void print_c_list_kitty_header(const char *name)
{
	printf("struct cmdData %s[]={\n", name);

	funcPrefix = strdup(name);
	remove_words(funcPrefix,prefixList);

}

void print_c_list_foot()
{
	printf("\t\t{0x0000,NULL,NULL,0,0}};\n");
}

void print_c_list_kitty_foot()
{
	printf("\t\t{0x0000,NULL,0,0}};\n");
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
			if (strcasecmp(arg[n],"--c-list")==0) { output_type = e_c_list; break; }
			if (strcasecmp(arg[n],"--c-kitty-list")==0) { output_type = e_c_list_kitty; break; }
			if (strcasecmp(arg[n],"--interface")==0) { output_type = e_os4_xml_interface; break; }
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
					{
						char *libname = strdup(filename);
						char *ptr = NULL;
						int l = strlen(libname);

						if (l>0)
						for( ptr = libname + l - 1; ptr>libname; ptr--)
						{
							if (*ptr=='.') *ptr='\0';			// remove .lib extension

							if ((*ptr=='/')||(*ptr==':'))		// if at dir or volume
							{
								ptr++;
								break;
							}
						}
						
						switch (output_type)
						{
							case e_c_header:
								printf("// token list, + offset to 680x0 assembler\n\n");
								break;
							case e_c_list:
								print_c_list_header(ptr);
								break;
							case e_c_list_kitty:
								print_c_list_kitty_header(ptr);
								break;
							case e_os4_xml_interface:
								print_xml_interface_header(ptr);
								break;
						}
						if (libname) free(libname);
						libname;
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

								case e_c_list:
									make_c_list(ed -> tokenInfo, formated_text);
									printf("%s\n",formated_text);
									break;

								case e_c_list_kitty:
									make_c_list_kitty(ed -> tokenInfo, formated_text);
									printf("%s\n",formated_text);
									break;

								case e_os4_xml_interface:
									make_xml_interface(ed -> tokenInfo, formated_text);
									printf("%s",formated_text);
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

					switch (output_type)
					{
						case e_c_header:
							break;

						case e_c_list:
							print_c_list_foot();
							break;

						case e_c_list_kitty:
							print_c_list_kitty_foot();
							break;

						case e_os4_xml_interface:
							print_xml_interface_foot();
							break;
					}

					CloseExtension( ext );
				}
			}
			else printf("not correct file\n");
			
			if (funcPrefix) free(funcPrefix);

			free(filename);
		}


		closedown();
	}

	return 0;
}

