/** \file options_popt.h
 * \ingroup popt
 */

/* This file is a port of the popt library for use with the AJA SDK.
 * The code for the library was consolidated into a .h and .cpp file
 * to simplify building demonstration applications. Not all the features
 * of popt have been tested. Only simple command line parameter parsing
 * was needed for the SDK, but popt was ported to allow enhancing our
 * applications with additional functionality as needed.
*/

/* (C) 1998-2000 Red Hat, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from 
   ftp://ftp.rpm.org/pub/rpm/dist. */

/* Here is the contents of the COPYING file:
*/

/*
Copyright (c) 1998  Red Hat Software

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
*/

#ifndef H_POPT
#define H_POPT

#include <stdio.h>			/* for FILE * */
#include "export.h"

#define POPT_OPTION_DEPTH	10

/** \ingroup popt
 * \name Arg type identifiers
 */
#define POPT_ARG_NONE		 0U	/*!< no arg */
#define POPT_ARG_STRING		 1U	/*!< arg will be saved as string */
#define POPT_ARG_INT		 2U	/*!< arg ==> int */
#define POPT_ARG_LONG		 3U	/*!< arg ==> long */
#define POPT_ARG_INCLUDE_TABLE	 4U	/*!< arg points to table */
#define POPT_ARG_CALLBACK	 5U	/*!< table-wide callback... must be
					   set first in table; arg points 
					   to callback, descrip points to 
					   callback data to pass */
#define POPT_ARG_INTL_DOMAIN     6U	/*!< set the translation domain
					   for this table and any
					   included tables; arg points
					   to the domain string */
#define POPT_ARG_VAL		 7U	/*!< arg should take value val */
#define	POPT_ARG_FLOAT		 8U	/*!< arg ==> float */
#define	POPT_ARG_DOUBLE		 9U	/*!< arg ==> double */
#define	POPT_ARG_LONGLONG	 10U	/*!< arg ==> long long */

#define POPT_ARG_MAINCALL	16U+11U	/*!< EXPERIMENTAL: return (*arg) (argc, argv) */
#define	POPT_ARG_ARGV		12U	/*!< dupe'd arg appended to realloc'd argv array. */
#define	POPT_ARG_SHORT		13U	/*!< arg ==> short */
#define	POPT_ARG_BITSET		16U+14U	/*!< arg ==> bit set */

#define POPT_ARG_MASK		0x000000FFU
#define POPT_GROUP_MASK		0x0000FF00U

/** \ingroup popt
 * \name Arg modifiers
 */
#define POPT_ARGFLAG_ONEDASH	0x80000000U  /*!< allow -longoption */
#define POPT_ARGFLAG_DOC_HIDDEN 0x40000000U  /*!< don't show in help/usage */
#define POPT_ARGFLAG_STRIP	0x20000000U  /*!< strip this arg from argv(only applies to long args) */
#define	POPT_ARGFLAG_OPTIONAL	0x10000000U  /*!< arg may be missing */

#define	POPT_ARGFLAG_OR		0x08000000U  /*!< arg will be or'ed */
#define	POPT_ARGFLAG_NOR	0x09000000U  /*!< arg will be nor'ed */
#define	POPT_ARGFLAG_AND	0x04000000U  /*!< arg will be and'ed */
#define	POPT_ARGFLAG_NAND	0x05000000U  /*!< arg will be nand'ed */
#define	POPT_ARGFLAG_XOR	0x02000000U  /*!< arg will be xor'ed */
#define	POPT_ARGFLAG_NOT	0x01000000U  /*!< arg will be negated */
#define POPT_ARGFLAG_LOGICALOPS \
        (POPT_ARGFLAG_OR|POPT_ARGFLAG_AND|POPT_ARGFLAG_XOR)

#define	POPT_BIT_SET	(POPT_ARG_VAL|POPT_ARGFLAG_OR)
					/*!< set arg bit(s) */
#define	POPT_BIT_CLR	(POPT_ARG_VAL|POPT_ARGFLAG_NAND)
					/*!< clear arg bit(s) */

#define	POPT_ARGFLAG_SHOW_DEFAULT 0x00800000U /*!< show default value in --help */
#define	POPT_ARGFLAG_RANDOM	0x00400000U  /*!< random value in [1,arg] */
#define	POPT_ARGFLAG_TOGGLE	0x00200000U  /*!< permit --[no]opt prefix toggle */

