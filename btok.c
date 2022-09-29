
#include <stdio.h>
#include <unistd.h> // for getopt()
#include <stdlib.h> // for exit()
#include "edt.h"

/******************************************************************************


NAME:   btok.c

AUTHOR: Edward Thompson

DATE:   Wed Mar 30 16:20:30 CST 1988

ABSTRACT:	Binary to Kermit

DESCRIPTION:    Translates a file to a form that it is prefixed and 8
		bit quoted as a kermit packet would be.

RESTRICTIONS:   Care and understanding are needed when editing the
		output of this program.

WARNINGS: 	This uses stdio which is slow, but has no restrictions
		on the length of files or lines.

MODIFICATION HISTORY:

03/30/88	Began this module.
                                                                         (edt)
******************************************************************************/


/*---------------------------------------------------------------------------*/
/*                              Module Globals                               */
/*---------------------------------------------------------------------------*/

#define VERSION "btok (Binary to Kermit) Version 0.00a\n"

#define tochar(x) ((x)+32)
#define unchar(x) ((x)-32)
#define ctl(x)    ((x)^64)

int	pre_ch;		/* the character to use for prefixing */
int	ctl_ch;		/* the character to use for quoting   */
int	rep_ch;		/* the currently unsupported repeat character */
int	crlf_flg;	/* whether or not to translate <CR> and <LF> */


/*****************************************************************************
 *
 *  NAME
 *	prefix - prefix a character acoording to kermit standards
 *
 *  SYNOPSIS
 *	char *
 *	prefix(c, pre_char, ctl_char, crlf_flag)
 *	int	c;
 *	int	pre_char;
 *	int	ctl_char;
 *	int	rep_char;
 *	int	crlf_flag;
 *
 *  DESCRIPTION
 *	This function translates a character to a readable format even
 *	though it may be outside of the displayable range. This function
 *	uses the translation described in section 8 of the Kermit
 *	protocol manual.
 *
 *  PASSED PARAMETERS
 *	c - the character to translate
 *	pre_char  - the character to use to prefix
 *	ctl_char  - the character to use to show a control character
 *	rep_char  - currently unsupport repeat translation
 *	crlf_flag - flag to indicate whether to translate <CR> and <LF>
 *		    if true, translate the characters
 *
 *  FUNCTIONS CALLED
 *	NONE
 *
 *  CALLING FUNCTIONS
 *
 *  RETURN VALUE
 *	Pointer to a static string containing the readable version of
 *	the characters.
 *
 *  SEE ALSO
 *	Kermit Protocol Manual
 *
 *  WARNINGS
 *	Returns a pointer to a static area that is overwritten on every
 *	call.
 *
 *	This is a brute force method to building the parser!
 *                                                                       (edt)
 *****************************************************************************/

extern char * prefix(int c, int pre_char, int ctl_char, int rep_char, int crlf_flag);
char * prefix(int c, int pre_char, int ctl_char, int rep_char, int crlf_flag)
{
static	char	buf[10];
int	i;

	i = 0;
	if (c == ctl_char)			/* # -> ## */
	{
		buf[i++] = (char) ctl_char;
		buf[i++] = (char) ctl_char;
	}
	else if (c == pre_char)			/* & -> #& */
	{
		buf[i++] = (char) ctl_char;
		buf[i++] = (char) pre_char;
	}
	else if (c == rep_char)			/* ~ -> #~ */
	{
		buf[i++] = (char) ctl_char;
		buf[i++] = (char) rep_char;
	}
	else if ((c >= ' ') && (c <= '~'))	/* A -> A */
	{
		buf[i++] = (char) c;
	}
	else if ((c < ' ') || (c == 0x7f))			/* ^A -> &A */
	{
		if (((c == '\n') || (c == '\r')) && (!crlf_flag))
			buf[i++] = (char) c;
		else
		{
			buf[i++] = (char) ctl_char;
			buf[i++] = (char) ctl(c);
		}
	}
	else if ((c & 0x7f) == ctl_char)	/*  #+0x80 -> &## */
	{
		buf[i++] = (char) pre_char;
		buf[i++] = (char) ctl_char;
		buf[i++] = (char) ctl_char;
	}
	else if ((c & 0x7f) == pre_char)	/* &+0x80 -> &#& */
	{
		buf[i++] = (char) pre_char;
		buf[i++] = (char) ctl_char;
		buf[i++] = (char) pre_char;
	}
	else if ((c & 0x7f) == rep_char)	/* ~+0x80 -> &#~ */
	{
		buf[i++] = (char) pre_char;
		buf[i++] = (char) ctl_char;
		buf[i++] = (char) rep_char;
	}
	else if (((c & 0x7f) >= ' ') && ((c & 0x7f) <= '~'))
	{
		buf[i++] = (char) pre_char;
		buf[i++] = (char) (c & 0x7f);
	}
	else if (((c & 0x7f) < ' ') || ((c & 0x7f) == 0x7f)) /* ^A+0x80->&#A */
	{
		buf[i++] = (char) pre_char;
		buf[i++] = (char) ctl_char;
		buf[i++] = (char) ctl(c & 0x7f);
	}
	buf[i++] = '\0';
	return(buf);
}
/*****************************************************************************
 *
 *  NAME
 *	do_prefix - do the prefix translation
 *
 *  SYNOPSIS
 *	void
 *	do_prefix(fp)
 *	FILE	*fp;
 *
 *  DESCRIPTION
 *	This function takes a stream pointer and writes to standard out
 *	a version of the input that has been prefixed and quoted
 *	according to the Kermit Protocol Manual. This will encode
 *	a binary file into 7 bits without control characters and also
 *	make it somewhat readable if the file contains readable text.
 *
 *  PASSED PARAMETERS
 *	fp - the stream to convert
 *
 *  FUNCTIONS CALLED
 *	prefix(), getc(BA_LIB), putc(BA_LIB)
 *
 *  CALLING FUNCTIONS
 *	main()
 *
 *  RETURN VALUE
 *	NONE
 *
 *  SEE ALSO
 *	Kermit Protocol Manual, prefix()
 *
 *  WARNINGS
 *                                                                       (edt)
 *****************************************************************************/

