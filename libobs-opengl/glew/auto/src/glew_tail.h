/* ------------------------------------------------------------------------- */

/* error codes */
#define GLEW_OK 0
#define GLEW_NO_ERROR 0
#define GLEW_ERROR_NO_GL_VERSION 1  /* missing GL version */
#define GLEW_ERROR_GL_VERSION_10_ONLY 2  /* Need at least OpenGL 1.1 */
#define GLEW_ERROR_GLX_VERSION_11_ONLY 3  /* Need at least GLX 1.2 */

/* string codes */
#define GLEW_VERSION 1
#define GLEW_VERSION_MAJOR 2
#define GLEW_VERSION_MINOR 3
#define GLEW_VERSION_MICRO 4

/* API */
#ifdef GLEW_MX

typedef struct GLEWContextStruct GLEWContext;
GLEWAPI GLenum GLEWAPIENTRY glewContextInit (GLEWContext *ctx);
GLEWAPI GLboolean GLEWAPIENTRY glewContextIsSupported (const GLEWContext *ctx, const char *name);

#define glewInit() glewContextInit(glewGetContext())
#define glewIsSupported(x) glewContextIsSupported(glewGetContext(), x)
#define glewIsExtensionSupported(x) glewIsSupported(x)

#define GLEW_GET_VAR(x) (*(const GLboolean*)&(glewGetContext()->x))
#ifdef _WIN32
#  define GLEW_GET_FUN(x) glewGetContext()->x
#else
#  define GLEW_GET_FUN(x) x
#endif

#else /* GLEW_MX */

GLEWAPI GLenum GLEWAPIENTRY glewInit (void);
GLEWAPI GLboolean GLEWAPIENTRY glewIsSupported (const char *name);
#define glewIsExtensionSupported(x) glewIsSupported(x)

#define GLEW_GET_VAR(x) (*(const GLboolean*)&x)
#define GLEW_GET_FUN(x) x

#endif /* GLEW_MX */

GLEWAPI GLboolean glewExperimental;
GLEWAPI GLboolean GLEWAPIENTRY glewGetExtension (const char *name);
GLEWAPI const GLubyte * GLEWAPIENTRY glewGetErrorString (GLenum error);
GLEWAPI const GLubyte * GLEWAPIENTRY glewGetString (GLenum name);

#ifdef __cplusplus
}
#endif

#ifdef GLEW_APIENTRY_DEFINED
#undef GLEW_APIENTRY_DEFINED
#undef APIENTRY
#undef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifdef GLEW_CALLBACK_DEFINED
#undef GLEW_CALLBACK_DEFINED
#undef CALLBACK
#endif

#ifdef GLEW_WINGDIAPI_DEFINED
#undef GLEW_WINGDIAPI_DEFINED
#undef WINGDIAPI
#endif

#undef GLAPI
/* #undef GLEWAPI */

#endif /* __glew_h__ */
