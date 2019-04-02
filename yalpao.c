/*
 * yalpao.c --
 *
 * Implements Yorick interface to Alpao deformable mirrors.
 *
 *-----------------------------------------------------------------------------
 *
 * Copyright (C) 2019: Éric Thiébaut <eric.thiebaut@univ-lyon1.fr>
 *
 * See LICENSE.md for details.
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>

#include <pstdlib.h>
#include <play.h>
#include <yapi.h>

/* Define some macros to get rid of some GNU extensions when not compiling
   with GCC. */
#if ! (defined(__GNUC__) && __GNUC__ > 1)
#   define __attribute__(x)
#   define __inline__
#   define __FUNCTION__        ""
#   define __PRETTY_FUNCTION__ ""
#endif

PLUG_API void y_error(const char *) __attribute__ ((noreturn));

static void push_string(const char* str);

void
Y_yalpao_greetings(int argc)
{
  push_string("Hello, this is \"YAlpao\" (Yorick interface to Alpao deformable mirrors)");
}

static void
push_string(const char* str)
{
  ypush_q(NULL)[0] = p_strcpy(str);
}

/*---------------------------------------------------------------------------*/
