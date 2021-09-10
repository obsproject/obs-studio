/** \ingroup popt
 * \file options_popt.cpp
 */

/* This file is a port of the popt library for use with the AJA SDK.
 * The code for the library was consolidated into a .h and .cpp file
 * to simplify building demonstration applications. Not all the features
 * of popt have been tested. Only simple command line parameter parsing
 * was needed for the SDK, but popt was ported to allow enhancing our
 * applications with additional functionality as needed.
*/

/* (C) 1998-2002 Red Hat, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from
   ftp://ftp.rpm.org/pub/rpm/dist */

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

#undef	MYDEBUG

/* This section handles platform differences
*/

#if defined(AJA_LINUX) || defined (AJA_MAC)

#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#define POPT_SYSCONFDIR "."

#endif

#if defined (AJA_MAC)
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#endif

#if defined(AJA_WINDOWS)

#pragma warning (disable:4302)
#pragma warning (disable:4311)
#pragma warning (disable:4996)

#include <BaseTsd.h>
#include <io.h>
#include <process.h>
#include <sys/stat.h>

typedef signed __int8    int8_t;
typedef signed __int16   int16_t;
typedef signed __int32   int32_t;
typedef signed __int64   int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

typedef short uid_t;
static uid_t getuid() { return 0; }

#define strtoll _strtoi64

#define execvp(a, b) _spawnvp(_P_OVERLAY, (a), (b))

#define X_OK 0
static bool access(char* t, int x) {return true;}

#define POPT_SYSCONFDIR "."

#define ssize_t SSIZE_T

#define open(a, b) _open((a), (b))
#define close(a) _close((a))
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_IWGRP 0
#define S_IWOTH 0

/* Copy SRC to DEST, returning the address of the terminating '\0' in DEST.  */
inline char * stpcpy (char *dest, const char * src) {
    register char *d = dest;
    register const char *s = src;

    do
	*d++ = *s;
    while (*s++ != '\0');
    return d - 1;
}

#else
#pragma GCC diagnostic ignored "-Wunused-parameter"
#if defined (AJA_LINUX)
	#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

/* End of platform differences section
*/

/* This section is from popt's system.h
*/

#include <ctype.h>

/* XXX isspace(3) has i18n encoding signednesss issues on Solaris. */
#define	_isspaceptr(_chp)	isspace((int)(*(unsigned char *)(_chp)))

#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void * xmalloc (size_t size);

void * xcalloc (size_t nmemb, size_t size);

void * xrealloc (void * ptr, size_t size);

char * xstrdup (const char *str);

/* Memory allocation via macro defs to get meaningful locations from mtrace() */
#define	xmalloc(_size) 		malloc(_size)
#define	xcalloc(_nmemb, _size)	calloc((_nmemb), (_size))
#define	xrealloc(_ptr, _size)	realloc((_ptr), (_size))
#define	xstrdup(_str)	strdup(_str)

#define __attribute__(x) 
#define UNUSED(x) x __attribute__((__unused__))

#include "options_popt.h"

/* End of section from popt's system.h
*/

#include <float.h>
#include <math.h>

/* This section is from poptint.h
*/

/**
 * Wrapper to free(3), hides const compilation noise, permit NULL, return NULL.
 * @param p		memory to free
 * @retval		NULL always
 */
static inline void *
_free(const void * p)
{
    if (p != NULL)	free((void *)p);
    return NULL;
}

/* Bit mask macros. */
typedef	unsigned int __pbm_bits;
#define	__PBM_NBITS		(8 * sizeof (__pbm_bits))
#define	__PBM_IX(d)		((d) / __PBM_NBITS)
#define __PBM_MASK(d)		((__pbm_bits) 1 << (((unsigned)(d)) % __PBM_NBITS))
typedef struct {
    __pbm_bits bits[1];
} pbm_set;
#define	__PBM_BITS(set)	((set)->bits)

#define	PBM_ALLOC(d)	calloc(__PBM_IX (d) + 1, sizeof(__pbm_bits))
#define	PBM_FREE(s)	_free(s);
#define PBM_SET(d, s)   (__PBM_BITS (s)[__PBM_IX (d)] |= __PBM_MASK (d))
#define PBM_CLR(d, s)   (__PBM_BITS (s)[__PBM_IX (d)] &= ~__PBM_MASK (d))
#define PBM_ISSET(d, s) ((__PBM_BITS (s)[__PBM_IX (d)] & __PBM_MASK (d)) != 0)

extern void poptJlu32lpair(const void *key, size_t size,
                uint32_t *pc, uint32_t *pb);

/** \ingroup popt
 * Typedef's for string and array of strings.
 */
typedef const char * poptString;
typedef poptString * poptArgv;

/** \ingroup popt
 * A union to simplify opt->arg access without casting.
 */
typedef union poptArg_u {
    void * ptr;
    int * intp;
    short * shortp;
    long * longp;
    long long * longlongp;
    float * floatp;
    double * doublep;
    const char ** argv;
    poptCallbackType cb;
    poptOption opt;
} poptArg;

extern unsigned int _poptArgMask;
extern unsigned int _poptGroupMask;

#define	poptArgType(_opt)	((_opt)->argInfo & _poptArgMask)
#define	poptGroup(_opt)		((_opt)->argInfo & _poptGroupMask)

#define	F_ISSET(_opt, _FLAG)	((_opt)->argInfo & POPT_ARGFLAG_##_FLAG)
#define	LF_ISSET(_FLAG)		(argInfo & POPT_ARGFLAG_##_FLAG)
#define	CBF_ISSET(_opt, _FLAG)	((_opt)->argInfo & POPT_CBFLAG_##_FLAG)

/* XXX sick hack to preserve pretense of a popt-1.x ABI. */
#define	poptSubstituteHelpI18N(opt) \
  { \
    if ((&opt) == poptHelpOptions) (opt) = *poptHelpOptionsI18N; \
  }
// if ((opt) == poptHelpOptions) (opt) = poptHelpOptionsI18N;

struct optionStackEntry {
    int argc;
    poptArgv argv;
    pbm_set * argb;
    int next;
    char * nextArg;
    const char * nextCharArg;
    poptItem currAlias;
    int stuffed;
};

struct poptContext_s {
    struct optionStackEntry optionStack[POPT_OPTION_DEPTH];
    struct optionStackEntry * os;
    poptArgv leftovers;
    int numLeftovers;
    int nextLeftover;
    const struct poptOption * options;
    int restLeftover;
    const char * appName;
    poptItem aliases;
    int numAliases;
    unsigned int flags;
    poptItem execs;
    int numExecs;
    poptArgv finalArgv;
    int finalArgvCount;
    int finalArgvAlloced;
    int (*maincall) (int argc, const char **argv);
    poptItem doExec;
    const char * execPath;
    int execAbsolute;
    const char * otherHelp;
    pbm_set * arg_strip;
};

int   POPT_fprintf (FILE* stream, const char *format, ...);

const char *POPT_prev_char (const char *str);

const char *POPT_next_char (const char *str);

#define _(foo) foo

#define D_(dom, str) str
#define POPT_(foo) foo

#define N_(foo) foo


/* End of section from poptint.h
*/

/* This section is from poptconfig.cpp
*/

#include <sys/stat.h>

/**
 * Return path(s) from a glob pattern.
 * @param con		context
 * @param pattern	glob pattern
 * @retval *acp		no. of paths
 * @retval *avp		array of paths
 * @return		0 on success
 */
static int poptGlob(UNUSED(poptContext con), const char * pattern,
		int * acp, const char *** avp)
{
    const char * pat = pattern;
    int rc = 0;		/* assume success */

    /* XXX skip the attention marker. */
    if (pat[0] == '@' && pat[1] != '(')
	pat++;

    {
	if (acp)
	    *acp = 1;
	if (avp && (*avp = (const char**)calloc((size_t)(1 + 1), sizeof (**avp))) != NULL)
	    (*avp)[0] = xstrdup(pat);
    }

    return rc;
}

int poptSaneFile(const char * fn)
{
    struct stat sb;
    uid_t uid = getuid();

    if (stat(fn, &sb) == -1)
	return 1;
    if ((uid_t)sb.st_uid != uid)
	return 0;
    if (!S_ISREG(sb.st_mode))
	return 0;
    if (sb.st_mode & (S_IWGRP|S_IWOTH))
	return 0;
    return 1;
}

int poptReadFile(const char * fn, char ** bp, size_t * nbp, int flags)
{
    int fdno;
    char * b = NULL;
    off_t nb = 0;
    char * s, * t, * se;
    int rc = POPT_ERROR_ERRNO;	/* assume failure */

    fdno = open(fn, O_RDONLY);
    if (fdno < 0)
	goto exit;

    if ((nb = lseek(fdno, 0, SEEK_END)) == (off_t)-1
     || lseek(fdno, 0, SEEK_SET) == (off_t)-1
     || (b = (char*)calloc(sizeof(*b), (size_t)nb + 1)) == NULL
     || read(fdno, (char *)b, (size_t)nb) != (ssize_t)nb)
    {
	int oerrno = errno;
	(void) close(fdno);
	errno = oerrno;
	goto exit;
    }
    if (close(fdno) == -1)
	goto exit;
    if (b == NULL) {
	rc = POPT_ERROR_MALLOC;
	goto exit;
    }
    rc = 0;

   /* Trim out escaped newlines. */
    if (flags & POPT_READFILE_TRIMNEWLINES)
    {
	for (t = b, s = b, se = b + nb; *s && s < se; s++) {
	    switch (*s) {
	    case '\\':
		if (s[1] == '\n') {
		    s++;
		    continue;
		}
	    default:
		*t++ = *s;
		break;
	    }
	}
	*t++ = '\0';
	nb = (off_t)(t - b);
    }

exit:
    if (rc != 0) {
	if (b)
	    free(b);
	b = NULL;
	nb = 0;
    }
    if (bp)
	*bp = b;
    else if (b)
	free(b);
    if (nbp)
	*nbp = (size_t)nb;
    return rc;
}

/**
 * Check for application match.
 * @param con		context
 * @param s		config application name
 * return		0 if config application matches
 */
static int configAppMatch(poptContext con, const char * s)
{
    int rc = 1;

    if (con->appName == NULL)	/* XXX can't happen. */
	return rc;

	rc = strcmp(s, con->appName);
    return rc;
}

static int poptConfigLine(poptContext con, char * line)
{
    char *b = NULL;
    size_t nb = 0;
    char * se = line;
    const char * appName;
    const char * entryType;
    const char * opt;
    struct poptItem_s item_buf;
    poptItem item = &item_buf;
    int i, j;
    int rc = POPT_ERROR_BADCONFIG;

    if (con->appName == NULL)
	goto exit;
    
    memset(item, 0, sizeof(*item));

    appName = se;
    while (*se != '\0' && !_isspaceptr(se)) se++;
    if (*se == '\0')
	goto exit;
    else
	*se++ = '\0';

    if (configAppMatch(con, appName)) goto exit;

    while (*se != '\0' && _isspaceptr(se)) se++;
    entryType = se;
    while (*se != '\0' && !_isspaceptr(se)) se++;
    if (*se != '\0') *se++ = '\0';

    while (*se != '\0' && _isspaceptr(se)) se++;
    if (*se == '\0') goto exit;
    opt = se;
    while (*se != '\0' && !_isspaceptr(se)) se++;
    if (opt[0] == '-' && *se == '\0') goto exit;
    if (*se != '\0') *se++ = '\0';

    while (*se != '\0' && _isspaceptr(se)) se++;
    if (opt[0] == '-' && *se == '\0') goto exit;

    if (opt[0] == '-' && opt[1] == '-')
	item->option.longName = opt + 2;
    else if (opt[0] == '-' && opt[2] == '\0')
	item->option.shortName = opt[1];
    else {
	const char * fn = opt;

	/* XXX handle globs and directories in fn? */
	if ((rc = poptReadFile(fn, &b, &nb, POPT_READFILE_TRIMNEWLINES)) != 0)
	    goto exit;
	if (b == NULL || nb == 0)
	    goto exit;

	/* Append remaining text to the interpolated file option text. */
	if (*se != '\0') {
	    size_t nse = strlen(se) + 1;
	    if ((b = (char*)realloc(b, (nb + nse))) == NULL)	/* XXX can't happen */
		goto exit;
	    (void) stpcpy( stpcpy(&b[nb-1], " "), se);
	    nb += nse;
	}
	se = b;

	/* Use the basename of the path as the long option name. */
	{   const char * longName = strrchr(fn, '/');
	    if (longName != NULL)
		longName++;
	    else
		longName = fn;
	    if (longName == NULL)	/* XXX can't happen. */
		goto exit;
	    /* Single character basenames are treated as short options. */
	    if (longName[1] != '\0')
		item->option.longName = longName;
	    else
		item->option.shortName = longName[0];
	}
    }

    if (poptParseArgvString(se, &item->argc, &item->argv)) goto exit;

    item->option.argInfo = POPT_ARGFLAG_DOC_HIDDEN;
    for (i = 0, j = 0; i < item->argc; i++, j++) {
	const char * f;
	if (!strncmp(item->argv[i], "--POPTdesc=", sizeof("--POPTdesc=")-1)) {
	    f = item->argv[i] + sizeof("--POPTdesc=");
	    if (f[0] == '$' && f[1] == '"') f++;
	    item->option.descrip = f;
	    item->option.argInfo &= ~POPT_ARGFLAG_DOC_HIDDEN;
	    j--;
	} else
	if (!strncmp(item->argv[i], "--POPTargs=", sizeof("--POPTargs=")-1)) {
	    f = item->argv[i] + sizeof("--POPTargs=");
	    if (f[0] == '$' && f[1] == '"') f++;
	    item->option.argDescrip = f;
	    item->option.argInfo &= ~POPT_ARGFLAG_DOC_HIDDEN;
	    item->option.argInfo |= POPT_ARG_STRING;
	    j--;
	} else
	if (j != i)
	    item->argv[j] = item->argv[i];
    }
    if (j != i) {
	item->argv[j] = NULL;
	item->argc = j;
    }
	
    if (!strcmp(entryType, "alias"))
	rc = poptAddItem(con, item, 0);
    else if (!strcmp(entryType, "exec"))
	rc = poptAddItem(con, item, 1);
exit:
    rc = 0;	/* XXX for now, always return success */
    if (b)
	free(b);
    return rc;
}

