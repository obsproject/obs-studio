local util = require "util"
local struct = require "Sample_Struct"
local common = require "CommonStyle"

local function GetIncludeGuard(spec, options)
  local temp = 
    options.prefix .. spec.GetIncludeGuardString() .. "_THIS_IS_A_TEST_H"
  return temp:upper()
end

local function GetExtensionVarName(extName, spec, options)
	return options.prefix .. spec.DeclPrefix() .. "ext_" .. extName
end

local function GetEnumName(enum, spec, options)
	return spec.EnumNamePrefix() .. enum.name
end

local function GetFuncPtrName(func, spec, options)
  return options.prefix .. "_testc_".. spec.FuncNamePrefix() .. func.name
end

local function GetFuncPtrDef(hFile, func, spec, options)
  return string.format("%s (%s *%s)(%s)",
    common.GetFuncReturnType(func),
    spec.GetCodegenPtrType(),
    GetFuncPtrName(func, spec, options),
    common.GetFuncParamList(func))
end

local function GetFuncPtrType(hFile, func, spec, options)
  return string.format("%s (%s *)(%s)",
    common.GetFuncReturnType(func),
    spec.GetCodegenPtrType(),
    common.GetFuncParamList(func))
end

local function GetMainLoaderFuncName(spec, options)
  return options.prefix .. spec.DeclPrefix() .. "LoadFunctions"
end

local function GetExtFuncLoaderName(extName, spec, options)
  return "Load_" .. extName;
end

local function GetMappingTableStructName(spec, options)
  return string.format("%s%sStringToExtMap",
    options.prefix, spec.DeclPrefix())
end

local function GetMappingTableVarName()
  return "g_stringToExtMap"
end




local my_style = {}

local hdr = {}
my_style.hdr = hdr

function hdr.GetFilename(basename, spec, options)
  return basename .. ".h"
end

function hdr.WriteBlockBeginIncludeGuard(hFile, spec, options)
  local guard = GetIncludeGuard(spec, options)
  hFile:fmt("#ifndef %s\n", guard)
  hFile:fmt("#define %s\n", guard)
end

function hdr.WriteBlockEndIncludeGuard(hFile, spec, options)
  hFile:fmt("#endif /*%s*/\n", GetIncludeGuard(spec, options))
end

function hdr.WriteGuards(hFile, spec, options)
  hFile:rawwrite(spec.GetHeaderInit())
end

function hdr.WriteTypedefs(hFile, specData, spec, options)
  local defArray = common.GetStdTypedefs()
	
  --Use include-guards for the typedefs, since they're common among
  --headers in this style.
  hFile:write("#ifndef GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS\n")
  hFile:write("#define GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS\n")
  hFile:write("\n")
  hFile:inc()
	
  for _, def in ipairs(defArray) do
    hFile:write(def)
  end
	
  hFile:dec()
  hFile:write("\n")
  hFile:write("#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/\n")
  hFile:write("\n")

  common.WritePassthruData(hFile, specData.funcData.passthru)
end

hdr.WriteBlockBeginExtern = common.WriteExternCStart
hdr.WriteBlockEndExtern = common.WriteExternCEnd

function hdr.WriteExtension(hFile, extName, spec, options)
  hFile:fmt("extern int %s;\n", GetExtensionVarName(extName, spec, options));
end

function hdr.WriteEnumerator(hFile, enum, enumTable, spec, options, enumSeen)
  if(enumSeen[enum.name]) then return end

  hFile:fmt("#define %s %s\n",
    GetEnumName(enum, spec, options),
    common.ResolveEnumValue(enum, enumTable))
end

function hdr.WriteFunction(hFile, func, spec, options, funcSeen)
  if(funcSeen[func.name]) then return end

  hFile:fmt("extern %s;\n",
    GetFuncPtrDef(hFile, func, spec, options))

  hFile:fmt("#define %s %s\n",
    common.GetOpenGLFuncName(func, spec),
    GetFuncPtrName(func, spec, options))
end

function hdr.WriteMainLoaderFunc(hFile, spec, options)
	hFile:fmt("int %s(%s);\n",
		GetMainLoaderFuncName(spec, options),
		spec.GetLoaderParams())
end


local src = {}
my_style.src = src

function src.GetFilename(basename, spec, options)
  return basename .. ".c"
end

function src.WriteIncludes(hFile, basename, spec, options)
  hFile:writeblock([[
#include <stdlib.h>
#include <string.h>
]])
  local base = util.ParsePath(hdr.GetFilename(basename, spec, options))
  hFile:fmt('#include "%s"\n', base)
end

function src.WriteLoaderFunc(hFile, spec, options)
  hFile:writeblock(spec.GetLoaderFunc())
