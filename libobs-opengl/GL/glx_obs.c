#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "glx_obs.h"

#if defined(__APPLE__)
#include <dlfcn.h>

static void* AppleGLGetProcAddress (const const char *name)
{
	static void* image = NULL;
	
	if (NULL == image)
		image = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_LAZY);

	return (image ? dlsym(image, name) : NULL);
}
#define IntGetProcAddress(name) AppleGLGetProcAddress(name)
#endif /* __APPLE__ */

#if defined(_WIN32)

#ifdef _MSC_VER
#pragma warning(disable: 4055)
#pragma warning(disable: 4054)
#pragma warning(disable: 4996)
#endif
	
#define IntGetProcAddress(name) wglGetProcAddress((LPCSTR)name)
#endif

/* Linux, FreeBSD, other */
#ifndef IntGetProcAddress
	#ifndef GLX_ARB_get_proc_address
	#define GLX_ARB_get_proc_address 1

	typedef void (*__GLXextFuncPtr)(void);
	extern __GLXextFuncPtr glXGetProcAddressARB (const GLubyte *);

	#endif /* GLX_ARB_get_proc_address */

	#define IntGetProcAddress(name) (*glXGetProcAddressARB)((const GLubyte*)name)
#endif

int glx_ext_ARB_create_context = glx_LOAD_FAILED;
int glx_ext_ARB_create_context_profile = glx_LOAD_FAILED;
int glx_ext_ARB_create_context_robustness = glx_LOAD_FAILED;
int glx_ext_ARB_fbconfig_float = glx_LOAD_FAILED;
int glx_ext_ARB_framebuffer_sRGB = glx_LOAD_FAILED;
int glx_ext_ARB_multisample = glx_LOAD_FAILED;
int glx_ext_EXT_create_context_es2_profile = glx_LOAD_FAILED;
int glx_ext_EXT_fbconfig_packed_float = glx_LOAD_FAILED;
int glx_ext_EXT_framebuffer_sRGB = glx_LOAD_FAILED;
int glx_ext_EXT_import_context = glx_LOAD_FAILED;
int glx_ext_EXT_swap_control = glx_LOAD_FAILED;
int glx_ext_EXT_swap_control_tear = glx_LOAD_FAILED;

GLXContext (CODEGEN_FUNCPTR *_ptrc_glXCreateContextAttribsARB)(Display *, GLXFBConfig, GLXContext, Bool, const int *) = NULL;

static int Load_ARB_create_context()
{
	int numFailed = 0;
	_ptrc_glXCreateContextAttribsARB = (GLXContext (CODEGEN_FUNCPTR *)(Display *, GLXFBConfig, GLXContext, Bool, const int *))IntGetProcAddress("glXCreateContextAttribsARB");
	if(!_ptrc_glXCreateContextAttribsARB) numFailed++;
	return numFailed;
}

void (CODEGEN_FUNCPTR *_ptrc_glXFreeContextEXT)(Display *, GLXContext) = NULL;
GLXContextID (CODEGEN_FUNCPTR *_ptrc_glXGetContextIDEXT)(const GLXContext) = NULL;
Display * (CODEGEN_FUNCPTR *_ptrc_glXGetCurrentDisplayEXT)() = NULL;
GLXContext (CODEGEN_FUNCPTR *_ptrc_glXImportContextEXT)(Display *, GLXContextID) = NULL;
int (CODEGEN_FUNCPTR *_ptrc_glXQueryContextInfoEXT)(Display *, GLXContext, int, int *) = NULL;

static int Load_EXT_import_context()
{
	int numFailed = 0;
	_ptrc_glXFreeContextEXT = (void (CODEGEN_FUNCPTR *)(Display *, GLXContext))IntGetProcAddress("glXFreeContextEXT");
	if(!_ptrc_glXFreeContextEXT) numFailed++;
	_ptrc_glXGetContextIDEXT = (GLXContextID (CODEGEN_FUNCPTR *)(const GLXContext))IntGetProcAddress("glXGetContextIDEXT");
	if(!_ptrc_glXGetContextIDEXT) numFailed++;
	_ptrc_glXGetCurrentDisplayEXT = (Display * (CODEGEN_FUNCPTR *)())IntGetProcAddress("glXGetCurrentDisplayEXT");
	if(!_ptrc_glXGetCurrentDisplayEXT) numFailed++;
	_ptrc_glXImportContextEXT = (GLXContext (CODEGEN_FUNCPTR *)(Display *, GLXContextID))IntGetProcAddress("glXImportContextEXT");
	if(!_ptrc_glXImportContextEXT) numFailed++;
	_ptrc_glXQueryContextInfoEXT = (int (CODEGEN_FUNCPTR *)(Display *, GLXContext, int, int *))IntGetProcAddress("glXQueryContextInfoEXT");
	if(!_ptrc_glXQueryContextInfoEXT) numFailed++;
	return numFailed;
}

void (CODEGEN_FUNCPTR *_ptrc_glXSwapIntervalEXT)(Display *, GLXDrawable, int) = NULL;