int poptReadConfigFile(poptContext con, const char * fn)
{
    char * b = NULL, *be;
    size_t nb = 0;
    const char *se;
    char *t, *te;
    int rc;

    if ((rc = poptReadFile(fn, &b, &nb, POPT_READFILE_TRIMNEWLINES)) != 0)
	return (errno == ENOENT ? 0 : rc);
    if (b == NULL || nb == 0)
	return POPT_ERROR_BADCONFIG;

    if ((t = (char*)malloc(nb + 1)) == NULL)
	goto exit;
    te = t;

    be = (b + nb);
    for (se = b; se < be; se++) {
	switch (*se) {
	  case '\n':
	    *te = '\0';
	    te = t;
	    while (*te && _isspaceptr(te)) te++;
	    if (*te && *te != '#')
		    poptConfigLine(con, te);
	    break;
	  case '\\':
	    *te = *se++;
	    /* \ at the end of a line does not insert a \n */
	    if (se < be && *se != '\n') {
		te++;
		*te++ = *se;
	    }
	    break;
	  default:
	    *te++ = *se;
	    break;
	}
    }

    free(t);
    rc = 0;

exit:
    if (b)
	free(b);
    return rc;
}

int poptReadConfigFiles(poptContext con, const char * paths)
{
    char * buf = (paths ? xstrdup(paths) : NULL);
    const char * p;
    char * pe;
    int rc = 0;		/* assume success */

    for (p = buf; p != NULL && *p != '\0'; p = pe) {
	const char ** av = NULL;
	int ac = 0;
	int i;
	int xx;

	/* locate start of next path element */
	pe = (char*)strchr(p, ':');
	if (pe != NULL && *pe == ':')
	    *pe++ = '\0';
	else
	    pe = (char *) (p + strlen(p));

	xx = poptGlob(con, p, &ac, &av);

	/* work-off each resulting file from the path element */
	for (i = 0; i < ac; i++) {
	    const char * fn = av[i];
	    if (av[i] == NULL)	/* XXX can't happen */
		continue;
	    /* XXX should '@' attention be pushed into poptReadConfigFile? */
	    if (p[0] == '@' && p[1] != '(') {
		if (fn[0] == '@' && fn[1] != '(')
		    fn++;
		xx = poptSaneFile(fn);
		if (!xx && rc == 0)
		    rc = POPT_ERROR_BADCONFIG;
		continue;
	    }
	    xx = poptReadConfigFile(con, fn);
	    if (xx && rc == 0)
		rc = xx;
	    free((void *)av[i]);
	    av[i] = NULL;
	}
	free(av);
	av = NULL;
    }

    if (buf)
	free(buf);

    return rc;
}

int poptReadDefaultConfig(poptContext con, UNUSED(int useEnv))
{
    static const char _popt_sysconfdir[] = POPT_SYSCONFDIR "/popt";
    static const char _popt_etc[] = "/etc/popt";
    char * home;
    int rc = 0;		/* assume success */

    if (con->appName == NULL) goto exit;

    if (strcmp(_popt_sysconfdir, _popt_etc)) {
	rc = poptReadConfigFile(con, _popt_sysconfdir);
	if (rc) goto exit;
    }

    rc = poptReadConfigFile(con, _popt_etc);
    if (rc) goto exit;

    if ((home = getenv("HOME"))) {
	char * fn = (char*)malloc(strlen(home) + 20);
	if (fn != NULL) {
	    (void) stpcpy(stpcpy(fn, home), "/.popt");
	    rc = poptReadConfigFile(con, fn);
	    free(fn);
	} else
	    rc = POPT_ERROR_ERRNO;
	if (rc) goto exit;
    }

exit:
    return rc;
}

poptContext
poptFini(poptContext con)
{
    return poptFreeContext(con);
}

poptContext
poptInit(int argc, const char ** argv,
		const struct poptOption * options, const char * configPaths)
{
    poptContext con = NULL;
    const char * argv0;

    if (argv == NULL || argv[0] == NULL || options == NULL)
	return con;

    if ((argv0 = strrchr(argv[0], '/')) != NULL) argv0++;
    else argv0 = argv[0];
   
    con = poptGetContext(argv0, argc, (const char **)argv, options, 0);
    if (con != NULL&& poptReadConfigFiles(con, configPaths))
	con = poptFini(con);

    return con;
}

/* End of section from poptconfig.cpp
*/

/* This section is from popthelp.cpp
*/

#define	POPT_WCHAR_HACK
#ifdef 	POPT_WCHAR_HACK
#ifndef AJAMac
#include <wchar.h>			/* for mbsrtowcs */
#endif
#endif

/**
 * Display arguments.
 * @param con		context
 * @param foo		(unused)
 * @param key		option(s)
 * @param arg		(unused)
 * @param data		(unused)
 */
static void displayArgs(poptContext con,
		UNUSED(enum poptCallbackReason foo),
		struct poptOption * key, 
		UNUSED(const char * arg),
		UNUSED(void * data))
{
    if (key->shortName == '?')
	poptPrintHelp(con, stdout, 0);
    else
	poptPrintUsage(con, stdout, 0);

    exit(0);
}

/**
 * Empty table marker to enable displaying popt alias/exec options.
 */
struct poptOption poptAliasOptions[] = {
    POPT_TABLEEND
};

/**
 * Auto help table options.
 */
struct poptOption poptHelpOptions[] = {
  { NULL, '\0', POPT_ARG_CALLBACK, (void *)displayArgs, 0, NULL, NULL },
  { "help", '?', 0, NULL, (int)'?', N_("Show this help message"), NULL },
  { "usage", '\0', 0, NULL, (int)'u', N_("Display brief usage message"), NULL },
    POPT_TABLEEND
} ;

static struct poptOption poptHelpOptions2[] = {
	{ NULL, '\0', POPT_ARG_INTL_DOMAIN, (void*)"PACKAGE", 0, NULL, NULL},
  { NULL, '\0', POPT_ARG_CALLBACK, (void *)displayArgs, 0, NULL, NULL },
  { "help", '?', 0, NULL, (int)'?', N_("Show this help message"), NULL },
  { "usage", '\0', 0, NULL, (int)'u', N_("Display brief usage message"), NULL },
  { "", '\0',	0, NULL, 0, N_("Terminate options"), NULL },
    POPT_TABLEEND
} ;

struct poptOption * poptHelpOptionsI18N = poptHelpOptions2;

#define        _POPTHELP_MAXLINE       ((size_t)79)

typedef struct columns_s {
    size_t cur;
    size_t max;
} * columns_t;

/** 
 * Return no. of columns in output window.
 * @param fp           FILE
 * @return             no. of columns 
 */ 
static size_t maxColumnWidth(FILE *fp)
{   
    size_t maxcols = _POPTHELP_MAXLINE;
    return maxcols;
}   

/**
 * Determine number of display characters in a string.
 * @param s		string
 * @return		no. of display characters.
 */
static inline size_t stringDisplayWidth(const char *s)
{
    size_t n = strlen(s);
#ifndef AJAMac
    mbstate_t t;

    memset ((void *)&t, 0, sizeof (t));	/* In initial state.  */
    /* Determine number of display characters.  */
    n = mbsrtowcs (NULL, &s, n, &t);
#endif
    return n;
}

/**
 * @param opt		option(s)
 */
static const char *
getTableTranslationDomain(const struct poptOption *opt)
{
    if (opt != NULL)
    for (; opt->longName || opt->shortName || opt->arg; opt++) {
	if (opt->argInfo == POPT_ARG_INTL_DOMAIN)
	    return (const char*)opt->arg;
    }
    return NULL;
}

/**
 * @param opt		option(s)
 * @param translation_domain	translation domain
 */
static const char *
getArgDescrip(const struct poptOption * opt,
		const char * translation_domain)
{
    if (!poptArgType(opt)) return NULL;

    if (poptArgType(opt) == POPT_ARG_MAINCALL)
	return opt->argDescrip;
    if (poptArgType(opt) == POPT_ARG_ARGV)
	return opt->argDescrip;

    if (opt->argDescrip) {
	/* Some strings need popt library, not application, i18n domain. */
	if (opt == (poptHelpOptions + 1)
	 || opt == (poptHelpOptions + 2)
	 || !strcmp(opt->argDescrip, N_("Help options:"))
	 || !strcmp(opt->argDescrip, N_("Options implemented via popt alias/exec:")))
	    return POPT_(opt->argDescrip);

	/* Use the application i18n domain. */
	return D_(translation_domain, opt->argDescrip);
    }

    switch (poptArgType(opt)) {
    case POPT_ARG_NONE:		return POPT_("NONE");
#ifdef	DYING
    case POPT_ARG_VAL:		return POPT_("VAL");
#else
    case POPT_ARG_VAL:		return NULL;
#endif
    case POPT_ARG_INT:		return POPT_("INT");
    case POPT_ARG_SHORT:	return POPT_("SHORT");
    case POPT_ARG_LONG:		return POPT_("LONG");
    case POPT_ARG_LONGLONG:	return POPT_("LONGLONG");
    case POPT_ARG_STRING:	return POPT_("STRING");
    case POPT_ARG_FLOAT:	return POPT_("FLOAT");
    case POPT_ARG_DOUBLE:	return POPT_("DOUBLE");
    case POPT_ARG_MAINCALL:	return NULL;
    case POPT_ARG_ARGV:		return NULL;
    default:			return POPT_("ARG");
    }
}

/**
 * Display default value for an option.
 * @param lineLength	display positions remaining
 * @param opt		option(s)
 * @param translation_domain	translation domain
 * @return	a pointer to the display string.
 */
static char *
singleOptionDefaultValue(size_t lineLength,
		const struct poptOption * opt,
		const char * translation_domain)
{
    const char * defstr = D_(translation_domain, "default");
    char * le = (char*)malloc(4*lineLength + 1);
    char * l = le;

    if (le == NULL) return NULL;	/* XXX can't happen */
    *le = '\0';
    *le++ = '(';
    le = stpcpy(le, defstr);
    *le++ = ':';
    *le++ = ' ';
  if (opt->arg) {	/* XXX programmer error */
    poptArg arg;
	arg.ptr = opt->arg;
    switch (poptArgType(opt)) {
    case POPT_ARG_VAL:
    case POPT_ARG_INT:
	le += sprintf(le, "%d", arg.intp[0]);
	break;
    case POPT_ARG_SHORT:
	le += sprintf(le, "%hd", arg.shortp[0]);
	break;
    case POPT_ARG_LONG:
	le += sprintf(le, "%ld", arg.longp[0]);
	break;
    case POPT_ARG_LONGLONG:
	le += sprintf(le, "%lld", arg.longlongp[0]);
	break;
    case POPT_ARG_FLOAT:
    {	double aDouble = (double) arg.floatp[0];
	le += sprintf(le, "%g", aDouble);
    }	break;
    case POPT_ARG_DOUBLE:
	le += sprintf(le, "%g", arg.doublep[0]);
	break;
    case POPT_ARG_MAINCALL:
	le += sprintf(le, "%p", opt->arg);
	break;
    case POPT_ARG_ARGV:
	le += sprintf(le, "%p", opt->arg);
	break;
    case POPT_ARG_STRING:
    {	const char * s = arg.argv[0];
	if (s == NULL)
	    le = stpcpy(le, "null");
	else {
	    size_t limit = 4*lineLength - (le - l) - sizeof("\"\")");
	    size_t slen;
	    *le++ = '"';
	    strncpy(le, s, limit); le[limit] = '\0'; le += (slen = strlen(le));
	    if (slen == limit && s[limit])
		le[-1] = le[-2] = le[-3] = '.';
	    *le++ = '"';
	}
    }	break;
    case POPT_ARG_NONE:
    default:
	l = (char*)_free(l);
	return NULL;
	break;
    }
  }
    *le++ = ')';
    *le = '\0';

    return l;
}

/**
 * Display help text for an option.
 * @param fp		output file handle
 * @param columns	output display width control
 * @param opt		option(s)
 * @param translation_domain	translation domain
 */
static void singleOptionHelp(FILE * fp, columns_t columns,
		const struct poptOption * opt,
		const char * translation_domain)
{
    size_t maxLeftCol = columns->cur;
    size_t indentLength = maxLeftCol + 5;
    size_t lineLength = columns->max - indentLength;
    const char * help = D_(translation_domain, opt->descrip);
    const char * argDescrip = getArgDescrip(opt, translation_domain);
    /* Display shortName iff printable non-space. */
    int prtshort = (int)(isprint((int)opt->shortName) && opt->shortName != ' ');
    size_t helpLength;
    char * defs = NULL;
    char * left;
    size_t nb = maxLeftCol + 1;
    int displaypad = 0;

    /* Make sure there's more than enough room in target buffer. */
    if (opt->longName)	nb += strlen(opt->longName);
    if (F_ISSET(opt, TOGGLE)) nb += sizeof("[no]") - 1;
    if (argDescrip)	nb += strlen(argDescrip);

    left = (char*)malloc(nb);
    if (left == NULL) return;	/* XXX can't happen */
    left[0] = '\0';
    left[maxLeftCol] = '\0';

#define	prtlong	(opt->longName != NULL)	/* XXX splint needs a clue */
    if (!(prtshort || prtlong))
	goto out;
    if (prtshort && prtlong) {
	char *dash = F_ISSET(opt, ONEDASH) ? (char*)"-" : (char*)"--";
	left[0] = '-';
	left[1] = opt->shortName;
	(void) stpcpy(stpcpy(stpcpy(left+2, ", "), dash), opt->longName);
    } else if (prtshort) {
	left[0] = '-';
	left[1] = opt->shortName;
	left[2] = '\0';
    } else if (prtlong) {
	/* XXX --long always padded for alignment with/without "-X, ". */
	char *dash = poptArgType(opt) == POPT_ARG_MAINCALL ? (char*)""
		   : (F_ISSET(opt, ONEDASH) ? (char*)"-" : (char*)"--");
	const char *longName = opt->longName;
	const char *toggle;
	if (F_ISSET(opt, TOGGLE)) {
	    toggle = "[no]";
	    if (longName[0] == 'n' && longName[1] == 'o') {
		longName += sizeof("no") - 1;
		if (longName[0] == '-')
		    longName++;
	    }
	} else
	    toggle = "";
	(void) stpcpy(stpcpy(stpcpy(stpcpy(left, "    "), dash), toggle), longName);
    }
#undef	prtlong

    if (argDescrip) {
	char * le = left + strlen(left);

	if (F_ISSET(opt, OPTIONAL))
	    *le++ = '[';

	/* Choose type of output */
	if (F_ISSET(opt, SHOW_DEFAULT)) {
	    defs = singleOptionDefaultValue(lineLength, opt, translation_domain);
	    if (defs) {
		char * t = (char*)malloc((help ? strlen(help) : 0) +
				strlen(defs) + sizeof(" "));
		if (t) {
		    char * te = t;
		    if (help)
			te = stpcpy(te, help);
		    *te++ = ' ';
		    strcpy(te, defs);
		    defs = (char*)_free(defs);
		    defs = t;
		}
	    }
	}

	if (opt->argDescrip == NULL) {
	    switch (poptArgType(opt)) {
	    case POPT_ARG_NONE:
		break;
	    case POPT_ARG_VAL:
		break;
	    case POPT_ARG_INT:
	    case POPT_ARG_SHORT:
	    case POPT_ARG_LONG:
	    case POPT_ARG_LONGLONG:
	    case POPT_ARG_FLOAT:
	    case POPT_ARG_DOUBLE:
	    case POPT_ARG_STRING:
		*le++ = (opt->longName != NULL ? '=' : ' ');
		le = stpcpy(le, argDescrip);
		break;
	    default:
		break;
	    }
	} else {
	    char *leo;

	    /* XXX argDescrip[0] determines "--foo=bar" or "--foo bar". */
	    if (!strchr(" =(", argDescrip[0]))
		*le++ = ((poptArgType(opt) == POPT_ARG_MAINCALL) ? ' ' :
			 (poptArgType(opt) == POPT_ARG_ARGV) ? ' ' : '=');
	    le = stpcpy(leo = le, argDescrip);

	    /* Adjust for (possible) wide characters. */
	    displaypad = (int)((le - leo) - stringDisplayWidth(argDescrip));
	}
	if (F_ISSET(opt, OPTIONAL))
	    *le++ = ']';
	*le = '\0';
    }

    if (help)
	    POPT_fprintf(fp,"  %-*s   ", (int)(maxLeftCol+displaypad), left);
    else {
	    POPT_fprintf(fp,"  %s\n", left); 
	goto out;
    }

    left = (char*)_free(left);
    if (defs)
	help = defs;

    helpLength = strlen(help);
    while (helpLength > lineLength) {
	const char * ch;
	char format[16];

	ch = help + lineLength - 1;
	while (ch > help && !_isspaceptr(ch))
	    ch = POPT_prev_char(ch);
	if (ch == help) break;		/* give up */
	while (ch > (help + 1) && _isspaceptr(ch))
	    ch = POPT_prev_char (ch);
	ch = POPT_next_char(ch);

	/*
	 *  XXX strdup is necessary to add NUL terminator so that an unknown
	 *  no. of (possible) multi-byte characters can be displayed.
	 */
	{   char * fmthelp = xstrdup(help);
	    if (fmthelp) {
		fmthelp[ch - help] = '\0';
		sprintf(format, "%%s\n%%%ds", (int) indentLength);
		POPT_fprintf(fp, format, fmthelp, " ");
		free(fmthelp);
	    }
	}

	help = ch;
	while (_isspaceptr(help) && *help)
	    help = POPT_next_char(help);
	helpLength = strlen(help);
    }

    if (helpLength) fprintf(fp, "%s\n", help);
    help = NULL;

out:
    defs = (char*)_free(defs);
    left = (char*)_free(left);
}

