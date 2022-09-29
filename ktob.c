

#include <stdio.h>
#include <unistd.h> // for getopt()
#include <stdlib.h> // for exit()
#include "edt.h"

/******************************************************************************


NAME:   ktoa.c

AUTHOR: Edward Thompson

DATE:   Wed Mar 30 17:00:06 CST 1988

ABSTRACT:	Convert Kermit Files to Binary

DESCRIPTION:    This program takes a file that has been converted to the
		printable character range (' ' -- '~') via btok, using
		the prefix and quoting rules from the kermit protocol
		manual and translates the file back into it's binary
		form.

RESTRICTIONS:   

WARNINGS:	This usually finds a way to interpret the character
		sequence. If the file is not produced by btok, care
		must be taken to make sure that the desired results
		are acheived.

		This uses stdio which is slow, but has no restrictions
		on the length of files or lines.

MODIFICATION HISTORY:

03/30/88	Began this module.
                                                                         (edt)
******************************************************************************/


/*---------------------------------------------------------------------------*/
/*                              Module Globals                               */
/*---------------------------------------------------------------------------*/

#define VERSION "ktob (Kermit to Binary) Version 0.00a\n"

int	pre_ch;		/* the character to use for prefixing */
int	ctl_ch;		/* the character to use for quoting   */
int	rep_ch;		/* the currently unsupported repeat character */
int	crlf_flg;	/* whether or not to translate <CR> and <LF> */

#define tochar(x) ((x)+32)
#define unchar(x) ((x)-32)
#define ctl(x)    ((x)^64)

/*****************************************************************************
 *
 *  NAME
 *	unprefix - unprefix a character
 *
 *  SYNOPSIS
 *	int
 *	unprefix(s, pre_char, ctl_char, rep_char, crlf_flag)
 *	char	*s;
 *	int	pre_char;
 *	int	ctl_char;
 *	int	rep_char;
 *	int	crlf_flag;
 *
 *  DESCRIPTION
 *	This function reads a set of input characters and translates the
 *	first pattern into a byte using the Kermit 8-bit and control
 *	character prefixing algorithim. This is the converse to the 
 *	function prefix().
 *
 *  PASSED PARAMETERS
 *	s - a pointer to the characters to translate
 *	pre_char  - the character to use to prefix
 *	ctl_char  - the character to use to show a control character
 *	rep_char  - currently unsupport repeat translation
 *
 *  FUNCTIONS CALLED
 *	ctl()
 *
 *  CALLING FUNCTIONS
 *
 *  RETURN VALUE
 *	The byte value of the first translated character that was read
 *
 *  SEE ALSO
 *	The Kermit Protocol Manual, prefix()
 *
 *  WARNINGS
 *	This only does the first character. It is up to the calling
 *	routine to correctly update the pointer to the begining of the 
 *	next character.
 *
 *	If the stuff coming in is not formatted by prefix(), the output
 *	will most certainly be meaningless.
 *
 *	I know this code is not tight, but it makes sense to me the way
 *	it is written, so dont bug me about it.
 *                                                                       (edt)
 *****************************************************************************/

extern int unprefix(char *s, int pre_char, int ctl_char, int rep_char);
int unprefix(char *s, int pre_char, int ctl_char, int rep_char)
{
	if ((int) *s == ctl_char)			/* # */
	{
		if ((int) *(s+1) == ctl_char)
			return(ctl_char);		/* ## -> # */
		else if ((int) *(s+1) == pre_char)
			return(pre_char);		/* #& -> & */
		else if ((int) *(s+1) == rep_char)
			return(rep_char);		/* #~ -> ~ */
		else
			return(ctl((int) *(s+1)));	/* #A -> ^A */
	}
	else if ((int) *s == pre_char)			/* & */
	{
		if ((int) *(s+1) == ctl_char)		/* &# */
		{
			if ((int) *(s+2) == ctl_char)
				return(ctl_char+0x80);	/* &## -> #+0x80 */
			else if ((int) *(s+2) == pre_char)
				return(pre_char+0x80);	/* &#& -> &+0x80 */
			else if ((int) *(s+2) == rep_char)
				return(rep_char+0x80);	/* &#~ -> ~+0x80 */
			else				/* &#A -> ^A+0x80 */
				return(ctl((int) *(s+2))+0x80);
		}
		else					/* &A -> A+0x80 */
			return(((int) *(s+1))+0x80);
	}
	else						/* A -> A */
		return((int) *s);
}

/*****************************************************************************
 *
 *  NAME
 *	do_unprefix - unprefix the file
 *
 *  SYNOPSIS
 *	void
 *	do_unprefix(fp)
 *	FILE	*fp;
 *
 *  DESCRIPTION
 *	This function takes an incoming character stream and translates
 *	the characters from a "Kermit" format back to the binary file
 *	that it originated from.
 *
 *  PASSED PARAMETERS
 *	fp - the stream to translate
 *
 *  FUNCTIONS CALLED
 *	gets(BA_LIB), fputs(BA_LIB), unprefix()
 *
 *  CALLING FUNCTIONS
 *	main()
 *
 *  RETURN VALUE
 *	NONE
 *
 *  SEE ALSO
 *	Kermit Protocol Manual, unprefix()
 *
 *  WARNINGS
 *
 *                                                                       (edt)
 *****************************************************************************/

extern void do_unprefix(FILE *fp);
void do_unprefix(FILE *fp)
{
char	buf[10];
int	c;
int	i;

	while ((c = getc(fp)) != EOF)
	{
		i = 0;
		buf[i++] = (char) c;
		if (c == ctl_ch)	/* then we have to get one more */
			buf[i++] = (char) getc(fp);
		else if (c == pre_ch)	/* we have to get at least one more */
		{
			c = getc(fp);
			buf[i++] = (char) c;
			if (c == ctl_ch) /* then we have to get one more */
				buf[i++] = (char) getc(fp);
		}
		putc(unprefix(buf, pre_ch, ctl_ch, rep_ch), stdout);
	}
}

/******************************************************************************

  NAME
	ktob - Kermit to Binary

  SYNOPSIS
	ktob [ -? ] [ -v ] [ -l ] [ -p n ] [ -c n ] [ -r n ] [ files...  ]

  DESCRIPTION
 	This function takes a files and writes to standard out
	the binary files which was prefixed and quoted
	according to the Kermit Protocol Manual. This will decode
	a binary file that was encoded with btok. The advantage of
	btok & ktob over atob & btoa is that the result is readable
	where there was ascii text.
	 
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
	btok(edt), atob(LOCAL), uuencode(1)
                                                                         (edt)
******************************************************************************/

extern void usage(void);
void usage(void)
{
printf("\n");
printf("  NAME\n");
printf("\011ktob - Kermit to Binary\n");
printf("\n");
printf("  SYNOPSIS\n");
printf("\011ktob [ -? ] [ -v ] [ -l ] [ -p n ] [ -c n ] [ -r n ] [ files...  ]\n");
printf("\n");
printf("  DESCRIPTION\n");
printf(" \011This function takes a files and writes to standard out\n");
printf("\011the binary files which was prefixed and quoted\n");
printf("\011according to the Kermit Protocol Manual. This will decode\n");
printf("\011a binary file that was encoded with btok. The advantage of\n");
printf("\011btok & ktob over atob & btoa is that the result is readable\n");
printf("\011where there was ascii text.\n");
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
printf("\011btok(edt), atob(LOCAL), uuencode(1)\n");
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
				do_unprefix(fp);
				fclose(fp);
			}
		}
	}
	else
	{
		do_unprefix(stdin);
	}
}

