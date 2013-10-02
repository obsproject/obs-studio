/* ------------------------------------------------------------------------- */

#ifdef GLEW_MX
#define WGLEW_FUN_EXPORT
#define WGLEW_VAR_EXPORT
#else
#define WGLEW_FUN_EXPORT GLEW_FUN_EXPORT
#define WGLEW_VAR_EXPORT GLEW_VAR_EXPORT
#endif /* GLEW_MX */