/**
 * Find display width for longest argument string.
 * @param opt		option(s)
 * @param translation_domain	translation domain
 * @return		display width
 */
static size_t maxArgWidth(const struct poptOption * opt,
		       const char * translation_domain)
{
    size_t max = 0;
    size_t len = 0;
    const char * argDescrip;
    
    if (opt != NULL)
    while (opt->longName || opt->shortName || opt->arg) {
	if (poptArgType(opt) == POPT_ARG_INCLUDE_TABLE) {
	    if (opt->arg)	/* XXX program error */
	        len = maxArgWidth((const poptOption*)opt->arg, translation_domain);
	    if (len > max) max = len;
	} else if (!F_ISSET(opt, DOC_HIDDEN)) {
	    len = sizeof("  ")-1;
	    /* XXX --long always padded for alignment with/without "-X, ". */
	    len += sizeof("-X, ")-1;
	    if (opt->longName) {
		len += (F_ISSET(opt, ONEDASH) ? sizeof("-") : sizeof("--")) - 1;
		len += strlen(opt->longName);
	    }

	    argDescrip = getArgDescrip(opt, translation_domain);

	    if (argDescrip) {

		/* XXX argDescrip[0] determines "--foo=bar" or "--foo bar". */
		if (!strchr(" =(", argDescrip[0])) len += sizeof("=")-1;

		/* Adjust for (possible) wide characters. */
		len += stringDisplayWidth(argDescrip);
	    }

	    if (F_ISSET(opt, OPTIONAL)) len += sizeof("[]")-1;
	    if (len > max) max = len;
	}
	opt++;
    }
    
    return max;
}

/**
 * Display popt alias and exec help.
 * @param fp		output file handle
 * @param items		alias/exec array
 * @param nitems	no. of alias/exec entries
 * @param columns	output display width control
 * @param translation_domain	translation domain
 */
static void itemHelp(FILE * fp,
		poptItem items, int nitems,
		columns_t columns,
		const char * translation_domain)
{
    poptItem item;
    int i;

    if (items != NULL)
    for (i = 0, item = items; i < nitems; i++, item++) {
	const struct poptOption * opt;
	opt = &item->option;
	if ((opt->longName || opt->shortName) && !F_ISSET(opt, DOC_HIDDEN))
	    singleOptionHelp(fp, columns, opt, translation_domain);
    }
}

/**
 * Display help text for a table of options.
 * @param con		context
 * @param fp		output file handle
 * @param table		option(s)
 * @param columns	output display width control
 * @param translation_domain	translation domain
 */
static void singleTableHelp(poptContext con, FILE * fp,
		const struct poptOption * table,
		columns_t columns,
		const char * translation_domain)
{
    const struct poptOption * opt;
    const char *sub_transdom;

    if (table == poptAliasOptions) {
	itemHelp(fp, con->aliases, con->numAliases, columns, NULL);
	itemHelp(fp, con->execs, con->numExecs, columns, NULL);
	return;
    }

    if (table != NULL)
    for (opt = table; opt->longName || opt->shortName || opt->arg; opt++) {
	if ((opt->longName || opt->shortName) && !F_ISSET(opt, DOC_HIDDEN))
	    singleOptionHelp(fp, columns, opt, translation_domain);
    }

    if (table != NULL)
    for (opt = table; opt->longName || opt->shortName || opt->arg; opt++) {
	if (poptArgType(opt) != POPT_ARG_INCLUDE_TABLE)
	    continue;
	sub_transdom = getTableTranslationDomain((const poptOption*)opt->arg);
	if (sub_transdom == NULL)
	    sub_transdom = translation_domain;
	    
	/* If no popt aliases/execs, skip poptAliasOption processing. */
	if (opt->arg == poptAliasOptions && !(con->numAliases || con->numExecs))
	    continue;
	if (opt->descrip)
	    POPT_fprintf(fp, "\n%s\n", D_(sub_transdom, opt->descrip));

	singleTableHelp(con, fp, (const poptOption*)opt->arg, columns, sub_transdom);
    }
}

/**
 * @param con		context
 * @param fp		output file handle
 */
static size_t showHelpIntro(poptContext con, FILE * fp)
{
    size_t len = (size_t)6;

    POPT_fprintf(fp, POPT_("Usage:"));
    if (!(con->flags & POPT_CONTEXT_KEEP_FIRST)) {
	struct optionStackEntry * os = con->optionStack;
	const char * fn = (os->argv ? os->argv[0] : NULL);
	if (fn == NULL) return len;
	if (strchr(fn, '/')) fn = strrchr(fn, '/') + 1;
	/* XXX POPT_fprintf not needed for argv[0] display. */
	fprintf(fp, " %s", fn);
	len += strlen(fn) + 1;
    }

    return len;
}

void poptPrintHelp(poptContext con, FILE * fp, UNUSED(int flags))
{
    columns_t columns = (columns_t)calloc((size_t)1, sizeof(*columns));

    (void) showHelpIntro(con, fp);
    if (con->otherHelp)
	    POPT_fprintf(fp, " %s\n", con->otherHelp);
    else
	    POPT_fprintf(fp, " %s\n", POPT_("[OPTION...]"));

    if (columns) {
	columns->cur = maxArgWidth(con->options, NULL);
	columns->max = maxColumnWidth(fp);
	singleTableHelp(con, fp, con->options, columns, NULL);
	free(columns);
    }
}

/**
 * Display usage text for an option.
 * @param fp		output file handle
 * @param columns	output display width control
 * @param opt		option(s)
 * @param translation_domain	translation domain
 */
static size_t singleOptionUsage(FILE * fp, columns_t columns,
		const struct poptOption * opt,
		const char *translation_domain)
{
    size_t len = sizeof(" []")-1;
    const char * argDescrip = getArgDescrip(opt, translation_domain);
    /* Display shortName iff printable non-space. */
    int prtshort = (int)(isprint((int)opt->shortName) && opt->shortName != ' ');

#define	prtlong	(opt->longName != NULL)	/* XXX splint needs a clue */
    if (!(prtshort || prtlong))
	return columns->cur;

    len = sizeof(" []")-1;
    if (prtshort)
	len += sizeof("-c")-1;
    if (prtlong) {
	if (prtshort) len += sizeof("|")-1;
	len += (F_ISSET(opt, ONEDASH) ? sizeof("-") : sizeof("--")) - 1;
	len += strlen(opt->longName);
    }

    if (argDescrip) {

	/* XXX argDescrip[0] determines "--foo=bar" or "--foo bar". */
	if (!strchr(" =(", argDescrip[0])) len += sizeof("=")-1;

	/* Adjust for (possible) wide characters. */
	len += stringDisplayWidth(argDescrip);
    }

    if ((columns->cur + len) > columns->max) {
	fprintf(fp, "\n       ");
	columns->cur = (size_t)7;
    } 

    fprintf(fp, " [");
    if (prtshort)
	fprintf(fp, "-%c", opt->shortName);
    if (prtlong)
	fprintf(fp, "%s%s%s",
		(prtshort ? "|" : ""),
		(F_ISSET(opt, ONEDASH) ? "-" : "--"),
		opt->longName);
#undef	prtlong

    if (argDescrip) {
	/* XXX argDescrip[0] determines "--foo=bar" or "--foo bar". */
	if (!strchr(" =(", argDescrip[0])) fprintf(fp, "=");
	fprintf(fp, "%s", argDescrip);
    }
    fprintf(fp, "]");

    return columns->cur + len + 1;
}

/**
 * Display popt alias and exec usage.
 * @param fp		output file handle
 * @param columns	output display width control
 * @param item		alias/exec array
 * @param nitems	no. of ara/exec entries
 * @param translation_domain	translation domain
 */
static size_t itemUsage(FILE * fp, columns_t columns,
		poptItem item, int nitems,
		const char * translation_domain)
{
    int i;

    if (item != NULL)
    for (i = 0; i < nitems; i++, item++) {
	const struct poptOption * opt;
	opt = &item->option;
        if (poptArgType(opt) == POPT_ARG_INTL_DOMAIN) {
	    translation_domain = (const char *)opt->arg;
	} else
	if ((opt->longName || opt->shortName) && !F_ISSET(opt, DOC_HIDDEN)) {
	    columns->cur = singleOptionUsage(fp, columns, opt, translation_domain);
	}
    }

    return columns->cur;
}

/**
 * Keep track of option tables already processed.
 */
typedef struct poptDone_s {
    int nopts;
    int maxopts;
    const void ** opts;
} * poptDone;

/**
 * Display usage text for a table of options.
 * @param con		context
 * @param fp		output file handle
 * @param columns	output display width control
 * @param opt		option(s)
 * @param translation_domain	translation domain
 * @param done		tables already processed
 * @return the size of the current column
 */
static size_t singleTableUsage(poptContext con, FILE * fp, columns_t columns,
		const struct poptOption * opt,
		const char * translation_domain,
		poptDone done)
{
    if (opt != NULL)
    for (; (opt->longName || opt->shortName || opt->arg) ; opt++) {
        if (poptArgType(opt) == POPT_ARG_INTL_DOMAIN) {
	    translation_domain = (const char *)opt->arg;
	} else
	if (poptArgType(opt) == POPT_ARG_INCLUDE_TABLE) {
	    if (done) {
		int i = 0;
		if (done->opts != NULL)
		for (i = 0; i < done->nopts; i++) {
		    const void * that = done->opts[i];
		    if (that == NULL || that != opt->arg)
			continue;
		    break;
		}
		/* Skip if this table has already been processed. */
		if (opt->arg == NULL || i < done->nopts)
		    continue;
		if (done->opts != NULL && done->nopts < done->maxopts)
		    done->opts[done->nopts++] = (const void *) opt->arg;
	    }
	    columns->cur = singleTableUsage(con, fp, columns, (const poptOption*)opt->arg,
			translation_domain, done);
	} else
	if ((opt->longName || opt->shortName) && !F_ISSET(opt, DOC_HIDDEN)) {
	    columns->cur = singleOptionUsage(fp, columns, opt, translation_domain);
	}
    }

    return columns->cur;
}

/**
 * Return concatenated short options for display.
 * @todo Sub-tables should be recursed.
 * @param opt		option(s)
 * @param fp		output file handle
 * @retval str		concatenation of short options
 * @return		length of display string
 */
static size_t showShortOptions(const struct poptOption * opt, FILE * fp,
		char * str)
{
    /* bufsize larger then the ascii set, lazy allocation on top level call. */
    size_t nb = (size_t)300;
    char * s = (str != NULL ? str : (char*)calloc((size_t)1, nb));
    size_t len = (size_t)0;

    if (s == NULL)
	return 0;

    if (opt != NULL)
    for (; (opt->longName || opt->shortName || opt->arg); opt++) {
	if (!F_ISSET(opt, DOC_HIDDEN) && opt->shortName && !poptArgType(opt))
	{
	    /* Display shortName iff unique printable non-space. */
	    if (!strchr(s, opt->shortName) && isprint((int)opt->shortName)
	     && opt->shortName != ' ')
		s[strlen(s)] = opt->shortName;
	} else if (poptArgType(opt) == POPT_ARG_INCLUDE_TABLE)
	    if (opt->arg)	/* XXX program error */
		len = showShortOptions((const poptOption*)opt->arg, fp, s);
    } 

    /* On return to top level, print the short options, return print length. */
    if (s != str && *s != '\0') {
	fprintf(fp, " [-%s]", s);
	len = strlen(s) + sizeof(" [-]")-1;
    }
    if (s != str)
	free(s);
    return len;
}