end

function src.WriteExtension(hFile, extName, spec, options)
  hFile:fmt("int %s = 0;\n", GetExtensionVarName(extName, spec, options));
end

function src.WriteFunction(hFile, func, spec, options, funcSeen)
  if(funcSeen[func.name]) then return end
  hFile:fmt("%s = NULL;\n", GetFuncPtrDef(hFile, func, spec, options));
end

function src.WriteBlockBeginExtFuncLoader(hFile, extName, spec, options)
  hFile:fmt("static void %s()\n", GetExtFuncLoaderName(extName, spec, options))
  hFile:write("{\n")
  hFile:inc()
end

function src.WriteBlockEndExtFuncLoader(hFile, extName, spec, options)
  hFile:dec()
  hFile:write("}\n")
end

function src.WriteLoadFunction(hFile, func, spec, options)
  hFile:fmt('%s = (%s)%s("%s%s");\n',
    GetFuncPtrName(func, spec, options),
	GetFuncPtrType(hFile, func, spec, options),
    spec.GetPtrLoaderFuncName(),
    spec.FuncNamePrefix(),
    func.name)
end

function src.WriteBlockBeginCoreLoader(hFile, spec, options)
  hFile:write("static void Load_Version()\n")
  hFile:write("{\n")
  hFile:inc()
end

function src.WriteBlockEndCoreLoader(hFile, version, spec, options)
  hFile:dec()
  hFile:write("}\n")
end

local function WriteHelpers(hFile, specData, spec, options)
  common.WriteCClearExtensionVarsFunc(hFile, specData, spec, options,
    GetExtensionVarName, "0")
  hFile:write("\n")
  hFile:write("typedef void (*PFN_LOADFUNCPOINTERS)();\n")
	hFile:fmt("typedef struct %s_s\n", 
		GetMappingTableStructName(spec, options))
	hFile:write("{\n")
	hFile:inc()
	hFile:writeblock [[
char *extensionName;
int *extensionVariable;
PFN_LOADFUNCPOINTERS LoadExtension;
]]
	hFile:dec()
	hFile:fmt("} %s;\n", GetMappingTableStructName(spec, options))
	hFile:write "\n"
	
	hFile:write "\n" --From last line of previous code.
	hFile:fmt("static %s %s[] = {\n",
	  GetMappingTableStructName(spec, options),
	  GetMappingTableVarName())
	hFile:inc()
	for _, extName in ipairs(options.extensions) do
	  if(#specData.extdefs[extName].funcs > 0) then
		hFile:fmt('{"%s", &%s, %s},\n',
		  spec.ExtNamePrefix() .. extName,
		  GetExtensionVarName(extName, spec, options),
		  GetExtFuncLoaderName(extName, spec, options))
	  else
		hFile:fmt('{"%s", &%s, NULL},\n',
		  spec.ExtNamePrefix() .. extName,
		  GetExtensionVarName(extName, spec, options))
	  end
	end
	hFile:dec()
	hFile:write("};\n")
	hFile:write("\n")
	hFile:fmt("static int g_extensionMapSize = %i;\n", #options.extensions);	
	hFile:write "\n"

	common.WriteCFindExtEntryFunc(hFile, specData, spec, options,
		GetMappingTableStructName(spec, options),
		GetMappingTableVarName())
		
	hFile:write("\n")

	hFile:fmtblock([[
static void LoadExtByName(const char *extensionName)
{
	%s *entry = NULL;
	entry = FindExtEntry(extensionName);
	if(entry)
	{
		if(entry->LoadExtension)
		{
			int numFailed = entry->LoadExtension();
			if(numFailed == 0)
			{
				*(entry->extensionVariable) = 1;
			}
			else
			{
				*(entry->extensionVariable) = 1;
			}
		}
		else
		{
			*(entry->extensionVariable) = 1;
		}
	}
}
]], GetMappingTableStructName(spec, options))

	hFile:write("\n")
	local indexed = spec.GetIndexedExtStringFunc(options);
	if(not indexed) then
	  common.WriteProcessExtsFromStringFunc(hFile, "LoadExtByName(%s)")
	else
	  --New style
	end

	return indexed
end

function src.WriteMainLoaderFunc(hFile, specData, spec, options)
  WriteHelpers(hFile, specData, spec, options)
  hFile:write("\n")

  hFile:fmt("int %s(%s)\n",
    GetMainLoaderFuncName(spec, options),
    spec.GetLoaderParams())
  hFile:write("{\n")
  hFile:inc()
  hFile:dec()
  hFile:write("}\n")
end




local function Create()
    return util.DeepCopyTable(my_style), struct
end

return { Create = Create }
