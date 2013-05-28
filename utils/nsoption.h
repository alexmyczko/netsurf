/*
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
 * Option reading and saving (interface).
 *
 * Global options are defined in desktop/options.h
 * Distinct target options are defined in <TARGET>/options.h
 *
 * The implementation API is slightly compromised because it still has
 * "global" tables for both the default and current option tables.
 *
 * The initialisation and read/write interfaces take pointers to an
 * option table which would let us to make the option structure
 * opaque.
 *
 * All the actual acessors assume direct access to a global option
 * table (nsoptions). To avoid this the acessors would have to take a
 * pointer to the active options table and be implemented as functions
 * within nsoptions.c
 *
 * Indirect acees would have an impact on performance of NetSurf as
 * the expected option lookup cost is currently that of a simple
 * dereference (which this current implementation keeps).
 */

#ifndef _NETSURF_UTILS_NSOPTION_H_
#define _NETSURF_UTILS_NSOPTION_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "utils/errors.h"

/* allow targets to include any necessary headers of their own */
#define NSOPTION_BOOL(NAME, DEFAULT)
#define NSOPTION_STRING(NAME, DEFAULT)
#define NSOPTION_INTEGER(NAME, DEFAULT)
#define NSOPTION_UINT(NAME, DEFAULT)
#define NSOPTION_COLOUR(NAME, DEFAULT)

#include "desktop/options.h"
#if defined(riscos)
#include "riscos/options.h"
#elif defined(nsgtk)
#include "gtk/options.h"
#elif defined(nsbeos)
#include "beos/options.h"
#elif defined(nsamiga)
#include "amiga/options.h"
#elif defined(nsframebuffer)
#include "framebuffer/options.h"
#elif defined(nsatari)
#include "atari/options.h"
#elif defined(nsmonkey)
#include "monkey/options.h"
#endif

#undef NSOPTION_BOOL
#undef NSOPTION_STRING
#undef NSOPTION_INTEGER
#undef NSOPTION_UINT
#undef NSOPTION_COLOUR



enum { OPTION_HTTP_PROXY_AUTH_NONE = 0,
       OPTION_HTTP_PROXY_AUTH_BASIC = 1,
       OPTION_HTTP_PROXY_AUTH_NTLM = 2 };

#define DEFAULT_MARGIN_TOP_MM 10
#define DEFAULT_MARGIN_BOTTOM_MM 10
#define DEFAULT_MARGIN_LEFT_MM 10
#define DEFAULT_MARGIN_RIGHT_MM 10
#define DEFAULT_EXPORT_SCALE 0.7

#ifndef DEFAULT_REFLOW_PERIOD
#define DEFAULT_REFLOW_PERIOD 25 /* time in cs */
#endif

enum nsoption_type_e {
	OPTION_BOOL,
	OPTION_INTEGER,
	OPTION_UINT,
	OPTION_STRING,
	OPTION_COLOUR
};

struct nsoption_s {
	const char *key;
	int key_len;
	enum nsoption_type_e type;
	union {
		bool b;
		int i;
		unsigned int u;
		char *s;
		const char *cs;
		colour c;
	} value;
};

/* construct the option enumeration */
#define NSOPTION_BOOL(NAME, DEFAULT) NSOPTION_##NAME,
#define NSOPTION_STRING(NAME, DEFAULT) NSOPTION_##NAME,
#define NSOPTION_INTEGER(NAME, DEFAULT) NSOPTION_##NAME,
#define NSOPTION_UINT(NAME, DEFAULT) NSOPTION_##NAME,
#define NSOPTION_COLOUR(NAME, DEFAULT) NSOPTION_##NAME,

enum nsoption_e {
#include "desktop/options.h"
#if defined(riscos)
#include "riscos/options.h"
#elif defined(nsgtk)
#include "gtk/options.h"
#elif defined(nsbeos)
#include "beos/options.h"
#elif defined(nsamiga)
#include "amiga/options.h"
#elif defined(nsframebuffer)
#include "framebuffer/options.h"
#elif defined(nsatari)
#include "atari/options.h"
#elif defined(nsmonkey)
#include "monkey/options.h"
#endif
	NSOPTION_LISTEND /* end of list */
};

#undef NSOPTION_BOOL
#undef NSOPTION_STRING
#undef NSOPTION_INTEGER
#undef NSOPTION_UINT
#undef NSOPTION_COLOUR

/* global option table */
extern struct nsoption_s *nsoptions;

/* global default option table */
extern struct nsoption_s *nsoptions_default;