void poptPrintUsage(poptContext con, FILE * fp, UNUSED(int flags))
{
    columns_t columns = (columns_t)calloc((size_t)1, sizeof(*columns));
    struct poptDone_s done_buf;
    poptDone done = &done_buf;

    memset(done, 0, sizeof(*done));
    done->nopts = 0;
    done->maxopts = 64;
  if (columns) {
    columns->cur = done->maxopts * sizeof(*done->opts);
    columns->max = maxColumnWidth(fp);
    done->opts = (const void**)calloc((size_t)1, columns->cur);
    if (done->opts != NULL)
	done->opts[done->nopts++] = (const void *) con->options;

    columns->cur = showHelpIntro(con, fp);
    columns->cur += showShortOptions(con->options, fp, NULL);
    columns->cur = singleTableUsage(con, fp, columns, con->options, NULL, done);
    columns->cur = itemUsage(fp, columns, con->aliases, con->numAliases, NULL);
    columns->cur = itemUsage(fp, columns, con->execs, con->numExecs, NULL);

    if (con->otherHelp) {
	columns->cur += strlen(con->otherHelp) + 1;
	if (columns->cur > columns->max) fprintf(fp, "\n       ");
	fprintf(fp, " %s", con->otherHelp);
    }

    fprintf(fp, "\n");
    if (done->opts != NULL)
	free(done->opts);
    free(columns);
  }
}

void poptSetOtherOptionHelp(poptContext con, const char * text)
{
    con->otherHelp = (const char*)_free(con->otherHelp);
    con->otherHelp = xstrdup(text);
}

/* End of section from popthelp.cpp
*/

/* This section is from poptint.cpp
*/

#include <stdarg.h>

/* Any pair of 32 bit hashes can be used. lookup3.c generates pairs, will do. */
#define _JLU3_jlu32lpair        1
#define	jlu32lpair	poptJlu32lpair

/* This section is from lookup3.c
*/

/* -------------------------------------------------------------------- */
/*
 * lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 * 
 * These are functions for producing 32-bit hashes for hash table lookup.
 * jlu32w(), jlu32l(), jlu32lpair(), jlu32b(), _JLU3_MIX(), and _JLU3_FINAL() 
 * are externally useful functions.  Routines to test the hash are included 
 * if SELF_TEST is defined.  You can use this free for any purpose.  It's in
 * the public domain.  It has no warranty.
 * 
 * You probably want to use jlu32l().  jlu32l() and jlu32b()
 * hash byte arrays.  jlu32l() is is faster than jlu32b() on
 * little-endian machines.  Intel and AMD are little-endian machines.
 * On second thought, you probably want jlu32lpair(), which is identical to
 * jlu32l() except it returns two 32-bit hashes for the price of one.  
 * You could implement jlu32bpair() if you wanted but I haven't bothered here.
 * 
 * If you want to find a hash of, say, exactly 7 integers, do
 *   a = i1;  b = i2;  c = i3;
 *   _JLU3_MIX(a,b,c);
 *   a += i4; b += i5; c += i6;
 *   _JLU3_MIX(a,b,c);
 *   a += i7;
 *   _JLU3_FINAL(a,b,c);
 * then use c as the hash value.  If you have a variable size array of
 * 4-byte integers to hash, use jlu32w().  If you have a byte array (like
 * a character string), use jlu32l().  If you have several byte arrays, or
 * a mix of things, see the comments above jlu32l().  
 * 
 * Why is this so big?  I read 12 bytes at a time into 3 4-byte integers, 
 * then mix those integers.  This is fast (you can do a lot more thorough
 * mixing with 12*3 instructions on 3 integers than you can with 3 instructions
 * on 1 byte), but shoehorning those bytes into integers efficiently is messy.
*/
/* -------------------------------------------------------------------- */

static const union _dbswap {
    const uint32_t ui;
    const unsigned char uc[4];
} endian = { 0x11223344 };
# define HASH_LITTLE_ENDIAN	(endian.uc[0] == (unsigned char) 0x44)
# define HASH_BIG_ENDIAN	(endian.uc[0] == (unsigned char) 0x11)

#ifndef ROTL32
# define ROTL32(x, s) (((x) << (s)) | ((x) >> (32 - (s))))
#endif

/* NOTE: The _size parameter should be in bytes. */
#define	_JLU3_INIT(_h, _size)	(0xdeadbeef + ((uint32_t)(_size)) + (_h))

/* -------------------------------------------------------------------- */
/*
 * _JLU3_MIX -- mix 3 32-bit values reversibly.
 * 
 * This is reversible, so any information in (a,b,c) before _JLU3_MIX() is
 * still in (a,b,c) after _JLU3_MIX().
 * 
 * If four pairs of (a,b,c) inputs are run through _JLU3_MIX(), or through
 * _JLU3_MIX() in reverse, there are at least 32 bits of the output that
 * are sometimes the same for one pair and different for another pair.
 * This was tested for:
 * * pairs that differed by one bit, by two bits, in any combination
 *   of top bits of (a,b,c), or in any combination of bottom bits of
 *   (a,b,c).
 * * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 *   the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 *   is commonly produced by subtraction) look like a single 1-bit
 *   difference.
 * * the base values were pseudorandom, all zero but one bit set, or 
 *   all zero plus a counter that starts at zero.
 * 
 * Some k values for my "a-=c; a^=ROTL32(c,k); c+=b;" arrangement that
 * satisfy this are
 *     4  6  8 16 19  4
 *     9 15  3 18 27 15
 *    14  9  3  7 17  3
 * Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
 * for "differ" defined as + with a one-bit base and a two-bit delta.  I
 * used http://burtleburtle.net/bob/hash/avalanche.html to choose 
 * the operations, constants, and arrangements of the variables.
 * 
 * This does not achieve avalanche.  There are input bits of (a,b,c)
 * that fail to affect some output bits of (a,b,c), especially of a.  The
 * most thoroughly mixed value is c, but it doesn't really even achieve
 * avalanche in c.
 * 
 * This allows some parallelism.  Read-after-writes are good at doubling
 * the number of bits affected, so the goal of mixing pulls in the opposite
 * direction as the goal of parallelism.  I did what I could.  Rotates
 * seem to cost as much as shifts on every machine I could lay my hands
 * on, and rotates are much kinder to the top and bottom bits, so I used
 * rotates.
 */
/* -------------------------------------------------------------------- */
#define _JLU3_MIX(a,b,c) \
{ \
  a -= c;  a ^= ROTL32(c, 4);  c += b; \
  b -= a;  b ^= ROTL32(a, 6);  a += c; \
  c -= b;  c ^= ROTL32(b, 8);  b += a; \
  a -= c;  a ^= ROTL32(c,16);  c += b; \
  b -= a;  b ^= ROTL32(a,19);  a += c; \
  c -= b;  c ^= ROTL32(b, 4);  b += a; \
}

/* -------------------------------------------------------------------- */
/**
 * _JLU3_FINAL -- final mixing of 3 32-bit values (a,b,c) into c
 * 
 * Pairs of (a,b,c) values differing in only a few bits will usually
 * produce values of c that look totally different.  This was tested for
 * * pairs that differed by one bit, by two bits, in any combination
 *   of top bits of (a,b,c), or in any combination of bottom bits of
 *   (a,b,c).
 * * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 *   the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 *   is commonly produced by subtraction) look like a single 1-bit
 *   difference.
 * * the base values were pseudorandom, all zero but one bit set, or 
 *   all zero plus a counter that starts at zero.
 * 
 * These constants passed:
 *  14 11 25 16 4 14 24
 *  12 14 25 16 4 14 24
 * and these came close:
 *   4  8 15 26 3 22 24
 *  10  8 15 26 3 22 24
 *  11  8 15 26 3 22 24
 */
/* -------------------------------------------------------------------- */
#define _JLU3_FINAL(a,b,c) \
{ \
  c ^= b; c -= ROTL32(b,14); \
  a ^= c; a -= ROTL32(c,11); \
  b ^= a; b -= ROTL32(a,25); \
  c ^= b; c -= ROTL32(b,16); \
  a ^= c; a -= ROTL32(c,4);  \
  b ^= a; b -= ROTL32(a,14); \
  c ^= b; c -= ROTL32(b,24); \
}

#if defined(_JLU3_jlu32lpair)
/**
 * jlu32lpair: return 2 32-bit hash values.
 *
 * This is identical to jlu32l(), except it returns two 32-bit hash
 * values instead of just one.  This is good enough for hash table
 * lookup with 2^^64 buckets, or if you want a second hash if you're not
 * happy with the first, or if you want a probably-unique 64-bit ID for
 * the key.  *pc is better mixed than *pb, so use *pc first.  If you want
 * a 64-bit value do something like "*pc + (((uint64_t)*pb)<<32)".
 *
 * @param h		the previous hash, or an arbitrary value
 * @param *key		the key, an array of uint8_t values
 * @param size		the size of the key in bytes
 * @retval *pc,		IN: primary initval, OUT: primary hash
 * *retval *pb		IN: secondary initval, OUT: secondary hash
 */
void jlu32lpair(const void *key, size_t size, uint32_t *pc, uint32_t *pb)
{
    union { const void *ptr; size_t i; } u;
    uint32_t a = _JLU3_INIT(*pc, size);
    uint32_t b = a;
    uint32_t c = a;

    if (key == NULL)
	goto exit;

    c += *pb;	/* Add the secondary hash. */

    u.ptr = key;
    if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
	const uint32_t *k = (const uint32_t *)key;	/* read 32-bit chunks */

	/*-- all but last block: aligned reads and affect 32 bits of (a,b,c) */
	while (size > (size_t)12) {
	    a += k[0];
	    b += k[1];
	    c += k[2];
	    _JLU3_MIX(a,b,c);
	    size -= 12;
	    k += 3;
	}
	/*------------------------- handle the last (probably partial) block */
	/* 
	 * "k[2]&0xffffff" actually reads beyond the end of the string, but
	 * then masks off the part it's not allowed to read.  Because the
	 * string is aligned, the masked-off tail is in the same word as the
	 * rest of the string.  Every machine with memory protection I've seen
	 * does it on word boundaries, so is OK with this.  But VALGRIND will
	 * still catch it and complain.  The masking trick does make the hash
	 * noticably faster for short strings (like English words).
	 */
	switch (size) {
	case 12:	c += k[2]; b+=k[1]; a+=k[0]; break;
	case 11:	c += k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
	case 10:	c += k[2]&0xffff; b+=k[1]; a+=k[0]; break;
	case  9:	c += k[2]&0xff; b+=k[1]; a+=k[0]; break;
	case  8:	b += k[1]; a+=k[0]; break;
	case  7:	b += k[1]&0xffffff; a+=k[0]; break;
	case  6:	b += k[1]&0xffff; a+=k[0]; break;
	case  5:	b += k[1]&0xff; a+=k[0]; break;
	case  4:	a += k[0]; break;
	case  3:	a += k[0]&0xffffff; break;
	case  2:	a += k[0]&0xffff; break;
	case  1:	a += k[0]&0xff; break;
	case  0:	goto exit;
	}

    } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
	const uint16_t *k = (const uint16_t *)key;	/* read 16-bit chunks */
	const uint8_t  *k8;

	/*----------- all but last block: aligned reads and different mixing */
	while (size > (size_t)12) {
	    a += k[0] + (((uint32_t)k[1])<<16);
	    b += k[2] + (((uint32_t)k[3])<<16);
	    c += k[4] + (((uint32_t)k[5])<<16);
	    _JLU3_MIX(a,b,c);
	    size -= 12;
	    k += 6;
	}

	/*------------------------- handle the last (probably partial) block */
	k8 = (const uint8_t *)k;
	switch (size) {
	case 12:
	    c += k[4]+(((uint32_t)k[5])<<16);
	    b += k[2]+(((uint32_t)k[3])<<16);
	    a += k[0]+(((uint32_t)k[1])<<16);
	    break;
	case 11:
	    c += ((uint32_t)k8[10])<<16;
	case 10:
	    c += k[4];
	    b += k[2]+(((uint32_t)k[3])<<16);
	    a += k[0]+(((uint32_t)k[1])<<16);
	    break;
	case  9:
	    c += k8[8];
	case  8:
	    b += k[2]+(((uint32_t)k[3])<<16);
	    a += k[0]+(((uint32_t)k[1])<<16);
	    break;
	case  7:
	    b += ((uint32_t)k8[6])<<16;
	case  6:
	    b += k[2];
	    a += k[0]+(((uint32_t)k[1])<<16);
	    break;
	case  5:
	    b += k8[4];
	case  4:
	    a += k[0]+(((uint32_t)k[1])<<16);
	    break;
	case  3:
	    a += ((uint32_t)k8[2])<<16;
	case  2:
	    a += k[0];
	    break;
	case  1:
	    a += k8[0];
	    break;
	case  0:
	    goto exit;
	}

    } else {		/* need to read the key one byte at a time */
	const uint8_t *k = (const uint8_t *)key;

	/*----------- all but the last block: affect some 32 bits of (a,b,c) */
	while (size > (size_t)12) {
	    a += k[0];
	    a += ((uint32_t)k[1])<<8;
	    a += ((uint32_t)k[2])<<16;
	    a += ((uint32_t)k[3])<<24;
	    b += k[4];
	    b += ((uint32_t)k[5])<<8;
	    b += ((uint32_t)k[6])<<16;
	    b += ((uint32_t)k[7])<<24;
	    c += k[8];
	    c += ((uint32_t)k[9])<<8;
	    c += ((uint32_t)k[10])<<16;
	    c += ((uint32_t)k[11])<<24;
	    _JLU3_MIX(a,b,c);
	    size -= 12;
	    k += 12;
	}

	/*---------------------------- last block: affect all 32 bits of (c) */
	switch (size) {
	case 12:	c += ((uint32_t)k[11])<<24;
	case 11:	c += ((uint32_t)k[10])<<16;
	case 10:	c += ((uint32_t)k[9])<<8;
	case  9:	c += k[8];
	case  8:	b += ((uint32_t)k[7])<<24;
	case  7:	b += ((uint32_t)k[6])<<16;
	case  6:	b += ((uint32_t)k[5])<<8;
	case  5:	b += k[4];
	case  4:	a += ((uint32_t)k[3])<<24;
	case  3:	a += ((uint32_t)k[2])<<16;
	case  2:	a += ((uint32_t)k[1])<<8;
	case  1:	a += k[0];
	    break;
	case  0:
	    goto exit;
	}
    }

    _JLU3_FINAL(a,b,c);

exit:
    *pc = c;
    *pb = b;
    return;
}
#endif	/* defined(_JLU3_jlu32lpair) */

/* End of section from lookup3.c
*/