extern void do_prefix(FILE *fp);
void do_prefix(FILE *fp)
{
int	c;

	while((c = getc(fp)) != EOF)
	{
		fputs(prefix(c, pre_ch, ctl_ch, rep_ch, crlf_flg), stdout);
	}
}

/******************************************************************************

  NAME
	btok - Binary to Kermit

  SYNOPSIS
	btok [ -? ] [ -v ] [ -l ] [ -p n ] [ -c n ] [ -r n ] [ files...  ]

  DESCRIPTION
 	This function takes a files and writes to standard out
	a version of the input that has been prefixed and quoted
	according to the Kermit Protocol Manual. This will encode
	a binary file into 7 bits without control characters and also
	make it somewhat readable if the file contains readable text.
	 
	-?	Will give the manual page for function.

	-v	Will give the version number.

	-l	Indicates to translate and prefix any carraige returns
		and line feeds.

	-p n	Set the prefix character to n. The default is '&'.

	-c n	Set the control character to n. The default is '#'.

	-r n	Set the repeat character to n. The default is '~'.

  AUTHOR
	Ed Thompson

  DATE
	Wed Mar 30 16:37:50 CST 1988

  SEE ALSO
	atob(LOCAL), uuencode(1)
                                                                         (edt)
******************************************************************************/


extern void usage(void);
void usage(void)
{
printf("  NAME\n");
printf("\011btok - Binary to Kermit\n");
printf("\n");
printf("  SYNOPSIS\n");
printf("\011btok [ -? ] [ -v ] [ -l ] [ -p n ] [ -c n ] [ -r n ] [ files...  ]\n");
printf("\n");
printf("  DESCRIPTION\n");
printf(" \011This function takes a files and writes to standard out\n");
printf("\011a version of the input that has been prefixed and quoted\n");
printf("\011according to the Kermit Protocol Manual. This will encode\n");
printf("\011a binary file into 7 bits without control characters and also\n");
printf("\011make it somewhat readable if the file contains readable text.\n");
printf("\011 \n");
printf("\011-?\011Will give the manual page for function.\n");
printf("\n");
printf("\011-v\011Will give the version number.\n");
printf("\n");
printf("\011-l\011Indicates to translate and prefix any carraige returns\n");
printf("\011\011and line feeds.\n");
printf("\n");
printf("\011-p n\011Set the prefix character to n. The default is '&'.\n");
printf("\n");
printf("\011-c n\011Set the control character to n. The default is '#'.\n");
printf("\n");
printf("\011-r n\011Set the repeat character to n. The default is '~'.\n");
printf("\n");
printf("  AUTHOR\n");
printf("\011Ed Thompson\n");
printf("\n");
printf("  DATE\n");
printf("\011Wed Mar 30 16:37:50 CST 1988\n");
printf("\n");
printf("  SEE ALSO\n");
printf("\011atob(LOCAL), uuencode(1)\n");
printf("\n");
}

int main(int argc, char **argv)
{
int	c;
extern	char	*optarg;
extern	int	optind;
FILE	*fp;

	pre_ch = '&';
	ctl_ch = '#';
	rep_ch = '~';
	crlf_flg = 0;
	while((c = getopt(argc, argv, "?vlp:c:r:")) != EOF)
		switch(c) 
		{
			case 'l':
				crlf_flg = 1;
				break;
			case 'p':
				pre_ch = *optarg;
				break;
			case 'c':
				ctl_ch = *optarg;
				break;
			case 'r':
				rep_ch = *optarg;
				break;
			case '?':
				usage();
				exit(1);
				break;
			case 'v':
				printf(VERSION);
				exit(0);
				break;
			default:
				usage();
				exit(1);
				break;
		}
	if (optind < argc)
	{
		for (; optind < argc; optind++)
		{
			if ((fp = fopen(argv[optind], "r"))
				== (FILE *) NULL)
			{
				fprintf(stderr, "Could Not Open %s in %s.\n",
					argv[optind], argv[0]);
			}
			else
			{
				do_prefix(fp);
				fclose(fp);
			}
		}
	}
	else
	{
		do_prefix(stdin);
	}
}