/** \ingroup popt
 * \name Callback modifiers
 */
#define POPT_CBFLAG_PRE		0x80000000U  /*!< call the callback before parse */
#define POPT_CBFLAG_POST	0x40000000U  /*!< call the callback after parse */
#define POPT_CBFLAG_INC_DATA	0x20000000U  /*!< use data from the include line,
					       not the subtable */
#define POPT_CBFLAG_SKIPOPTION	0x10000000U  /*!< don't callback with option */
#define POPT_CBFLAG_CONTINUE	0x08000000U  /*!< continue callbacks with option */

/** \ingroup popt
 * \name Error return values
 */
#define POPT_ERROR_NOARG	-10	/*!< missing argument */
#define POPT_ERROR_BADOPT	-11	/*!< unknown option */
#define POPT_ERROR_OPTSTOODEEP	-13	/*!< aliases nested too deeply */
#define POPT_ERROR_BADQUOTE	-15	/*!< error in paramter quoting */
#define POPT_ERROR_ERRNO	-16	/*!< errno set, use strerror(errno) */
#define POPT_ERROR_BADNUMBER	-17	/*!< invalid numeric value */
#define POPT_ERROR_OVERFLOW	-18	/*!< number too large or too small */
#define	POPT_ERROR_BADOPERATION	-19	/*!< mutually exclusive logical operations requested */
#define	POPT_ERROR_NULLARG	-20	/*!< opt->arg should not be NULL */
#define	POPT_ERROR_MALLOC	-21	/*!< memory allocation failed */
#define	POPT_ERROR_BADCONFIG	-22	/*!< config file failed sanity test */

/** \ingroup popt
 * \name poptBadOption() flags
 */
#define POPT_BADOPTION_NOALIAS  (1U << 0)  /*!< don't go into an alias */

/** \ingroup popt
 * \name poptGetContext() flags
 */
#define POPT_CONTEXT_NO_EXEC	(1U << 0)  /*!< ignore exec expansions */
#define POPT_CONTEXT_KEEP_FIRST	(1U << 1)  /*!< pay attention to argv[0] */
#define POPT_CONTEXT_POSIXMEHARDER (1U << 2) /*!< options can't follow args */
#define POPT_CONTEXT_ARG_OPTS	(1U << 4) /*!< return args as options with value 0 */

/** \ingroup popt
 */
struct poptOption {
    const char * longName;	/*!< may be NULL */
    char shortName;		/*!< may be NUL */
    unsigned int argInfo;
    void * arg;			/*!< depends on argInfo */
    int val;			/*!< 0 means don't return, just update flag */
    const char * descrip;	/*!< description for autohelp -- may be NULL */
    const char * argDescrip;	/*!< argument description for autohelp */
};

/** \ingroup popt
 * A popt alias argument for poptAddAlias().
 */
struct poptAlias {
    const char * longName;	/*!< may be NULL */
    char shortName;		/*!< may be NUL */
    int argc;
    const char ** argv;		/*!< must be free()able */
};

/** \ingroup popt
 * A popt alias or exec argument for poptAddItem().
 */
typedef struct poptItem_s {
    struct poptOption option;	/*!< alias/exec name(s) and description. */
    int argc;			/*!< (alias) no. of args. */
    const char ** argv;		/*!< (alias) args, must be free()able. */
} * poptItem;

/** \ingroup popt
 * \name Auto-generated help/usage
 */

/**
 * Empty table marker to enable displaying popt alias/exec options.
 */
extern struct poptOption poptAliasOptions[];
#define POPT_AUTOALIAS { NULL, '\0', POPT_ARG_INCLUDE_TABLE, poptAliasOptions, \
			0, "Options implemented via popt alias/exec:", NULL },

/**
 * Auto help table options.
 */
extern struct poptOption poptHelpOptions[];

extern struct poptOption * poptHelpOptionsI18N;

#define POPT_AUTOHELP { NULL, '\0', POPT_ARG_INCLUDE_TABLE, poptHelpOptions, \
			0, "Help options:", NULL },

#define POPT_TABLEEND { NULL, '\0', 0, NULL, 0, NULL, NULL }

/** \ingroup popt
 */
typedef struct poptContext_s * poptContext;

/** \ingroup popt
 */