static const unsigned char utf8_skip_data[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

const char *
POPT_prev_char (const char *str)
{
    const char *p = str;

    while (1) {
	p--;
	if (((unsigned)*p & 0xc0) != (unsigned)0x80)
	    return p;
    }
}

const char *
POPT_next_char (const char *str)
{
    const char *p = str;

    while (*p != '\0') {
	p++;
	if (((unsigned)*p & 0xc0) != (unsigned)0x80)
	    break;
    }
    return p;
}

#if !defined(POPT_fprintf)	/* XXX lose all the goop ... */

int
POPT_fprintf (FILE * stream, const char * format, ...)
{
    char * b = NULL;
    int rc;
    va_list ap;

    size_t nb = (size_t)1;

    /* HACK: add +1 to the realloc no. of bytes "just in case". */
    /* XXX Likely unneeded, the issues wrto vsnprintf(3) return b0rkage have
     * to do with whether the final '\0' is counted (or not). The code
     * below already adds +1 for the (possibly already counted) trailing NUL.
     */
    while ((b = (char*)realloc(b, nb+1)) != NULL) {
	va_start(ap, format);
	rc = vsnprintf(b, nb, format, ap);
	va_end(ap);
	if (rc > -1) {	/* glibc 2.1 */
	    if ((size_t)rc < nb)
		break;
	    nb = (size_t)(rc + 1);	/* precise buffer length known */
	} else 		/* glibc 2.0 */
	    nb += (nb < (size_t)100 ? (size_t)100 : nb);
    }

    rc = 0;
    if (b != NULL) {
	    rc = fprintf(stream, "%s", b);
	free (b);
    }

    return rc;
}

#endif	/* !defined(POPT_fprintf) */

/* End of section from poptint.cpp
*/

/* This section is from poptparse.cpp
*/

#define POPT_ARGV_ARRAY_GROW_DELTA 5

int poptDupArgv(int argc, const char **argv,
		int * argcPtr, const char *** argvPtr)
{
    size_t nb = (argc + 1) * sizeof(*argv);
    const char ** argv2;
    char * dst;
    int i;

    if (argc <= 0 || argv == NULL)	/* XXX can't happen */
	return POPT_ERROR_NOARG;
    for (i = 0; i < argc; i++) {
	if (argv[i] == NULL)
	    return POPT_ERROR_NOARG;
	nb += strlen(argv[i]) + 1;
    }
	
    dst = (char*)malloc(nb);
    if (dst == NULL)			/* XXX can't happen */
	return POPT_ERROR_MALLOC;
    argv2 = (const char **) dst;
    dst += (argc + 1) * sizeof(*argv);
    *dst = '\0';

    for (i = 0; i < argc; i++) {
	argv2[i] = dst;
	dst = stpcpy(dst, argv[i]);
	dst++;	/* trailing NUL */
    }
    argv2[argc] = NULL;

    if (argvPtr) {
	*argvPtr = argv2;
    } else {
	free(argv2);
	argv2 = NULL;
    }
    if (argcPtr)
	*argcPtr = argc;
    return 0;
}

int poptParseArgvString(const char * s, int * argcPtr, const char *** argvPtr)
{
    const char * src;
    char quote = '\0';
    int argvAlloced = POPT_ARGV_ARRAY_GROW_DELTA;
    const char ** argv = (const char**)malloc(sizeof(*argv) * argvAlloced);
    int argc = 0;
    size_t buflen = strlen(s) + 1;
    char * buf, * bufOrig = NULL;
    int rc = POPT_ERROR_MALLOC;

    if (argv == NULL) return rc;
    buf = bufOrig = (char*)calloc((size_t)1, buflen);
    if (buf == NULL) {
	free(argv);
	return rc;
    }
    argv[argc] = buf;

    for (src = s; *src != '\0'; src++) {
	if (quote == *src) {
	    quote = '\0';
	} else if (quote != '\0') {
	    if (*src == '\\') {
		src++;
		if (!*src) {
		    rc = POPT_ERROR_BADQUOTE;
		    goto exit;
		}
		if (*src != quote) *buf++ = '\\';
	    }
	    *buf++ = *src;
	} else if (_isspaceptr(src)) {
	    if (*argv[argc] != '\0') {
		buf++, argc++;
		if (argc == argvAlloced) {
		    argvAlloced += POPT_ARGV_ARRAY_GROW_DELTA;
		    argv = (const char**)realloc(argv, sizeof(*argv) * argvAlloced);
		    if (argv == NULL) goto exit;
		}
		argv[argc] = buf;
	    }
	} else switch (*src) {
	  case '"':
	  case '\'':
	    quote = *src;
	    break;
	  case '\\':
	    src++;
	    if (!*src) {
		rc = POPT_ERROR_BADQUOTE;
		goto exit;
	    }
	  default:
	    *buf++ = *src;
	    break;
	}
    }

    if (strlen(argv[argc])) {
	argc++, buf++;
    }

    rc = poptDupArgv(argc, argv, argcPtr, argvPtr);

exit:
    if (bufOrig) free(bufOrig);
    if (argv) free(argv);
    return rc;
}

/* still in the dev stage.
 * return values, perhaps 1== file erro
 * 2== line to long
 * 3== umm.... more?
 */
int poptConfigFileToString(FILE *fp, char ** argstrp,
		UNUSED(int flags))
{
    char line[999];
    char * argstr;
    char * p;
    char * q;
    char * x;
    size_t t;
    size_t argvlen = 0;
    size_t maxlinelen = sizeof(line);
    size_t linelen;
    size_t maxargvlen = (size_t)480;

    *argstrp = NULL;

    /*   |   this_is   =   our_line
     *	     p             q      x
     */

    if (fp == NULL)
	return POPT_ERROR_NULLARG;

    argstr = (char*)calloc(maxargvlen, sizeof(*argstr));
    if (argstr == NULL) return POPT_ERROR_MALLOC;

    while (fgets(line, (int)maxlinelen, fp) != NULL) {
	p = line;

	/* loop until first non-space char or EOL */
	while( *p != '\0' && _isspaceptr(p) )
	    p++;

	linelen = strlen(p);
	if (linelen >= maxlinelen-1) {
	    free(argstr);
	    return POPT_ERROR_OVERFLOW;	/* XXX line too long */
	}

	if (*p == '\0' || *p == '\n') continue;	/* line is empty */
	if (*p == '#') continue;		/* comment line */

	q = p;

	while (*q != '\0' && (!_isspaceptr(q)) && *q != '=')
	    q++;

	if (_isspaceptr(q)) {
	    /* a space after the name, find next non space */
	    *q++='\0';
	    while( *q != '\0' && _isspaceptr(q) ) q++;
	}
	if (*q == '\0') {
	    /* single command line option (ie, no name=val, just name) */
	    q[-1] = '\0';		/* kill off newline from fgets() call */
	    argvlen += (t = (size_t)(q - p)) + (sizeof(" --")-1);
	    if (argvlen >= maxargvlen) {
		maxargvlen = (t > maxargvlen) ? t*2 : maxargvlen*2;
		argstr = (char*)realloc(argstr, maxargvlen);
		if (argstr == NULL) return POPT_ERROR_MALLOC;
	    }
	    strcat(argstr, " --");
	    strcat(argstr, p);
	    continue;
	}
	if (*q != '=')
	    continue;	/* XXX for now, silently ignore bogus line */
		
	/* *q is an equal sign. */
	*q++ = '\0';

	/* find next non-space letter of value */
	while (*q != '\0' && _isspaceptr(q))
	    q++;
	if (*q == '\0')
	    continue;	/* XXX silently ignore missing value */

	/* now, loop and strip all ending whitespace */
	x = p + linelen;
	while (_isspaceptr(--x))
	    *x = '\0';	/* null out last char if space (including fgets() NL) */

	/* rest of line accept */
	t = (size_t)(x - p);
	argvlen += t + (sizeof("' --='")-1);
	if (argvlen >= maxargvlen) {
	    maxargvlen = (t > maxargvlen) ? t*2 : maxargvlen*2;
	    argstr = (char*)realloc(argstr, maxargvlen);
	    if (argstr == NULL) return POPT_ERROR_MALLOC;
	}
	strcat(argstr, " --");
	strcat(argstr, p);
	strcat(argstr, "=\"");
	strcat(argstr, q);
	strcat(argstr, "\"");
    }

    *argstrp = argstr;
    return 0;
}

/* End of section from poptparse.cpp
*/

/* The rest of the file is from popt.cpp
*/

#ifdef	MYDEBUG
int _popt_debug = 0;
#endif

unsigned int _poptArgMask = POPT_ARG_MASK;
unsigned int _poptGroupMask = POPT_GROUP_MASK;

void poptSetExecPath(poptContext con, const char * path, int allowAbsolute)
{
    con->execPath = (const char*)_free(con->execPath);
    con->execPath = xstrdup(path);
    con->execAbsolute = allowAbsolute;
    return;
}

static void invokeCallbacksPRE(poptContext con, const struct poptOption * opt)
{
    if (opt != NULL)
    for (; opt->longName || opt->shortName || opt->arg; opt++) {
	poptArg arg;
	arg.ptr = opt->arg;
	if (arg.ptr)
	switch (poptArgType(opt)) {
	case POPT_ARG_INCLUDE_TABLE:	/* Recurse on included sub-tables. */
	    poptSubstituteHelpI18N(arg.opt);	/* XXX side effects */
	    invokeCallbacksPRE(con, (const poptOption*)arg.ptr);
	    break;
	case POPT_ARG_CALLBACK:		/* Perform callback. */
	    if (!CBF_ISSET(opt, PRE))
		break;
	    arg.cb(con, POPT_CALLBACK_REASON_PRE, NULL, NULL, opt->descrip);
	    break;
	}
    }
}

static void invokeCallbacksPOST(poptContext con, const struct poptOption * opt)
{
    if (opt != NULL)
    for (; opt->longName || opt->shortName || opt->arg; opt++) {
	poptArg arg;
	arg.ptr = opt->arg;
	if (arg.ptr)
	switch (poptArgType(opt)) {
	case POPT_ARG_INCLUDE_TABLE:	/* Recurse on included sub-tables. */
	    poptSubstituteHelpI18N(arg.opt);	/* XXX side effects */
	    invokeCallbacksPOST(con, (const poptOption*)arg.ptr);
	    break;
	case POPT_ARG_CALLBACK:		/* Perform callback. */
	    if (!CBF_ISSET(opt, POST))
		break;
	    arg.cb(con, POPT_CALLBACK_REASON_POST, NULL, NULL, opt->descrip);
	    break;
	}
    }
}

static void invokeCallbacksOPTION(poptContext con,
				const struct poptOption * opt,
				const struct poptOption * myOpt,
				const void * myData, int shorty)
{
    const struct poptOption * cbopt = NULL;
    poptArg cbarg;
	cbarg.ptr = NULL;

    if (opt != NULL)
    for (; opt->longName || opt->shortName || opt->arg; opt++) {
	poptArg arg;
	arg.ptr = opt->arg;
	switch (poptArgType(opt)) {
	case POPT_ARG_INCLUDE_TABLE:	/* Recurse on included sub-tables. */
	    poptSubstituteHelpI18N(arg.opt);	/* XXX side effects */
	    if (opt->arg != NULL)
		invokeCallbacksOPTION(con, (const poptOption*)opt->arg, myOpt, myData, shorty);
	    break;
	case POPT_ARG_CALLBACK:		/* Save callback info. */
	    if (CBF_ISSET(opt, SKIPOPTION))
		break;
	    cbopt = opt;
	    cbarg.ptr = opt->arg;
	    break;
	default:		/* Perform callback on matching option. */
	    if (cbopt == NULL || cbarg.cb == NULL)
		break;
	    if ((myOpt->shortName && opt->shortName && shorty &&
			myOpt->shortName == opt->shortName)
	     || (myOpt->longName != NULL && opt->longName != NULL &&
			!strcmp(myOpt->longName, opt->longName)))
	    {	const void *cbData = (cbopt->descrip ? cbopt->descrip : myData);
		cbarg.cb(con, POPT_CALLBACK_REASON_OPTION,
			myOpt, con->os->nextArg, cbData);
		/* Terminate (unless explcitly continuing). */
		if (!CBF_ISSET(cbopt, CONTINUE))
		    return;
	    }
	    break;
	}
    }
}

poptContext poptGetContext(const char * name, int argc, const char ** argv,
			const struct poptOption * options, unsigned int flags)
{
    poptContext con = (poptContext)malloc(sizeof(*con));

    if (con == NULL) return NULL;	/* XXX can't happen */
    memset(con, 0, sizeof(*con));

    con->os = con->optionStack;
    con->os->argc = argc;
    con->os->argv = argv;
    con->os->argb = NULL;

    if (!(flags & POPT_CONTEXT_KEEP_FIRST))
	con->os->next = 1;		/* skip argv[0] */

    con->leftovers = (poptArgv)calloc( (size_t)(argc + 1), sizeof(*con->leftovers) );
    con->options = options;
    con->aliases = NULL;
    con->numAliases = 0;
    con->flags = flags;
    con->execs = NULL;
    con->numExecs = 0;
    con->finalArgvAlloced = argc * 2;
    con->finalArgv = (poptArgv)calloc( (size_t)con->finalArgvAlloced, sizeof(*con->finalArgv) );
    con->execAbsolute = 1;
    con->arg_strip = NULL;

    if (getenv("POSIXLY_CORRECT") || getenv("POSIX_ME_HARDER"))
	con->flags |= POPT_CONTEXT_POSIXMEHARDER;

    if (name)
	con->appName = xstrdup(name);

    invokeCallbacksPRE(con, con->options);

    return con;
}

static void cleanOSE(struct optionStackEntry *os)
{
    os->nextArg = (char*)_free(os->nextArg);
    os->argv = (poptArgv)_free(os->argv);
    os->argb = (pbm_set*)PBM_FREE(os->argb);
}

void poptResetContext(poptContext con)
{
    int i;

    if (con == NULL) return;
    while (con->os > con->optionStack) {
	cleanOSE(con->os--);
    }
    con->os->argb = (pbm_set*)PBM_FREE(con->os->argb);
    con->os->currAlias = NULL;
    con->os->nextCharArg = NULL;
    con->os->nextArg = NULL;
    con->os->next = 1;			/* skip argv[0] */

    con->numLeftovers = 0;
    con->nextLeftover = 0;
    con->restLeftover = 0;
    con->doExec = NULL;

    if (con->finalArgv != NULL)
    for (i = 0; i < con->finalArgvCount; i++) {
	con->finalArgv[i] = (poptString)_free(con->finalArgv[i]);
    }

    con->finalArgvCount = 0;
    con->arg_strip = (pbm_set*)PBM_FREE(con->arg_strip);
    return;
}

/* Only one of longName, shortName should be set, not both. */
static int handleExec(poptContext con,
		const char * longName, char shortName)
{
    poptItem item;
    int i;

    if (con->execs == NULL || con->numExecs <= 0) /* XXX can't happen */
	return 0;

    for (i = con->numExecs - 1; i >= 0; i--) {
	item = con->execs + i;
	if (longName && !(item->option.longName &&
			!strcmp(longName, item->option.longName)))
	    continue;
	else if (shortName != item->option.shortName)
	    continue;
	break;
    }
    if (i < 0) return 0;


    if (con->flags & POPT_CONTEXT_NO_EXEC)
	return 1;

    if (con->doExec == NULL) {
	con->doExec = con->execs + i;
	return 1;
    }

    /* We already have an exec to do; remember this option for next
       time 'round */
    if ((con->finalArgvCount + 1) >= (con->finalArgvAlloced)) {
	con->finalArgvAlloced += 10;
	con->finalArgv = (poptArgv)realloc(con->finalArgv,
			sizeof(*con->finalArgv) * con->finalArgvAlloced);
    }

    i = con->finalArgvCount++;
    if (con->finalArgv != NULL)	/* XXX can't happen */
    {	char *s  = (char*)malloc((longName ? strlen(longName) : 0) + sizeof("--"));
	if (s != NULL) {	/* XXX can't happen */
	    con->finalArgv[i] = s;
	    *s++ = '-';
	    if (longName)
		s = stpcpy( stpcpy(s, "-"), longName);
	    else
		*s++ = shortName;
	    *s = '\0';
	} else
	    con->finalArgv[i] = NULL;
    }

    return 1;
}

/**
 * Compare long option for equality, adjusting for POPT_ARGFLAG_TOGGLE.
 * @param opt           option
 * @param longName	arg option
 * @param longNameLen	arg option length
 * @return		does long option match?
 */
static int
longOptionStrcmp(const struct poptOption * opt,
		const char * longName, size_t longNameLen)
{
    const char * optLongName = opt->longName;
    int rc;

    if (optLongName == NULL || longName == NULL)	/* XXX can't heppen */
	return 0;

    if (F_ISSET(opt, TOGGLE)) {
	if (optLongName[0] == 'n' && optLongName[1] == 'o') {
	    optLongName += sizeof("no") - 1;
	    if (optLongName[0] == '-')
		optLongName++;
	}
	if (longName[0] == 'n' && longName[1] == 'o') {
	    longName += sizeof("no") - 1;
	    longNameLen -= sizeof("no") - 1;
	    if (longName[0] == '-') {
		longName++;
		longNameLen--;
	    }
	}
    }
    rc = (int)(strlen(optLongName) == longNameLen);
    if (rc)
	rc = (int)(strncmp(optLongName, longName, longNameLen) == 0);
    return rc;
}

/* Only one of longName, shortName may be set at a time */
static int handleAlias(poptContext con,
		const char * longName, size_t longNameLen,
		char shortName,
		const char * nextArg)
{
    poptItem item = con->os->currAlias;
    int rc;
    int i;

    if (item) {
	if (longName && item->option.longName != NULL
	 && longOptionStrcmp(&item->option, longName, longNameLen))
	    return 0;
	else
	if (shortName && shortName == item->option.shortName)
	    return 0;
    }

    if (con->aliases == NULL || con->numAliases <= 0) /* XXX can't happen */
	return 0;

    for (i = con->numAliases - 1; i >= 0; i--) {
	item = con->aliases + i;
	if (longName) {
	    if (item->option.longName == NULL)
		continue;
	    if (!longOptionStrcmp(&item->option, longName, longNameLen))
		continue;
	} else if (shortName != item->option.shortName)
	    continue;
	break;
    }
    if (i < 0) return 0;

    if ((con->os - con->optionStack + 1) == POPT_OPTION_DEPTH)
	return POPT_ERROR_OPTSTOODEEP;

    if (longName == NULL && nextArg != NULL && *nextArg != '\0')
	con->os->nextCharArg = nextArg;

    con->os++;
    con->os->next = 0;
    con->os->stuffed = 0;
    con->os->nextArg = NULL;
    con->os->nextCharArg = NULL;
    con->os->currAlias = con->aliases + i;
    {	const char ** av;
	int ac = con->os->currAlias->argc;
	/* Append --foo=bar arg to alias argv array (if present). */ 
	if (longName && nextArg != NULL && *nextArg != '\0') {
	    av = (const char**)malloc((ac + 1 + 1) * sizeof(*av));
	    if (av != NULL) {	/* XXX won't happen. */
		for (i = 0; i < ac; i++) {
		    av[i] = con->os->currAlias->argv[i];
		}
		av[ac++] = nextArg;
		av[ac] = NULL;
	    } else	/* XXX revert to old popt behavior if malloc fails. */
		av = con->os->currAlias->argv;
	} else
	    av = con->os->currAlias->argv;
	rc = poptDupArgv(ac, av, &con->os->argc, &con->os->argv);
	if (av != NULL && av != con->os->currAlias->argv)
	    free(av);
    }
    con->os->argb = NULL;

    return (rc ? rc : 1);
}

/**
 * Return absolute path to executable by searching PATH.
 * @param argv0		name of executable
 * @return		(malloc'd) absolute path to executable (or NULL)
 */
static
const char * findProgramPath(const char * argv0)
{
    char *path = NULL, *s = NULL, *se;
    char *t = NULL;

    if (argv0 == NULL) return NULL;	/* XXX can't happen */

    /* If there is a / in argv[0], it has to be an absolute path. */
    /* XXX Hmmm, why not if (argv0[0] == '/') ... instead? */
    if (strchr(argv0, '/'))
	return xstrdup(argv0);

    if ((path = getenv("PATH")) == NULL || (path = xstrdup(path)) == NULL)
	return NULL;

    /* The return buffer in t is big enough for any path. */
    if ((t = (char*)malloc(strlen(path) + strlen(argv0) + sizeof("/"))) != NULL)
    for (s = path; s && *s; s = se) {

	/* Snip PATH element into [s,se). */
	if ((se = strchr(s, ':')))
	    *se++ = '\0';

	/* Append argv0 to PATH element. */
	(void) stpcpy(stpcpy(stpcpy(t, s), "/"), argv0);

	/* If file is executable, bingo! */
	if (!access(t, X_OK))
	    break;
    }

    /* If no executable was found in PATH, return NULL. */
    if (!(s && *s) && t != NULL)
	t = (char*)_free(t);
    path = (char*)_free(path);

    return t;
}

static int execCommand(poptContext con)
{
    poptItem item = con->doExec;
    poptArgv argv = NULL;
    int argc = 0;
    int ec = POPT_ERROR_ERRNO;

    if (item == NULL) /*XXX can't happen*/
	return POPT_ERROR_NOARG;

    if (item->argv == NULL || item->argc < 1 ||
	(!con->execAbsolute && strchr(item->argv[0], '/')))
	    return POPT_ERROR_NOARG;

    argv = (poptArgv)malloc(sizeof(*argv) *
			(6 + item->argc + con->numLeftovers + con->finalArgvCount));
    if (argv == NULL) return POPT_ERROR_MALLOC;

    if (!strchr(item->argv[0], '/') && con->execPath != NULL) {
	char *s = (char *)malloc(strlen(con->execPath) + strlen(item->argv[0]) + sizeof("/"));
	if (s)
	    (void)stpcpy(stpcpy(stpcpy(s, con->execPath), "/"), item->argv[0]);

	argv[argc] = s;
    } else
	argv[argc] = findProgramPath(item->argv[0]);
    if (argv[argc++] == NULL) {
	ec = POPT_ERROR_NOARG;
	goto exit;
    }

    if (item->argc > 1) {
	memcpy(argv + argc, item->argv + 1, sizeof(*argv) * (item->argc - 1));
	argc += (item->argc - 1);
    }

    if (con->finalArgv != NULL && con->finalArgvCount > 0) {
	memcpy(argv + argc, con->finalArgv,
		sizeof(*argv) * con->finalArgvCount);
	argc += con->finalArgvCount;
    }

    if (con->leftovers != NULL && con->numLeftovers > 0) {
	memcpy(argv + argc, con->leftovers, sizeof(*argv) * con->numLeftovers);
	argc += con->numLeftovers;
    }

    argv[argc] = NULL;

    execvp(argv[0], (char *const *)argv);

exit:
    if (argv) {
        if (argv[0])
            free((void *)argv[0]);
        free(argv);
    }
    return ec;
}

static const struct poptOption *
findOption(const struct poptOption * opt,
		const char * longName, size_t longNameLen,
		char shortName,
		poptCallbackType * callback,
		const void ** callbackData,
		unsigned int argInfo)
{
    const struct poptOption * cb = NULL;
    poptArg cbarg;
	cbarg.ptr = NULL;

    /* This happens when a single - is given */
    if (LF_ISSET(ONEDASH) && !shortName && (longName && *longName == '\0'))
	shortName = '-';

    for (; opt->longName || opt->shortName || opt->arg; opt++) {
	poptArg arg;
	arg.ptr = opt->arg;

	switch (poptArgType(opt)) {
	case POPT_ARG_INCLUDE_TABLE:	/* Recurse on included sub-tables. */
	{   const struct poptOption * opt2;

	    poptSubstituteHelpI18N(arg.opt);	/* XXX side effects */
	    if (arg.ptr == NULL) continue;	/* XXX program error */
//	    opt2 = findOption(arg.opt, longName, longNameLen, shortName, callback,
//			      callbackData, argInfo);
	    opt2 = findOption((const struct poptOption*)arg.ptr, longName, longNameLen, shortName, callback,
			      callbackData, argInfo);
	    if (opt2 == NULL) continue;
	    /* Sub-table data will be inheirited if no data yet. */
	    if (callback && *callback
	     && callbackData && *callbackData == NULL)
		*callbackData = opt->descrip;
	    return opt2;
	}   break;
	case POPT_ARG_CALLBACK:
	    cb = opt;
	    cbarg.ptr = opt->arg;
	    continue;
	    break;
	default:
	    break;
	}

	if (longName != NULL && opt->longName != NULL &&
		   (!LF_ISSET(ONEDASH) || F_ISSET(opt, ONEDASH)) &&
		   longOptionStrcmp(opt, longName, longNameLen))
	{
	    break;
	} else if (shortName && shortName == opt->shortName) {
	    break;
	}
    }

    if (opt->longName == NULL && !opt->shortName)
	return NULL;

    if (callback)
	*callback = (cb ? cbarg.cb : NULL);
    if (callbackData)
	*callbackData = (cb && !CBF_ISSET(cb, INC_DATA) ? cb->descrip : NULL);

    return opt;
}

static const char * findNextArg(poptContext con,
		unsigned argx, int delete_arg)
{
    struct optionStackEntry * os = con->os;
    const char * arg;

    do {
	int i;
	arg = NULL;
	while (os->next == os->argc && os > con->optionStack) os--;
	if (os->next == os->argc && os == con->optionStack) break;
	if (os->argv != NULL)
	for (i = os->next; i < os->argc; i++) {
	    if (os->argb && PBM_ISSET(i, os->argb))
		continue;
	    if (*os->argv[i] == '-')
		continue;
	    if (--argx > 0)
		continue;
	    arg = os->argv[i];
	    if (delete_arg) {
		if (os->argb == NULL) os->argb = (pbm_set*)PBM_ALLOC(os->argc);
		if (os->argb != NULL)	/* XXX can't happen */
		    PBM_SET(i, os->argb);
	    }
	    break;
	}
	if (os > con->optionStack) os--;
    } while (arg == NULL);
    return arg;
}

static const char *
expandNextArg(poptContext con, const char * s)
{
    const char * a = NULL;
    char *t, *te;
    size_t tn = strlen(s) + 1;
    char c;

    te = t = (char*)malloc(tn);
    if (t == NULL) return NULL;		/* XXX can't happen */
    *t = '\0';
    while ((c = *s++) != '\0') {
	switch (c) {
	case '!':
	    if (!(s[0] == '#' && s[1] == ':' && s[2] == '+'))
		break;
	    /* XXX Make sure that findNextArg deletes only next arg. */
	    if (a == NULL) {
		if ((a = findNextArg(con, 1U, 1)) == NULL)
		    break;
	    }
	    s += sizeof("#:+") - 1;

	    tn += strlen(a);
	    {   size_t pos = (size_t) (te - t);
		if ((t = (char*)realloc(t, tn)) == NULL)	/* XXX can't happen */
		    return NULL;
		te = stpcpy(t + pos, a);
	    }
	    continue;
	    break;
	default:
	    break;
	}
	*te++ = c;
    }
    *te++ = '\0';
    /* If the new string is longer than needed, shorten. */
    if ((t + tn) > te) {
	if ((te = (char*)realloc(t, (size_t)(te - t))) == NULL)
	    free(t);
	t = te;
    }
    return t;
}

static void poptStripArg(poptContext con, int which)
{
    if (con->arg_strip == NULL)
	con->arg_strip = (pbm_set*)PBM_ALLOC(con->optionStack[0].argc);
    if (con->arg_strip != NULL)		/* XXX can't happen */
    PBM_SET(which, con->arg_strip);
    return;
}

unsigned int _poptBitsN = _POPT_BITS_N;
unsigned int _poptBitsM = _POPT_BITS_M;
unsigned int _poptBitsK = _POPT_BITS_K;

static int _poptBitsNew(poptBits *bitsp)
{
    if (bitsp == NULL)
	return POPT_ERROR_NULLARG;

    /* XXX handle negated initialization. */
    if (*bitsp == NULL) {
	if (_poptBitsN == 0) {
	    _poptBitsN = _POPT_BITS_N;
	    _poptBitsM = _POPT_BITS_M;
	}
	if (_poptBitsM == 0U) _poptBitsM = (3 * _poptBitsN) / 2;
	if (_poptBitsK == 0U || _poptBitsK > 32U) _poptBitsK = _POPT_BITS_K;
	*bitsp = (poptBits)PBM_ALLOC(_poptBitsM-1);
    }
    return 0;
}

int poptBitsAdd(poptBits bits, const char * s)
{
    size_t ns = (s ? strlen(s) : 0);
    uint32_t h0 = 0;
    uint32_t h1 = 0;

    if (bits == NULL || ns == 0)
	return POPT_ERROR_NULLARG;

    poptJlu32lpair(s, ns, &h0, &h1);

    for (ns = 0; ns < (size_t)_poptBitsK; ns++) {
        uint32_t h = h0 + (uint32_t)ns * h1;
        uint32_t ix = (h % _poptBitsM);
        PBM_SET(ix, bits);
    }
    return 0;
}

int poptBitsChk(poptBits bits, const char * s)
{
    size_t ns = (s ? strlen(s) : 0);
    uint32_t h0 = 0;
    uint32_t h1 = 0;
    int rc = 1;

    if (bits == NULL || ns == 0)
	return POPT_ERROR_NULLARG;

    poptJlu32lpair(s, ns, &h0, &h1);

    for (ns = 0; ns < (size_t)_poptBitsK; ns++) {
        uint32_t h = h0 + (uint32_t)ns * h1;
        uint32_t ix = (h % _poptBitsM);
        if (PBM_ISSET(ix, bits))
            continue;
        rc = 0;
        break;
    }
    return rc;
}

int poptBitsClr(poptBits bits)
{
    static size_t nbw = (__PBM_NBITS/8);
    size_t nw = (__PBM_IX(_poptBitsM-1) + 1);

    if (bits == NULL)
	return POPT_ERROR_NULLARG;
    memset(bits, 0, nw * nbw);
    return 0;
}

int poptBitsDel(poptBits bits, const char * s)
{
    size_t ns = (s ? strlen(s) : 0);
    uint32_t h0 = 0;
    uint32_t h1 = 0;

    if (bits == NULL || ns == 0)
	return POPT_ERROR_NULLARG;

    poptJlu32lpair(s, ns, &h0, &h1);

    for (ns = 0; ns < (size_t)_poptBitsK; ns++) {
        uint32_t h = h0 + (uint32_t)ns * h1;
        uint32_t ix = (h % _poptBitsM);
        PBM_CLR(ix, bits);
    }
    return 0;
}

int poptBitsIntersect(poptBits *ap, const poptBits b)
{
    __pbm_bits *abits;
    __pbm_bits *bbits;
    __pbm_bits rc = 0;
    size_t nw = (__PBM_IX(_poptBitsM-1) + 1);
    size_t i;

    if (ap == NULL || b == NULL || _poptBitsNew(ap))
	return POPT_ERROR_NULLARG;
    abits = __PBM_BITS(*ap);
    bbits = __PBM_BITS(b);

    for (i = 0; i < nw; i++) {
        abits[i] &= bbits[i];
	rc |= abits[i];
    }
    return (rc ? 1 : 0);
}

int poptBitsUnion(poptBits *ap, const poptBits b)
{
    __pbm_bits *abits;
    __pbm_bits *bbits;
    __pbm_bits rc = 0;
    size_t nw = (__PBM_IX(_poptBitsM-1) + 1);
    size_t i;

    if (ap == NULL || b == NULL || _poptBitsNew(ap))
	return POPT_ERROR_NULLARG;
    abits = __PBM_BITS(*ap);
    bbits = __PBM_BITS(b);

    for (i = 0; i < nw; i++) {
        abits[i] |= bbits[i];
	rc |= abits[i];
    }
    return (rc ? 1 : 0);
}

int poptBitsArgs(poptContext con, poptBits *ap)
{
    const char ** av;
    int rc = 0;

    if (con == NULL || ap == NULL || _poptBitsNew(ap) ||
	con->leftovers == NULL || con->numLeftovers == con->nextLeftover)
	return POPT_ERROR_NULLARG;

    /* some apps like [like RPM ;-) ] need this NULL terminated */
    con->leftovers[con->numLeftovers] = NULL;

    for (av = con->leftovers + con->nextLeftover; *av != NULL; av++) {
	if ((rc = poptBitsAdd(*ap, *av)) != 0)
	    break;
    }
    return rc;
}

int poptSaveBits(poptBits * bitsp,
		UNUSED(unsigned int argInfo), const char * s)
{
    char *tbuf = NULL;
    char *t, *te;
    int rc = 0;

    if (bitsp == NULL || s == NULL || *s == '\0' || _poptBitsNew(bitsp))
	return POPT_ERROR_NULLARG;

    /* Parse comma separated attributes. */
    te = tbuf = xstrdup(s);
    while ((t = te) != NULL && *t) {
	while (*te != '\0' && *te != ',')
	    te++;
	if (*te != '\0')
	    *te++ = '\0';
	/* XXX Ignore empty strings. */
	if (*t == '\0')
	    continue;
	/* XXX Permit negated attributes. caveat emptor: false negatives. */
	if (*t == '!') {
	    t++;
	    if ((rc = poptBitsChk(*bitsp, t)) > 0)
		rc = poptBitsDel(*bitsp, t);
	} else
	    rc = poptBitsAdd(*bitsp, t);
	if (rc)
	    break;
    }
    tbuf = (char*)_free(tbuf);
    return rc;
}

int poptSaveString(const char *** argvp,
		UNUSED(unsigned int argInfo), const char * val)
{
    int argc = 0;

    if (argvp == NULL || val == NULL)
	return POPT_ERROR_NULLARG;

    /* XXX likely needs an upper bound on argc. */
    if (*argvp != NULL)
    while ((*argvp)[argc] != NULL)
	argc++;
 
    if ((*argvp = (const char**)xrealloc(*argvp, (argc + 1 + 1) * sizeof(**argvp))) != NULL) {
	(*argvp)[argc++] = xstrdup(val);
	(*argvp)[argc  ] = NULL;
    }
    return 0;
}

int poptSaveLongLong(long long * arg, unsigned int argInfo, long long aLongLong)
{
    if (arg == NULL )
	return POPT_ERROR_NULLARG;

    if (aLongLong != 0 && LF_ISSET(RANDOM)) {
	/* XXX avoid adding POPT_ERROR_UNIMPLEMENTED to minimize i18n churn. */
	return POPT_ERROR_BADOPERATION;
    }
    if (LF_ISSET(NOT))
	aLongLong = ~aLongLong;
    switch (LF_ISSET(LOGICALOPS)) {
    case 0:
	*arg = aLongLong;
	break;
    case POPT_ARGFLAG_OR:
	*(unsigned long long *)arg |= (unsigned long long)aLongLong;
	break;
    case POPT_ARGFLAG_AND:
	*(unsigned long long *)arg &= (unsigned long long)aLongLong;
	break;
    case POPT_ARGFLAG_XOR:
	*(unsigned long long *)arg ^= (unsigned long long)aLongLong;
	break;
    default:
	return POPT_ERROR_BADOPERATION;
	break;
    }
    return 0;
}

int poptSaveLong(long * arg, unsigned int argInfo, long aLong)
{
    /* XXX Check alignment, may fail on funky platforms. */
    if (arg == NULL || (((unsigned long)arg) & (sizeof(*arg)-1)))
	return POPT_ERROR_NULLARG;

    if (aLong != 0 && LF_ISSET(RANDOM)) {
	/* XXX avoid adding POPT_ERROR_UNIMPLEMENTED to minimize i18n churn. */
	return POPT_ERROR_BADOPERATION;
    }
    if (LF_ISSET(NOT))
	aLong = ~aLong;
    switch (LF_ISSET(LOGICALOPS)) {
    case 0:		   *arg = aLong; break;
    case POPT_ARGFLAG_OR:  *(unsigned long *)arg |= (unsigned long)aLong; break;
    case POPT_ARGFLAG_AND: *(unsigned long *)arg &= (unsigned long)aLong; break;
    case POPT_ARGFLAG_XOR: *(unsigned long *)arg ^= (unsigned long)aLong; break;
    default:
	return POPT_ERROR_BADOPERATION;
	break;
    }
    return 0;
}

int poptSaveInt(int * arg, unsigned int argInfo, long aLong)
{
    /* XXX Check alignment, may fail on funky platforms. */
    if (arg == NULL || (((unsigned long)arg) & (sizeof(*arg)-1)))
	return POPT_ERROR_NULLARG;

    if (aLong != 0 && LF_ISSET(RANDOM)) {
	/* XXX avoid adding POPT_ERROR_UNIMPLEMENTED to minimize i18n churn. */
	return POPT_ERROR_BADOPERATION;
    }
    if (LF_ISSET(NOT))
	aLong = ~aLong;
    switch (LF_ISSET(LOGICALOPS)) {
    case 0:		   *arg = (int) aLong;				break;
    case POPT_ARGFLAG_OR:  *(unsigned int *)arg |= (unsigned int) aLong; break;
    case POPT_ARGFLAG_AND: *(unsigned int *)arg &= (unsigned int) aLong; break;
    case POPT_ARGFLAG_XOR: *(unsigned int *)arg ^= (unsigned int) aLong; break;
    default:
	return POPT_ERROR_BADOPERATION;
	break;
    }
    return 0;
}

int poptSaveShort(short * arg, unsigned int argInfo, long aLong)
{
    /* XXX Check alignment, may fail on funky platforms. */
    if (arg == NULL || (((unsigned long)arg) & (sizeof(*arg)-1)))
	return POPT_ERROR_NULLARG;

    if (aLong != 0 && LF_ISSET(RANDOM)) {
	/* XXX avoid adding POPT_ERROR_UNIMPLEMENTED to minimize i18n churn. */
	return POPT_ERROR_BADOPERATION;
    }
    if (LF_ISSET(NOT))
	aLong = ~aLong;
    switch (LF_ISSET(LOGICALOPS)) {
    case 0:		   *arg = (short) aLong;
	break;
    case POPT_ARGFLAG_OR:  *(unsigned short *)arg |= (unsigned short) aLong;
	break;
    case POPT_ARGFLAG_AND: *(unsigned short *)arg &= (unsigned short) aLong;
	break;
    case POPT_ARGFLAG_XOR: *(unsigned short *)arg ^= (unsigned short) aLong;
	break;
    default: return POPT_ERROR_BADOPERATION;
	break;
    }
    return 0;
}

/**
 * Return argInfo field, handling POPT_ARGFLAG_TOGGLE overrides.
 * @param con		context
 * @param opt           option
 * @return		argInfo
 */
static unsigned int poptArgInfo(poptContext con, const struct poptOption * opt)
{
    unsigned int argInfo = opt->argInfo;

    if (con->os->argv != NULL && con->os->next > 0 && opt->longName != NULL)
    if (LF_ISSET(TOGGLE)) {
	const char * longName = con->os->argv[con->os->next-1];
	while (*longName == '-') longName++;
	/* XXX almost good enough but consider --[no]nofoo corner cases. */
	if (longName[0] != opt->longName[0] || longName[1] != opt->longName[1])
	{
	    if (!LF_ISSET(XOR)) {	/* XXX dont toggle with XOR */
		/* Toggle POPT_BIT_SET <=> POPT_BIT_CLR. */
		if (LF_ISSET(LOGICALOPS))
		    argInfo ^= (POPT_ARGFLAG_OR|POPT_ARGFLAG_AND);
		argInfo ^= POPT_ARGFLAG_NOT;
	    }
	}
    }
    return argInfo;
}

/**
 * Parse an integer expression.
 * @retval *llp		integer expression value
 * @param argInfo	integer expression type
 * @param val		integer expression string
 * @return		0 on success, otherwise POPT_* error.
 */
static int poptParseInteger(long long * llp,
		UNUSED(unsigned int argInfo),
		const char * val)
{
    if (val) {
	char *end = NULL;
	*llp = strtoll(val, &end, 0);

	/* XXX parse scaling suffixes here. */

	if (!(end && *end == '\0'))
	    return POPT_ERROR_BADNUMBER;
    } else
	*llp = 0;
    return 0;
}

/**
 * Save the option argument through the (*opt->arg) pointer.
 * @param con		context
 * @param opt           option
 * @return		0 on success, otherwise POPT_* error.
 */
static int poptSaveArg(poptContext con, const struct poptOption * opt)
{
    poptArg arg;
	arg.ptr = opt->arg;
    int rc = 0;		/* assume success */

    switch (poptArgType(opt)) {
    case POPT_ARG_BITSET:
	/* XXX memory leak, application is responsible for free. */
	rc = poptSaveBits((poptBits*)arg.ptr, opt->argInfo, con->os->nextArg);
	break;
    case POPT_ARG_ARGV:
	/* XXX memory leak, application is responsible for free. */
	rc = poptSaveString((const char***)arg.ptr, opt->argInfo, con->os->nextArg);
	break;
    case POPT_ARG_STRING:
	/* XXX memory leak, application is responsible for free. */
	arg.argv[0] = (con->os->nextArg) ? xstrdup(con->os->nextArg) : NULL;
	break;

    case POPT_ARG_INT:
    case POPT_ARG_SHORT:
    case POPT_ARG_LONG:
    case POPT_ARG_LONGLONG:
    {	unsigned int argInfo = poptArgInfo(con, opt);
	long long aNUM = 0;

	if ((rc = poptParseInteger(&aNUM, argInfo, con->os->nextArg)) != 0)
	    break;

	switch (poptArgType(opt)) {
	case POPT_ARG_LONGLONG:
/* XXX let's not demand C99 compiler flags for <limits.h> quite yet. */
	    rc = !(aNUM == LLONG_MIN || aNUM == LLONG_MAX)
		? poptSaveLongLong(arg.longlongp, argInfo, aNUM)
		: POPT_ERROR_OVERFLOW;
	    break;
	case POPT_ARG_LONG:
	    rc = !(aNUM < (long long)LONG_MIN || aNUM > (long long)LONG_MAX)
		? poptSaveLong(arg.longp, argInfo, (long)aNUM)
		: POPT_ERROR_OVERFLOW;
	    break;
	case POPT_ARG_INT:
	    rc = !(aNUM < (long long)INT_MIN || aNUM > (long long)INT_MAX)
		? poptSaveInt(arg.intp, argInfo, (long)aNUM)
		: POPT_ERROR_OVERFLOW;
	    break;
	case POPT_ARG_SHORT:
	    rc = !(aNUM < (long long)SHRT_MIN || aNUM > (long long)SHRT_MAX)
		? poptSaveShort(arg.shortp, argInfo, (long)aNUM)
		: POPT_ERROR_OVERFLOW;
	    break;
	}
    }   break;

    case POPT_ARG_FLOAT:
    case POPT_ARG_DOUBLE:
    {	char *end = NULL;
	double aDouble = 0.0;

	if (con->os->nextArg) {
	    int saveerrno = errno;
	    errno = 0;
	    aDouble = strtod(con->os->nextArg, &end);
	    if (errno == ERANGE) {
		rc = POPT_ERROR_OVERFLOW;
		break;
	    }
	    errno = saveerrno;
	    if (*end != '\0') {
		rc = POPT_ERROR_BADNUMBER;
		break;
	    }
	}

	switch (poptArgType(opt)) {
	case POPT_ARG_DOUBLE:
	    arg.doublep[0] = aDouble;
	    break;
	case POPT_ARG_FLOAT:
#define POPT_ABS(a)	((((a) - 0.0) < DBL_EPSILON) ? -(a) : (a))
	    if ((FLT_MIN - POPT_ABS(aDouble)) > DBL_EPSILON
	     || (POPT_ABS(aDouble) - FLT_MAX) > DBL_EPSILON)
		rc = POPT_ERROR_OVERFLOW;
	    else
		arg.floatp[0] = (float) aDouble;
	    break;
	}
    }   break;
    case POPT_ARG_MAINCALL:
	con->maincall = (int (*)(int,const char **))opt->arg;
	break;
    default:
	fprintf(stdout, POPT_("option type (%u) not implemented in popt\n"),
		poptArgType(opt));
	exit(EXIT_FAILURE);
	break;
    }
    return rc;
}

/* returns 'val' element, -1 on last item, POPT_ERROR_* on error */
int poptGetNextOpt(poptContext con)
{
    const struct poptOption * opt = NULL;
    int done = 0;

    if (con == NULL)
	return -1;
    while (!done) {
	const char * origOptString = NULL;
	poptCallbackType cb = NULL;
	const void * cbData = NULL;
	const char * longArg = NULL;
	int canstrip = 0;
	int shorty = 0;

	while (!con->os->nextCharArg && con->os->next == con->os->argc
		&& con->os > con->optionStack) {
	    cleanOSE(con->os--);
	}
	if (!con->os->nextCharArg && con->os->next == con->os->argc) {
	    invokeCallbacksPOST(con, con->options);

	    if (con->maincall) {
		(void) (*con->maincall) (con->finalArgvCount, con->finalArgv);
		return -1;
	    }

	    if (con->doExec) return execCommand(con);
	    return -1;
	}

	/* Process next long option */
	if (!con->os->nextCharArg) {
	    const char * optString;
            size_t optStringLen;
	    int thisopt;

	    if (con->os->argb && PBM_ISSET(con->os->next, con->os->argb)) {
		con->os->next++;
		continue;
	    }
	    thisopt = con->os->next;
	    if (con->os->argv != NULL)	/* XXX can't happen */
	    origOptString = con->os->argv[con->os->next++];

	    if (origOptString == NULL)	/* XXX can't happen */
		return POPT_ERROR_BADOPT;

	    if (con->restLeftover || *origOptString != '-' ||
		(*origOptString == '-' && origOptString[1] == '\0'))
	    {
		if (con->flags & POPT_CONTEXT_POSIXMEHARDER)
		    con->restLeftover = 1;
		if (con->flags & POPT_CONTEXT_ARG_OPTS) {
		    con->os->nextArg = xstrdup(origOptString);
		    return 0;
		}
		if (con->leftovers != NULL)	/* XXX can't happen */
		    con->leftovers[con->numLeftovers++] = origOptString;
		continue;
	    }

	    /* Make a copy we can hack at */
	    optString = origOptString;

	    if (optString[0] == '\0')
		return POPT_ERROR_BADOPT;

	    if (optString[1] == '-' && !optString[2]) {
		con->restLeftover = 1;
		continue;
	    } else {
		const char *oe;
		unsigned int argInfo = 0;

		optString++;
		if (*optString == '-')
		    optString++;
		else
		    argInfo |= POPT_ARGFLAG_ONEDASH;

		/* Check for "--long=arg" option. */
		for (oe = optString; *oe && *oe != '='; oe++)
		    {};
		optStringLen = (size_t)(oe - optString);
		if (*oe == '=')
		    longArg = oe + 1;

		/* XXX aliases with arg substitution need "--alias=arg" */
		if (handleAlias(con, optString, optStringLen, '\0', longArg)) {
		    longArg = NULL;
		    continue;
		}

		if (handleExec(con, optString, '\0'))
		    continue;

		opt = findOption(con->options, optString, optStringLen, '\0', &cb, &cbData,
				 argInfo);
		if (!opt && !LF_ISSET(ONEDASH))
		    return POPT_ERROR_BADOPT;
	    }

	    if (!opt) {
		con->os->nextCharArg = origOptString + 1;
		longArg = NULL;
	    } else {
		if (con->os == con->optionStack && F_ISSET(opt, STRIP))
		{
		    canstrip = 1;
		    poptStripArg(con, thisopt);
		}
		shorty = 0;
	    }
	}

	/* Process next short option */
	if (con->os->nextCharArg) {
	    const char * nextCharArg = con->os->nextCharArg;

	    con->os->nextCharArg = NULL;

	    if (handleAlias(con, NULL, 0, *nextCharArg, nextCharArg + 1))
		continue;

	    if (handleExec(con, NULL, *nextCharArg)) {
		/* Restore rest of short options for further processing */
		nextCharArg++;
		if (*nextCharArg != '\0')
		    con->os->nextCharArg = nextCharArg;
		continue;
	    }

	    opt = findOption(con->options, NULL, 0, *nextCharArg, &cb,
			     &cbData, 0);
	    if (!opt)
		return POPT_ERROR_BADOPT;
	    shorty = 1;

	    nextCharArg++;
	    if (*nextCharArg != '\0')
		con->os->nextCharArg = nextCharArg + (int)(*nextCharArg == '=');
	}

	if (opt == NULL) return POPT_ERROR_BADOPT;	/* XXX can't happen */
	if (opt->arg && poptArgType(opt) == POPT_ARG_NONE) {
	    unsigned int argInfo = poptArgInfo(con, opt);
	    if (poptSaveInt((int *)opt->arg, argInfo, 1L))
		return POPT_ERROR_BADOPERATION;
	} else if (poptArgType(opt) == POPT_ARG_VAL) {
	    if (opt->arg) {
		unsigned int argInfo = poptArgInfo(con, opt);
		if (poptSaveInt((int *)opt->arg, argInfo, (long)opt->val))
		    return POPT_ERROR_BADOPERATION;
	    }
	} else if (poptArgType(opt) != POPT_ARG_NONE) {
	    int rc;

	    con->os->nextArg = (char*)_free(con->os->nextArg);
	    if (longArg) {
		longArg = expandNextArg(con, longArg);
		con->os->nextArg = (char *) longArg;
	    } else if (con->os->nextCharArg) {
		longArg = expandNextArg(con, con->os->nextCharArg);
		con->os->nextArg = (char *) longArg;
		con->os->nextCharArg = NULL;
	    } else {
		while (con->os->next == con->os->argc &&
			con->os > con->optionStack)
		{
		    cleanOSE(con->os--);
		}
		if (con->os->next == con->os->argc) {
		    if (!F_ISSET(opt, OPTIONAL))
			return POPT_ERROR_NOARG;
		    con->os->nextArg = NULL;
		} else {

		    /*
		     * Make sure this isn't part of a short arg or the
		     * result of an alias expansion.
		     */
		    if (con->os == con->optionStack
		     && F_ISSET(opt, STRIP) && canstrip)
		    {
			poptStripArg(con, con->os->next);
		    }
		
		    if (con->os->argv != NULL) {	/* XXX can't happen */
			if (F_ISSET(opt, OPTIONAL) &&
			    con->os->argv[con->os->next][0] == '-') {
			    con->os->nextArg = NULL;
			} else {
			    /* XXX watchout: subtle side-effects live here. */
			    longArg = con->os->argv[con->os->next++];
			    longArg = expandNextArg(con, longArg);
			    con->os->nextArg = (char *) longArg;
			}
		    }
		}
	    }
	    longArg = NULL;

	   /* Save the option argument through a (*opt->arg) pointer. */
	    if (opt->arg != NULL && (rc = poptSaveArg(con, opt)) != 0)
		return rc;
	}

	if (cb)
	    invokeCallbacksOPTION(con, con->options, opt, cbData, shorty);
	else if (opt->val && (poptArgType(opt) != POPT_ARG_VAL))
	    done = 1;

	if ((con->finalArgvCount + 2) >= (con->finalArgvAlloced)) {
	    con->finalArgvAlloced += 10;
	    con->finalArgv = (poptArgv)realloc(con->finalArgv,
			    sizeof(*con->finalArgv) * con->finalArgvAlloced);
	}

	if (con->finalArgv != NULL)
	{   char *s = (char*)malloc((opt->longName ? strlen(opt->longName) : 0) + sizeof("--"));
	    if (s != NULL) {	/* XXX can't happen */
		con->finalArgv[con->finalArgvCount++] = s;
		*s++ = '-';
		if (opt->longName) {
		    if (!F_ISSET(opt, ONEDASH))
			*s++ = '-';
		    s = stpcpy(s, opt->longName);
		} else {
		    *s++ = opt->shortName;
		    *s = '\0';
		}
	    } else
		con->finalArgv[con->finalArgvCount++] = NULL;
	}

	if (opt->arg && poptArgType(opt) == POPT_ARG_NONE);
	else if (poptArgType(opt) == POPT_ARG_VAL);
	else if (poptArgType(opt) != POPT_ARG_NONE) {
	    if (con->finalArgv != NULL && con->os->nextArg != NULL)
	        con->finalArgv[con->finalArgvCount++] =
			xstrdup(con->os->nextArg);
	}
    }

    return (opt ? opt->val : -1);	/* XXX can't happen */
}

char * poptGetOptArg(poptContext con)
{
    char * ret = NULL;
    if (con) {
	ret = con->os->nextArg;
	con->os->nextArg = NULL;
    }
    return ret;
}

const char * poptGetArg(poptContext con)
{
    const char * ret = NULL;
    if (con && con->leftovers != NULL && con->nextLeftover < con->numLeftovers)
	ret = con->leftovers[con->nextLeftover++];
    return ret;
}

const char * poptPeekArg(poptContext con)
{
    const char * ret = NULL;
    if (con && con->leftovers != NULL && con->nextLeftover < con->numLeftovers)
	ret = con->leftovers[con->nextLeftover];
    return ret;
}

const char ** poptGetArgs(poptContext con)
{
    if (con == NULL ||
	con->leftovers == NULL || con->numLeftovers == con->nextLeftover)
	return NULL;

    /* some apps like [like RPM ;-) ] need this NULL terminated */
    con->leftovers[con->numLeftovers] = NULL;

    return (con->leftovers + con->nextLeftover);
}

static
poptItem poptFreeItems(poptItem items, int nitems)
{
    if (items != NULL) {
	poptItem item = items;
	while (--nitems >= 0) {
	    item->option.longName = (const char*)_free(item->option.longName);
	    item->option.descrip = (const char*)_free(item->option.descrip);
	    item->option.argDescrip = (const char*)_free(item->option.argDescrip);
	    item->argv = (const char**)_free(item->argv);
	    item++;
	}
	items = (poptItem)_free(items);
    }
    return NULL;
}

poptContext poptFreeContext(poptContext con)
{
    if (con == NULL) return con;
    poptResetContext(con);
    con->os->argb = (pbm_set*)_free(con->os->argb);

    con->aliases = poptFreeItems(con->aliases, con->numAliases);
    con->numAliases = 0;

    con->execs = poptFreeItems(con->execs, con->numExecs);
    con->numExecs = 0;

    con->leftovers = (poptArgv)_free(con->leftovers);
    con->finalArgv = (poptArgv)_free(con->finalArgv);
    con->appName = (const char*)_free(con->appName);
    con->otherHelp = (const char*)_free(con->otherHelp);
    con->execPath = (const char*)_free(con->execPath);
    con->arg_strip = (pbm_set*)PBM_FREE(con->arg_strip);
    
    con = (poptContext)_free(con);
    return con;
}

int poptAddAlias(poptContext con, struct poptAlias alias,
		UNUSED(int flags))
{
    struct poptItem_s item_buf;
    poptItem item = &item_buf;
    memset(item, 0, sizeof(*item));
    item->option.longName = alias.longName;
    item->option.shortName = alias.shortName;
    item->option.argInfo = POPT_ARGFLAG_DOC_HIDDEN;
    item->option.arg = 0;
    item->option.val = 0;
    item->option.descrip = NULL;
    item->option.argDescrip = NULL;
    item->argc = alias.argc;
    item->argv = alias.argv;
    return poptAddItem(con, item, 0);
}

int poptAddItem(poptContext con, poptItem newItem, int flags)
{
    poptItem * items, item;
    int * nitems;

    switch (flags) {
    case 1:
	items = &con->execs;
	nitems = &con->numExecs;
	break;
    case 0:
	items = &con->aliases;
	nitems = &con->numAliases;
	break;
    default:
	return 1;
	break;
    }

    *items = (poptItem)realloc((*items), ((*nitems) + 1) * sizeof(**items));
    if ((*items) == NULL)
	return 1;

    item = (*items) + (*nitems);

    item->option.longName =
	(newItem->option.longName ? xstrdup(newItem->option.longName) : NULL);
    item->option.shortName = newItem->option.shortName;
    item->option.argInfo = newItem->option.argInfo;
    item->option.arg = newItem->option.arg;
    item->option.val = newItem->option.val;
    item->option.descrip =
	(newItem->option.descrip ? xstrdup(newItem->option.descrip) : NULL);
    item->option.argDescrip =
       (newItem->option.argDescrip ? xstrdup(newItem->option.argDescrip) : NULL);
    item->argc = newItem->argc;
    item->argv = newItem->argv;

    (*nitems)++;

    return 0;
}

const char * poptBadOption(poptContext con, unsigned int flags)
{
    struct optionStackEntry * os = NULL;

    if (con != NULL)
	os = (flags & POPT_BADOPTION_NOALIAS) ? con->optionStack : con->os;

    return (os != NULL && os->argv != NULL ? os->argv[os->next - 1] : NULL);
}

const char * poptStrerror(const int error)
{
    switch (error) {
      case POPT_ERROR_NOARG:
	return POPT_("missing argument");
      case POPT_ERROR_BADOPT:
	return POPT_("unknown option");
      case POPT_ERROR_BADOPERATION:
	return POPT_("mutually exclusive logical operations requested");
      case POPT_ERROR_NULLARG:
	return POPT_("opt->arg should not be NULL");
      case POPT_ERROR_OPTSTOODEEP:
	return POPT_("aliases nested too deeply");
      case POPT_ERROR_BADQUOTE:
	return POPT_("error in parameter quoting");
      case POPT_ERROR_BADNUMBER:
	return POPT_("invalid numeric value");
      case POPT_ERROR_OVERFLOW:
	return POPT_("number too large or too small");
      case POPT_ERROR_MALLOC:
	return POPT_("memory allocation failed");
      case POPT_ERROR_BADCONFIG:
	return POPT_("config file failed sanity test");
      case POPT_ERROR_ERRNO:
	return strerror(errno);
      default:
	return POPT_("unknown error");
    }
}

int poptStuffArgs(poptContext con, const char ** argv)
{
    int argc;
    int rc;

    if ((con->os - con->optionStack) == POPT_OPTION_DEPTH)
	return POPT_ERROR_OPTSTOODEEP;

    for (argc = 0; argv[argc]; argc++)
	{};

    con->os++;
    con->os->next = 0;
    con->os->nextArg = NULL;
    con->os->nextCharArg = NULL;
    con->os->currAlias = NULL;
    rc = poptDupArgv(argc, argv, &con->os->argc, &con->os->argv);
    con->os->argb = NULL;
    con->os->stuffed = 1;

    return rc;
}

const char * poptGetInvocationName(poptContext con)
{
    return (con->os->argv ? con->os->argv[0] : "");
}

int poptStrippedArgv(poptContext con, int argc, char ** argv)
{
    int numargs = argc;
    int j = 1;
    int i;
    
    if (con->arg_strip)
    for (i = 1; i < argc; i++) {
	if (PBM_ISSET(i, con->arg_strip))
	    numargs--;
    }
    
    for (i = 1; i < argc; i++) {
	if (con->arg_strip && PBM_ISSET(i, con->arg_strip))
	    continue;
	argv[j] = (j < numargs) ? argv[i] : NULL;
	j++;
    }
    
    return numargs;
}