static int Load_EXT_swap_control()
{
	int numFailed = 0;
	_ptrc_glXSwapIntervalEXT = (void (CODEGEN_FUNCPTR *)(Display *, GLXDrawable, int))IntGetProcAddress("glXSwapIntervalEXT");
	if(!_ptrc_glXSwapIntervalEXT) numFailed++;
	return numFailed;
}

typedef int (*PFN_LOADFUNCPOINTERS)();
typedef struct glx_StrToExtMap_s
{
	char *extensionName;
	int *extensionVariable;
	PFN_LOADFUNCPOINTERS LoadExtension;
} glx_StrToExtMap;

static glx_StrToExtMap ExtensionMap[12] = {
	{"GLX_ARB_create_context", &glx_ext_ARB_create_context, Load_ARB_create_context},
	{"GLX_ARB_create_context_profile", &glx_ext_ARB_create_context_profile, NULL},
	{"GLX_ARB_create_context_robustness", &glx_ext_ARB_create_context_robustness, NULL},
	{"GLX_ARB_fbconfig_float", &glx_ext_ARB_fbconfig_float, NULL},
	{"GLX_ARB_framebuffer_sRGB", &glx_ext_ARB_framebuffer_sRGB, NULL},
	{"GLX_ARB_multisample", &glx_ext_ARB_multisample, NULL},
	{"GLX_EXT_create_context_es2_profile", &glx_ext_EXT_create_context_es2_profile, NULL},
	{"GLX_EXT_fbconfig_packed_float", &glx_ext_EXT_fbconfig_packed_float, NULL},
	{"GLX_EXT_framebuffer_sRGB", &glx_ext_EXT_framebuffer_sRGB, NULL},
	{"GLX_EXT_import_context", &glx_ext_EXT_import_context, Load_EXT_import_context},
	{"GLX_EXT_swap_control", &glx_ext_EXT_swap_control, Load_EXT_swap_control},
	{"GLX_EXT_swap_control_tear", &glx_ext_EXT_swap_control_tear, NULL},
};

static int g_extensionMapSize = 12;

static glx_StrToExtMap *FindExtEntry(const char *extensionName)
{
	int loop;
	glx_StrToExtMap *currLoc = ExtensionMap;
	for(loop = 0; loop < g_extensionMapSize; ++loop, ++currLoc)
	{
		if(strcmp(extensionName, currLoc->extensionName) == 0)
			return currLoc;
	}
	
	return NULL;
}

static void ClearExtensionVars()
{
	glx_ext_ARB_create_context = glx_LOAD_FAILED;
	glx_ext_ARB_create_context_profile = glx_LOAD_FAILED;
	glx_ext_ARB_create_context_robustness = glx_LOAD_FAILED;
	glx_ext_ARB_fbconfig_float = glx_LOAD_FAILED;
	glx_ext_ARB_framebuffer_sRGB = glx_LOAD_FAILED;
	glx_ext_ARB_multisample = glx_LOAD_FAILED;
	glx_ext_EXT_create_context_es2_profile = glx_LOAD_FAILED;
	glx_ext_EXT_fbconfig_packed_float = glx_LOAD_FAILED;
	glx_ext_EXT_framebuffer_sRGB = glx_LOAD_FAILED;
	glx_ext_EXT_import_context = glx_LOAD_FAILED;
	glx_ext_EXT_swap_control = glx_LOAD_FAILED;
	glx_ext_EXT_swap_control_tear = glx_LOAD_FAILED;
}


static void LoadExtByName(const char *extensionName)
{
	glx_StrToExtMap *entry = NULL;
	entry = FindExtEntry(extensionName);
	if(entry)
	{
		if(entry->LoadExtension)
		{
			int numFailed = entry->LoadExtension();
			if(numFailed == 0)
			{
				*(entry->extensionVariable) = glx_LOAD_SUCCEEDED;
			}
			else
			{
				*(entry->extensionVariable) = glx_LOAD_SUCCEEDED + numFailed;
			}
		}
		else
		{
			*(entry->extensionVariable) = glx_LOAD_SUCCEEDED;
		}
	}
}


static void ProcExtsFromExtString(const char *strExtList)
{
	size_t iExtListLen = strlen(strExtList);
	const char *strExtListEnd = strExtList + iExtListLen;
	const char *strCurrPos = strExtList;
	char strWorkBuff[256];

	while(*strCurrPos)
	{
		/*Get the extension at our position.*/
		int iStrLen = 0;
		const char *strEndStr = strchr(strCurrPos, ' ');
		int iStop = 0;
		if(strEndStr == NULL)
		{
			strEndStr = strExtListEnd;
			iStop = 1;
		}

		iStrLen = (int)((ptrdiff_t)strEndStr - (ptrdiff_t)strCurrPos);

		if(iStrLen > 255)
			return;

		strncpy(strWorkBuff, strCurrPos, iStrLen);
		strWorkBuff[iStrLen] = '\0';

		LoadExtByName(strWorkBuff);

		strCurrPos = strEndStr + 1;
		if(iStop) break;
	}
}

int glx_LoadFunctions(Display *display, int screen)
{
	ClearExtensionVars();
	
	
	ProcExtsFromExtString((const char *)glXQueryExtensionsString(display, screen));
	return glx_LOAD_SUCCEEDED;
}

