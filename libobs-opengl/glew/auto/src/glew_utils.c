/*
** The OpenGL Extension Wrangler Library
** Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
** Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
** Copyright (C) 2002, Lev Povalahev
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without 
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice, 
**   this list of conditions and the following disclaimer.
** * Redistributions in binary form must reproduce the above copyright notice, 
**   this list of conditions and the following disclaimer in the documentation 
**   and/or other materials provided with the distribution.
** * The name of the author may be used to endorse or promote products 
**   derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
** THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <GL/glew.h>
#if defined(_WIN32)
#  include <GL/wglew.h>
#elif !defined(__ANDROID__) && !defined(__native_client__) && (!defined(__APPLE__) || defined(GLEW_APPLE_GLX))
#  include <GL/glxew.h>
#endif

#include "glew_utils.h"

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <stdlib.h>
#include <string.h>

void* NSGLGetProcAddress (const GLubyte *name)
{
  NSSymbol symbol;
  char* symbolName;
  /* prepend a '_' for the Unix C symbol mangling convention */
  symbolName = malloc(strlen((const char*)name) + 2);
  strcpy(symbolName+1, (const char*)name);
  symbolName[0] = '_';
  symbol = NULL;
  if (NSIsSymbolNameDefined(symbolName))
    symbol = NSLookupAndBindSymbol(symbolName);
  free(symbolName);
  return symbol ? NSAddressOfSymbol(symbol) : NULL;
}
#endif /* __APPLE__ */

#if defined(__sgi) || defined (__sun)
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void* dlGetProcAddress (const GLubyte* name)
{
  static void* h = NULL;
  static void* gpa;

  if (h == NULL)
  {
    if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL) return NULL;
    gpa = dlsym(h, "glXGetProcAddress");
  }

  if (gpa != NULL)
    return ((void*(*)(const GLubyte*))gpa)(name);
  else
    return dlsym(h, (const char*)name);
}
#endif /* __sgi || __sun */

/*
 * GLEW, just like OpenGL or GLU, does not rely on the standard C library.
 * These functions implement the functionality required in this file.
 */

GLuint _glewStrLen (const GLubyte* s)
{
  GLuint i=0;
  while (s+i != NULL && s[i] != '\0') i++;
  return i;
}

GLuint _glewStrCLen (const GLubyte* s, GLubyte c)
{
  GLuint i=0;
  while (s+i != NULL && s[i] != '\0' && s[i] != c) i++;
  return i;
}

GLboolean _glewStrSame (const GLubyte* a, const GLubyte* b, GLuint n)
{
  GLuint i=0;
  while (i < n && a+i != NULL && b+i != NULL && a[i] == b[i]) i++;
  return i == n ? GL_TRUE : GL_FALSE;
}

GLboolean _glewStrSame1 (GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  while (*na > 0 && (**a == ' ' || **a == '\n' || **a == '\r' || **a == '\t'))
  {
    *a++;
    *na--;
  }
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && *a+i != NULL && b+i != NULL && *a[i] == b[i]) i++;
	if(i == nb)
	{
		*a = *a + nb;
		*na = *na - nb;
		return GL_TRUE;
	}
  }
  return GL_FALSE;
}

GLboolean _glewStrSame2 (GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && *a+i != NULL && b+i != NULL && *a[i] == b[i]) i++;
	if(i == nb)
	{
		*a = *a + nb;
		*na = *na - nb;
		return GL_TRUE;
	}
  }
  return GL_FALSE;
}

GLboolean _glewStrSame3 (GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && *a+i != NULL && b+i != NULL && *a[i] == b[i]) i++;
    if (i == nb && (*na == nb || *a[i] == ' ' || *a[i] == '\n' || *a[i] == '\r' || *a[i] == '\t'))
    {
      *a = *a + nb;
      *na = *na - nb;
      return GL_TRUE;
    }
  }
  return GL_FALSE;
}