/* default setting callback */
typedef nserror(nsoption_set_default_t)(struct nsoption_s *defaults);


/** Initialise option system.
 *
 * @param set_default callback to allow the customisation of the default
 *                    options.
 * @param ppots pointer to update to get options table or NULL.
 * @param pdefs pointer to update to get default options table or NULL.
 * @return The error status
 */
nserror nsoption_init(nsoption_set_default_t *set_default, struct nsoption_s **popts, struct nsoption_s **pdefs);


/** Read choices file and set them in the passed table
 *
 * @param path The path to read the file from
 * @param opts The options table to enerate values from or NULL to use global
 * @return The error status
 */
nserror nsoption_read(const char *path, struct nsoption_s *opts);


/** Write options that have changed from the defaults to a file.
 *
 * The \a nsoption_dump can be used to output all entries not just
 * changed ones.
 *
 * @param path The path to read the file from
 * @param opts The options table to enerate values from or NULL to use global
 * @param defs The default table to use or NULL to use global
 * @return The error status
 */
nserror nsoption_write(const char *path, struct nsoption_s *opts, struct nsoption_s *defs);


/**
 * Write all options to a stream.
 *
 * @param outf The stream to write to
 * @param opts The options table to enerate values from or NULL to use global
 * @return The error status
 */
nserror nsoption_dump(FILE *outf, struct nsoption_s *opts);

/**
 * Process commandline and set options approriately.
 *
 * @param pargc Pointer to the size of the argument vector.
 * @param argv The argument vector.
 * @param opts The options table to enerate values from or NULL to use global
 * @return The error status
 */
nserror nsoption_commandline(int *pargc, char **argv, struct nsoption_s *opts);

/**
 * Fill a buffer with an option using a format.
 *
 * The format string is copied into the output buffer with the
 * following replaced:
 * %k - The options key
 * %t - The options type
 * %V - value (HTML formatting)
 * %v - value (plain formatting)
 *
 * @param string The buffer in which to place the results.
 * @param size The size of the string buffer.
 * @param option The option .
 * @param fmt The format string.
 * @return The number of bytes written to \a string or -1 on error
 */
int nsoption_snoptionf(char *string, size_t size, enum nsoption_e option, const char *fmt);




/* value acessors - caution should be taken with type as this is not verified */
#define nsoption_bool(OPTION) (nsoptions[NSOPTION_##OPTION].value.b)
#define nsoption_int(OPTION) (nsoptions[NSOPTION_##OPTION].value.i)
#define nsoption_uint(OPTION) (nsoptions[NSOPTION_##OPTION].value.u)
#define nsoption_charp(OPTION) (nsoptions[NSOPTION_##OPTION].value.s)
#define nsoption_colour(OPTION) (nsoptions[NSOPTION_##OPTION].value.c)

#define nsoption_set_bool(OPTION, VALUE) nsoptions[NSOPTION_##OPTION].value.b = VALUE
#define nsoption_set_int(OPTION, VALUE) nsoptions[NSOPTION_##OPTION].value.i = VALUE
#define nsoption_set_colour(OPTION, VALUE) nsoptions[NSOPTION_##OPTION].value.c = VALUE
#define nsoption_set_charp(OPTION, VALUE)				\
	do {								\
		if (nsoptions[NSOPTION_##OPTION].value.s != NULL) {	\
			free(nsoptions[NSOPTION_##OPTION].value.s);	\
		}							\
		nsoptions[NSOPTION_##OPTION].value.s = VALUE;		\
		if ((nsoptions[NSOPTION_##OPTION].value.s != NULL) &&	\
		    (*nsoptions[NSOPTION_##OPTION].value.s == 0)) {	\
			free(nsoptions[NSOPTION_##OPTION].value.s);	\
			nsoptions[NSOPTION_##OPTION].value.s = NULL;	\
		}							\
	} while (0)

/* if a string option is unset set it otherwise leave it set */
#define nsoption_setnull_charp(OPTION, VALUE)				\
	do {								\
		if (nsoptions[NSOPTION_##OPTION].value.s == NULL) {	\
			nsoptions[NSOPTION_##OPTION].value.s = VALUE;	\
			if (*nsoptions[NSOPTION_##OPTION].value.s == 0) { \
				free(nsoptions[NSOPTION_##OPTION].value.s); \
				nsoptions[NSOPTION_##OPTION].value.s = NULL; \
			}						\
		} else {						\
			free(VALUE);					\
		}							\
	} while (0)


#endif