#ifndef __cplusplus
typedef struct poptOption * poptOption;
#endif

/** \ingroup popt
 */
enum poptCallbackReason {
    POPT_CALLBACK_REASON_PRE	= 0, 
    POPT_CALLBACK_REASON_POST	= 1,
    POPT_CALLBACK_REASON_OPTION = 2
};

#ifdef __cplusplus
extern "C" {
#endif

/** \ingroup popt
 * Table callback prototype.
 * @param con		context
 * @param reason	reason for callback
 * @param opt		option that triggered callback
 * @param arg		argument
 * @param data		argument data
 */
typedef void (*poptCallbackType) (poptContext con, 
		enum poptCallbackReason reason,
		const struct poptOption * opt,
		const char * arg,
		const void * data);

/** \ingroup popt
 * Destroy context.
 * @param con		context
 * @return		NULL always
 */
poptContext poptFreeContext(poptContext con);

/** \ingroup popt
 * Initialize popt context.
 * @param name		context name (usually argv[0] program name)
 * @param argc		no. of arguments
 * @param argv		argument array
 * @param options	address of popt option table
 * @param flags		or'd POPT_CONTEXT_* bits
 * @return		initialized popt context
 */
poptContext poptGetContext(
		const char * name,
		int argc, const char ** argv,
		const struct poptOption * options,
		unsigned int flags);

/** \ingroup popt
 * Destroy context (alternative implementation).
 * @param con		context
 * @return		NULL always
 */
poptContext poptFini(poptContext con);

/** \ingroup popt
 * Initialize popt context (alternative implementation).
 * This routine does poptGetContext() and then poptReadConfigFiles().
 * @param argc		no. of arguments
 * @param argv		argument array
 * @param options	address of popt option table
 * @param configPaths	colon separated file path(s) to read.
 * @return		initialized popt context (NULL on error).
 */
poptContext poptInit(int argc, const char ** argv,
		const struct poptOption * options,
		const char * configPaths);

/** \ingroup popt
 * Reinitialize popt context.
 * @param con		context
 */
void poptResetContext(poptContext con);

/** \ingroup popt
 * Return value of next option found.
 * @param con		context
 * @return		next option val, -1 on last item, POPT_ERROR_* on error
 */
int poptGetNextOpt(poptContext con);

/** \ingroup popt
 * Return next option argument (if any).
 * @param con		context
 * @return		option argument, NULL if no argument is available
 */
char * poptGetOptArg(poptContext con);

/** \ingroup popt
 * Return next argument.
 * @param con		context
 * @return		next argument, NULL if no argument is available
 */
const char * poptGetArg(/*@null@*/poptContext con);

/** \ingroup popt
 * Peek at current argument.
 * @param con		context
 * @return		current argument, NULL if no argument is available
 */
const char * poptPeekArg(poptContext con);

/** \ingroup popt
 * Return remaining arguments.
 * @param con		context
 * @return		argument array, NULL terminated
 */
const char ** poptGetArgs(poptContext con);

/** \ingroup popt
 * Return the option which caused the most recent error.
 * @param con		context
 * @param flags		option flags
 * @return		offending option
 */
const char * poptBadOption(poptContext con, unsigned int flags);

/** \ingroup popt
 * Add arguments to context.
 * @param con		context
 * @param argv		argument array, NULL terminated
 * @return		0 on success, POPT_ERROR_OPTSTOODEEP on failure
 */
int poptStuffArgs(poptContext con, const char ** argv);

/** \ingroup popt
 * Add alias to context.
 * @todo Pass alias by reference, not value.
 * @note This function is deprecated. Use poptAddItem instead.
 * @param con		context
 * @param alias		alias to add
 * @param flags		(unused)
 * @return		0 on success
 */
int poptAddAlias(poptContext con, struct poptAlias alias, int flags);

/** \ingroup popt
 * Add alias/exec item to context.
 * @param con		context
 * @param newItem	alias/exec item to add
 * @param flags		0 for alias, 1 for exec
 * @return		0 on success
 */
int poptAddItem(poptContext con, poptItem newItem, int flags);

/** \ingroup popt
 * Perform sanity checks on a file path.
 * @param fn		file name
 * @return		0 on OK, 1 on NOTOK.
 */
int poptSaneFile(const char * fn);

/**
 * Read a file into a buffer.
 * @param fn		file name
 * @retval *bp		buffer (malloc'd) (or NULL)
 * @retval *nbp		no. of bytes in buffer (including final NUL) (or NULL)
 * @param flags		1 to trim escaped newlines
 * return		0 on success
 */
int poptReadFile(const char * fn, char ** bp,
		size_t * nbp, int flags);
#define	POPT_READFILE_TRIMNEWLINES	1

/** \ingroup popt
 * Read configuration file.
 * @param con		context
 * @param fn		file name to read
 * @return		0 on success, POPT_ERROR_ERRNO on failure
 */
int poptReadConfigFile(poptContext con, const char * fn);

/** \ingroup popt
 * Read configuration file(s).
 * Colon separated files to read, looping over poptReadConfigFile().
 * Note that an '@' character preceeding a path in the list will
 * also perform additional sanity checks on the file before reading.
 * @param con		context
 * @param paths		colon separated file name(s) to read
 * @return		0 on success, POPT_ERROR_BADCONFIG on failure
 */
int poptReadConfigFiles(poptContext con, const char * paths);

/** \ingroup popt
 * Read default configuration from /etc/popt and $HOME/.popt.
 * @param con		context
 * @param useEnv	(unused)
 * @return		0 on success, POPT_ERROR_ERRNO on failure
 */
int poptReadDefaultConfig(poptContext con, int useEnv);

/** \ingroup popt
 * Duplicate an argument array.
 * @note: The argument array is malloc'd as a single area, so only argv must
 * be free'd.
 *
 * @param argc		no. of arguments
 * @param argv		argument array
 * @retval argcPtr	address of returned no. of arguments
 * @retval argvPtr	address of returned argument array
 * @return		0 on success, POPT_ERROR_NOARG on failure
 */
int poptDupArgv(int argc, const char **argv,
		int * argcPtr,
		const char *** argvPtr);

/** \ingroup popt
 * Parse a string into an argument array.
 * The parse allows ', ", and \ quoting, but ' is treated the same as " and
 * both may include \ quotes.
 * @note: The argument array is malloc'd as a single area, so only argv must
 * be free'd.
 *
 * @param s		string to parse
 * @retval argcPtr	address of returned no. of arguments
 * @retval argvPtr	address of returned argument array
 */
int poptParseArgvString(const char * s,
		int * argcPtr, const char *** argvPtr);

/** \ingroup popt
 * Parses an input configuration file and returns an string that is a 
 * command line.  For use with popt.  You must free the return value when done.
 *
 * Given the file:
\verbatim
# this line is ignored
    #   this one too
aaa
  bbb
    ccc   
bla=bla

this_is   =   fdsafdas
     bad_line=        
  reall bad line  
  reall bad line  = again
5555=   55555   
  test = with lots of spaces
\endverbatim
*
* The result is:
\verbatim
--aaa --bbb --ccc --bla="bla" --this_is="fdsafdas" --5555="55555" --test="with lots of spaces"
\endverbatim
*
* Passing this to poptParseArgvString() yields an argv of:
\verbatim
'--aaa'
'--bbb' 
'--ccc' 
'--bla=bla' 
'--this_is=fdsafdas' 
'--5555=55555' 
'--test=with lots of spaces' 
\endverbatim
 *
 * @bug NULL is returned if file line is too long.
 * @bug Silently ignores invalid lines.
 *
 * @param fp		file handle to read
 * @param argstrp	return string of options (malloc'd)
 * @param flags		unused
 * @return		0 on success
 * @see			poptParseArgvString
 */
int poptConfigFileToString(FILE *fp, char ** argstrp, int flags);

/** \ingroup popt
 * Return formatted error string for popt failure.
 * @param error		popt error
 * @return		error string
 */
const char * poptStrerror(const int error);

/** \ingroup popt
 * Limit search for executables.
 * @param con		context
 * @param path		single path to search for executables
 * @param allowAbsolute	absolute paths only?
 */
void poptSetExecPath(poptContext con, const char * path, int allowAbsolute);

/** \ingroup popt
 * Print detailed description of options.
 * @param con		context
 * @param fp		ouput file handle
 * @param flags		(unused)
 */
void poptPrintHelp(poptContext con, FILE * fp, int flags);

/** \ingroup popt
 * Print terse description of options.
 * @param con		context
 * @param fp		ouput file handle
 * @param flags		(unused)
 */
void poptPrintUsage(poptContext con, FILE * fp, int flags);

/** \ingroup popt
 * Provide text to replace default "[OPTION...]" in help/usage output.
 * @param con		context
 * @param text		replacement text
 */
void poptSetOtherOptionHelp(poptContext con, const char * text);

/** \ingroup popt
 * Return argv[0] from context.
 * @param con		context
 * @return		argv[0]
 */
const char * poptGetInvocationName(poptContext con);

/** \ingroup popt
 * Shuffle argv pointers to remove stripped args, returns new argc.
 * @param con		context
 * @param argc		no. of args
 * @param argv		arg vector
 * @return		new argc
 */
int poptStrippedArgv(poptContext con, int argc, char ** argv);

/**
 * Add a string to an argv array.
 * @retval *argvp	argv array
 * @param argInfo	(unused)
 * @param val		string arg to add (using strdup)
 * @return		0 on success, POPT_ERROR_NULLARG/POPT_ERROR_BADOPERATION
 */
int poptSaveString(const char *** argvp, unsigned int argInfo,
		const char * val);

/**
 * Save a long long, performing logical operation with value.
 * @warning Alignment check may be too strict on certain platorms.
 * @param arg		integer pointer, aligned on int boundary.
 * @param argInfo	logical operation (see POPT_ARGFLAG_*)
 * @param aLongLong	value to use
 * @return		0 on success, POPT_ERROR_NULLARG/POPT_ERROR_BADOPERATION
 */
int poptSaveLongLong(long long * arg, unsigned int argInfo,
		long long aLongLong);

/**
 * Save a long, performing logical operation with value.
 * @warning Alignment check may be too strict on certain platorms.
 * @param arg		integer pointer, aligned on int boundary.
 * @param argInfo	logical operation (see POPT_ARGFLAG_*)
 * @param aLong		value to use
 * @return		0 on success, POPT_ERROR_NULLARG/POPT_ERROR_BADOPERATION
 */
int poptSaveLong(long * arg, unsigned int argInfo, long aLong);

/**
 * Save a short integer, performing logical operation with value.
 * @warning Alignment check may be too strict on certain platorms.
 * @param arg		short pointer, aligned on short boundary.
 * @param argInfo	logical operation (see POPT_ARGFLAG_*)
 * @param aLong		value to use
 * @return		0 on success, POPT_ERROR_NULLARG/POPT_ERROR_BADOPERATION
 */
int poptSaveShort(short * arg, unsigned int argInfo, long aLong);

/**
 * Save an integer, performing logical operation with value.
 * @warning Alignment check may be too strict on certain platorms.
 * @param arg		integer pointer, aligned on int boundary.
 * @param argInfo	logical operation (see POPT_ARGFLAG_*)
 * @param aLong		value to use
 * @return		0 on success, POPT_ERROR_NULLARG/POPT_ERROR_BADOPERATION
 */
int poptSaveInt(int * arg, unsigned int argInfo, long aLong);

/* The bit set typedef. */
typedef struct poptBits_s {
    unsigned int bits[1];
} * poptBits;

#define _POPT_BITS_N    1024U    /* estimated population */
#define _POPT_BITS_M    ((3U * _POPT_BITS_N) / 2U)
#define _POPT_BITS_K    16U      /* no. of linear hash combinations */

extern unsigned int _poptBitsN;
extern  unsigned int _poptBitsM;
extern  unsigned int _poptBitsK;

int poptBitsAdd(poptBits bits, const char * s);
int poptBitsChk(poptBits bits, const char * s);
int poptBitsClr(poptBits bits);
int poptBitsDel(poptBits bits, const char * s);
int poptBitsIntersect(poptBits * ap, const poptBits b);
int poptBitsUnion(poptBits * ap, const poptBits b);
int poptBitsArgs(poptContext con, poptBits * ap);

/**
 * Save a string into a bit set (experimental).
 * @retval *bits	bit set (lazily malloc'd if NULL)
 * @param argInfo	logical operation (see POPT_ARGFLAG_*)
 * @param s		string to add to bit set
 * @return		0 on success, POPT_ERROR_NULLARG/POPT_ERROR_BADOPERATION
 */
int poptSaveBits(poptBits * bitsp, unsigned int argInfo,
		const char * s);

#ifdef  __cplusplus
}
#endif

#endif
