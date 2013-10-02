/* ------------------------------------------------------------------------- */

GLboolean GLEWAPIENTRY glewGetExtension (const char* name)
{    
  const GLubyte* start;
  const GLubyte* end;
  start = (const GLubyte*)glGetString(GL_EXTENSIONS);
  if (start == 0)
    return GL_FALSE;
  end = start + _glewStrLen(start);
  return _glewSearchExtension(name, start, end);
}

/* ------------------------------------------------------------------------- */

#ifndef GLEW_MX
static
#endif
GLenum GLEWAPIENTRY glewContextInit (GLEW_CONTEXT_ARG_DEF_LIST)
{
  const GLubyte* s;
  GLuint dot;
  GLint major, minor;
  const GLubyte* extStart;
  const GLubyte* extEnd;
  /* query opengl version */
  s = glGetString(GL_VERSION);
  dot = _glewStrCLen(s, '.');
  if (dot == 0)
    return GLEW_ERROR_NO_GL_VERSION;
  
  major = s[dot-1]-'0';
  minor = s[dot+1]-'0';

  if (minor < 0 || minor > 9)
    minor = 0;
  if (major<0 || major>9)
    return GLEW_ERROR_NO_GL_VERSION;
  

  if (major == 1 && minor == 0)
  {
    return GLEW_ERROR_GL_VERSION_10_ONLY;
  }
  else
  {
    CONST_CAST(GLEW_VERSION_4_4)   = ( major > 4 )                 || ( major == 4 && minor >= 4 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_4_3)   = GLEW_VERSION_4_4   == GL_TRUE || ( major == 4 && minor >= 3 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_4_2)   = GLEW_VERSION_4_3   == GL_TRUE || ( major == 4 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_4_1)   = GLEW_VERSION_4_2   == GL_TRUE || ( major == 4 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_4_0)   = GLEW_VERSION_4_1   == GL_TRUE || ( major == 4               ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_3_3)   = GLEW_VERSION_4_0   == GL_TRUE || ( major == 3 && minor >= 3 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_3_2)   = GLEW_VERSION_3_3   == GL_TRUE || ( major == 3 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_3_1)   = GLEW_VERSION_3_2   == GL_TRUE || ( major == 3 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_3_0)   = GLEW_VERSION_3_1   == GL_TRUE || ( major == 3               ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_2_1)   = GLEW_VERSION_3_0   == GL_TRUE || ( major == 2 && minor >= 1 ) ? GL_TRUE : GL_FALSE;    
    CONST_CAST(GLEW_VERSION_2_0)   = GLEW_VERSION_2_1   == GL_TRUE || ( major == 2               ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_5)   = GLEW_VERSION_2_0   == GL_TRUE || ( major == 1 && minor >= 5 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_4)   = GLEW_VERSION_1_5   == GL_TRUE || ( major == 1 && minor >= 4 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_3)   = GLEW_VERSION_1_4   == GL_TRUE || ( major == 1 && minor >= 3 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_2_1) = GLEW_VERSION_1_3   == GL_TRUE                                 ? GL_TRUE : GL_FALSE; 
    CONST_CAST(GLEW_VERSION_1_2)   = GLEW_VERSION_1_2_1 == GL_TRUE || ( major == 1 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_1)   = GLEW_VERSION_1_2   == GL_TRUE || ( major == 1 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
  }

  /* query opengl extensions string */
  extStart = glGetString(GL_EXTENSIONS);
  if (extStart == 0)
    extStart = (const GLubyte*)"";
  extEnd = extStart + _glewStrLen(extStart);

  /* initialize extensions */
