
local common = require "CommonStyle"
local struct = require "Structure"
local util = require "util"


local my_style = {}
my_style.header = {}
my_style.source = {}

function my_style.WriteLargeHeading(hFile, headingName)
	hFile:write("/*", string.rep("*", #headingName), "*/\n")
	hFile:write("/*", headingName, "*/\n")
end

function my_style.WriteSmallHeading(hFile, headingName)
	hFile:write("/*", headingName, "*/\n")
end

----------------------------------------------------------------
-- Header file construction

function my_style.header.GetFilename(basename, options)
	return basename .. ".h"
end


local function GetIncludeGuard(hFile, spec, options)
	local str = "POINTER_C_GENERATED_HEADER_" ..
		spec.GetIncludeGuardString() .. "_H"

	if(#options.prefix > 0) then
		return options.prefix:upper() .. "_" .. str
	end
	
	return str
end

function my_style.header.WriteBlockBeginIncludeGuard(hFile, spec, options)
	local inclGuard = GetIncludeGuard(hFile, spec, options)
	
	hFile:fmt("#ifndef %s\n", inclGuard)
	hFile:fmt("#define %s\n", inclGuard)
end

function my_style.header.WriteBlockEndIncludeGuard(hFile, spec, options)
	local inclGuard = GetIncludeGuard(hFile, spec, options)
	
	hFile:fmt("#endif //%s\n", inclGuard)
end

function my_style.header.WriteInit(hFile, spec, options)
	hFile:rawwrite(spec.GetHeaderInit())
end

function my_style.header.WriteStdTypedefs(hFile, specData, options)
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
end

function my_style.header.WriteSpecTypedefs(hFile, specData, options)
	hFile:push()
	common.WritePassthruData(hFile, specData.funcData.passthru)
	hFile:pop()
end

function my_style.header.WriteBlockBeginDecl(hFile, specData, options)
	common.WriteExternCStart(hFile)
end

function my_style.header.WriteBlockBeginExtVarDecl(hFile, spec, options)
end

function my_style.header.WriteBlockEndExtVarDecl(hFile, spec, options)
end

local function GetExtVariableName(extName, spec, options)
	return options.prefix .. spec.DeclPrefix() .. "ext_" .. extName
end

function my_style.header.WriteExtVariableDecl(hFile, extName, specData, spec, options)
	hFile:write("extern int ", GetExtVariableName(extName, spec, options), ";\n");
end

function my_style.header.WriteBlockBeginEnumDecl(hFile, specData, options) end

function my_style.header.WriteBlockEndEnumDecl(hFile, specData, options) end

local function GetEnumName(enum, spec, options)
	return spec.EnumNamePrefix() .. enum.name
end

function my_style.header.WriteEnumDecl(hFile, enum, enumTable, spec, options, enumSeen)
	if(enumSeen[enum.name]) then
		hFile:fmt("/*Copied %s%s From: %s*/\n",
			spec.EnumNamePrefix(),
			enum.name,
			enumSeen[enum.name])
	else
		hFile:fmt("#define %s %s\n",
			GetEnumName(enum, spec, options),
			common.ResolveEnumValue(enum, enumTable))
	end
end

function my_style.header.WriteBlockBeginFuncDecl(hFile, specData, options)
end

local function GetFuncPtrName(func, spec, options)
	return options.prefix .. "_ptrc_".. spec.FuncNamePrefix() .. func.name
end

local function GetFuncPtrType(hFile, func, spec, options)
	return string.format("%s (%s *)(%s)",
		common.GetFuncReturnType(func),
		spec.GetCodegenPtrType(),
		common.GetFuncParamList(func))
end

local function GetFuncPtrDef(hFile, func, spec, options)
	return string.format("%s (%s *%s)(%s)",
		common.GetFuncReturnType(func),
		spec.GetCodegenPtrType(),
		GetFuncPtrName(func, spec, options),
		common.GetFuncParamList(func))
end

function my_style.header.WriteFuncDecl(hFile, func, spec, options)
	--Declare the function pointer.
	hFile:write("extern ",
		GetFuncPtrDef(hFile, func, spec, options),
		";\n")
	
	--#define it to the proper OpenGL name.
	hFile:fmt("#define %s %s\n",
		common.GetOpenGLFuncName(func, spec),
		GetFuncPtrName(func, spec, options))
end

function my_style.header.WriteBlockEndFuncDecl(hFile, specData, options)
end

function my_style.header.WriteBlockBeginExtFuncDecl(hFile, extName,
	spec, options)
	hFile:fmt("#ifndef %s\n", spec.ExtNamePrefix() .. extName)
	hFile:fmt("#define %s 1\n", spec.ExtNamePrefix() .. extName)
end

function my_style.header.WriteBlockEndExtFuncDecl(hFile, extName,
	spec, options)
	hFile:fmt("#endif /*%s*/ \n", spec.ExtNamePrefix() .. extName)
end

function my_style.header.WriteBlockBeginSysDecl(hFile, spec, options)
end

function my_style.header.WriteBlockEndSysDecl(hFile, spec, options)
end

local function GetStatusCodeEnumName(spec, options)
	return string.format("%s%sLoadStatus", options.prefix, spec.DeclPrefix())
end

local function GetStatusCodeName(name, spec, options)
	return string.format("%s%s%s", options.prefix, spec.DeclPrefix(), name)
end

function my_style.header.WriteUtilityDecls(hFile, spec, options)
	hFile:fmt("enum %s\n", GetStatusCodeEnumName(spec, options))
	hFile:write("{\n")
	hFile:inc()
		hFile:write(GetStatusCodeName("LOAD_FAILED", spec, options), " = 0,\n")
		hFile:write(GetStatusCodeName("LOAD_SUCCEEDED", spec, options), " = 1,\n")
	hFile:dec()
	hFile:write("};\n")
end

local function DecorateFuncName(name, spec, options)
	return string.format("%s%s%s", options.prefix, spec.DeclPrefix(), name)
end

local function GetLoaderFuncName(spec, options)
	return DecorateFuncName("LoadFunctions", spec, options)
end

function my_style.header.WriteMainLoaderFuncDecl(hFile, spec, options)
	hFile:fmt("int %s(%s);\n",
		GetLoaderFuncName(spec, options),
		spec.GetLoaderParams())
end

function my_style.header.WriteVersioningFuncDecls(hFile, spec, options)
	--Only for GL
	if(options.spec ~= "gl") then
		return
	end
	
	hFile:fmt("int %s();\n", DecorateFuncName("GetMinorVersion", spec, options))
	hFile:fmt("int %s();\n", DecorateFuncName("GetMajorVersion", spec, options))
	hFile:fmt("int %s(int majorVersion, int minorVersion);\n",
		DecorateFuncName("IsVersionGEQ", spec, options))
end

function my_style.header.WriteBlockEndDecl(hFile, specData, options)
	common.WriteExternCEnd(hFile)
end

--------------------------------------------------
-- Source file construction functions.

function my_style.source.GetFilename(basename, options)
	return basename .. ".c"
end

function my_style.source.WriteIncludes(hFile, basename, spec, options)
	hFile:writeblock([[
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
]])
	local base = util.ParsePath(my_style.header.GetFilename(basename, options))
	hFile:fmt('#include "%s"\n', base)
end

function my_style.source.WriteLoaderData(hFile, spec, options)
	hFile:writeblock(spec.GetLoaderFunc())
end

function my_style.source.WriteBlockBeginDef(hFile, spec, options) end
function my_style.source.WriteBlockEndDef(hFile, spec, options) end

function my_style.source.WriteBlockBeginExtVarDef(hFile, spec, options)
end

function my_style.source.WriteBlockEndExtVarDef(hFile, spec, options)
end

function my_style.source.WriteExtVariableDef(hFile, extName, specData, spec, options)
	hFile:fmt("int %s = %s;\n", GetExtVariableName(extName, spec, options),
		GetStatusCodeName("LOAD_FAILED", spec, options));
end

function my_style.source.WriteBlockBeginExtFuncDef(hFile, extName, spec, options)
end

function my_style.source.WriteBlockEndExtFuncDef(hFile, extName, spec, options)
end

function my_style.source.WriteFuncDef(hFile, func, spec, options, funcSeen)
	--Declare the function pointer, if not already declared.
	if(funcSeen[func.name]) then return end
	
	hFile:fmt("%s = NULL;\n",
		GetFuncPtrDef(hFile, func, spec, options))
end

local function GetExtLoaderFuncName(extName, spec, options)
	return "Load_" .. extName;
end

function my_style.source.WriteBlockBeginExtLoader(hFile, extName, spec, options)
	hFile:fmt("static int %s()\n", GetExtLoaderFuncName(extName, spec, options))
	hFile:write("{\n")
	hFile:inc()
	hFile:write("int numFailed = 0;\n")
end

function my_style.source.WriteBlockEndExtLoader(hFile, extName, spec, options)
	hFile:write("return numFailed;\n")
	hFile:dec()
	hFile:write("}\n")
end

function my_style.source.WriteExtFuncLoader(hFile, func, spec, options)
	hFile:fmt('%s = (%s)%s("%s%s");\n',
		GetFuncPtrName(func, spec, options),
		GetFuncPtrType(hFile, func, spec, options),
		common.GetProcAddressName(spec),
		spec.FuncNamePrefix(), func.name)
	hFile:fmt('if(!%s) numFailed++;\n', GetFuncPtrName(func, spec, options))
end

function my_style.source.WriteBlockBeginCoreFuncDef(hFile, spec, options)
end

function my_style.source.WriteBlockEndCoreFuncDef(hFile, spec, options)
end

local function GetCoreLoaderFuncName(spec, options)
	return "Load_Version_" .. options.version:gsub("%.", "_")
end

function my_style.source.WriteBlockBeginCoreLoader(hFile, spec, options)
	hFile:fmt("static int %s()\n", GetCoreLoaderFuncName(spec, options))
	hFile:write("{\n")
	hFile:inc()
	hFile:write("int numFailed = 0;\n")
end

function my_style.source.WriteBlockEndCoreLoader(hFile, version, spec, options)
	hFile:write("return numFailed;\n")
	hFile:dec()
	hFile:write("}\n")
end

function my_style.source.WriteCoreFuncLoader(hFile, func, spec, options)
	hFile:fmt('%s = (%s)%s("%s%s");\n',
		GetFuncPtrName(func, spec, options),
		GetFuncPtrType(hFile, func, spec, options),
		common.GetProcAddressName(spec),
		spec.FuncNamePrefix(), func.name)

	--Special hack for DSA_EXT functions in core functions.
	--They do not count against the loaded count.
	if(func.name:match("EXT$")) then
		hFile:write("/*An EXT_direct_state_access-based function. Don't count it.*/")
	else
		hFile:fmt('if(!%s) numFailed++;\n', GetFuncPtrName(func, spec, options))
	end
end

function my_style.source.WriteExtStringFuncDef(hFile, specData, spec, options, funcSeen)
	if(funcSeen[spec.GetExtStringFuncName()]) then
		return
	end
	
	--Check to see if its something we have to load.
	local function FindFuncName(funcName)
		for _, func in ipairs(specData.funcData.functions) do
			if(func.name == funcName) then
				return func
			end
		end
		
		return nil
	end
	
	local extStringFunc = FindFuncName(spec.GetExtStringFuncName())

	if(extStringFunc) then
		hFile:write("\n")
		hFile:fmt("static %s = NULL;\n",
			GetFuncPtrDef(hFile, extStringFunc, spec, options))
		hFile:write("\n")
	end
end

local function GetMapTableStructName(spec, options)
	return string.format("%s%sStrToExtMap", options.prefix, spec.DeclPrefix())
end

local function GetMapTableVarName()
	return "ExtensionMap"
end

function my_style.source.WriteBlockBeginSysDef(hFile, spec, options)
end

function my_style.source.WriteBlockEndSysDef(hFile, spec, options)
end

function my_style.source.WriteUtilityDefs(hFile, specData, spec, options)
	--Write the struct for the mapping table.
	local mapStructName = string.format("%s%sStrToExtMap_s", options.prefix, spec.DeclPrefix())
	common.WriteCMappingTable(hFile, specData, spec, options,
		GetMapTableStructName(spec, options),
		GetMapTableVarName(),
		GetExtVariableName,
		GetExtLoaderFuncName)
	hFile:write "\n"
	
	--Write function to find map entry by name.
	common.WriteCFindExtEntryFunc(hFile, specData, spec, options,
		GetMapTableStructName(spec, options),
		GetMapTableVarName())
	hFile:write "\n"

	--Write the function to clear the extension variables.
	common.WriteCClearExtensionVarsFunc(hFile, specData, spec, options,
		GetExtVariableName,
		GetStatusCodeName("LOAD_FAILED", spec, options))
	hFile:write "\n"
	
	--Write a function that loads an extension by name.
	common.WriteCLoadExtByNameFunc(hFile, specData, spec, options,
		GetMapTableStructName(spec, options),
		GetStatusCodeName("LOAD_SUCCEEDED", spec, options))
	hFile:write "\n"
end

local function WriteAncillaryFuncs(hFile, specData, spec, options)
	local indexed = spec.GetIndexedExtStringFunc(options);
	if(indexed) then
		common.FixupIndexedList(specData, indexed)
		hFile:writeblock(common.GetProcExtsFromExtListFunc(
			hFile, specData, spec, options,
			indexed, GetFuncPtrName, GetEnumName))
	else
		hFile:writeblock(common.GetProcessExtsFromStringFunc("LoadExtByName(%s)"))
	end
	
	hFile:write "\n"

	return indexed
end

local function WriteInMainFuncLoader(hFile, func, spec, options)
	hFile:fmt('%s = (%s)%s("%s%s");\n',
		GetFuncPtrName(func, spec, options),
		GetFuncPtrType(hFile, func, spec, options),
		common.GetProcAddressName(spec),
		spec.FuncNamePrefix(), func.name)
	hFile:fmt('if(!%s) return %s;\n',
		GetFuncPtrName(func, spec, options),
		GetStatusCodeName("LOAD_FAILED", spec, options))
end


function my_style.source.WriteMainLoaderFunc(hFile, specData, spec, options)
	local indexed = WriteAncillaryFuncs(hFile, specData, spec, options)

	--Write the function that calls the extension and core loaders.
	hFile:fmt("int %s(%s)\n",
		GetLoaderFuncName(spec, options),
		spec.GetLoaderParams())
	hFile:write("{\n")
	hFile:inc()

	if(options.version) then
		hFile:write("int numFailed = 0;\n")
	end

	hFile:write("ClearExtensionVars();\n")
	hFile:write("\n")

	--Load the extension, using runtime-facilities to tell what is available.
	if(indexed) then
		WriteInMainFuncLoader(hFile, indexed[1], spec, options)
		WriteInMainFuncLoader(hFile, indexed[3], spec, options)
		hFile:write("\n")
		hFile:write("ProcExtsFromExtList();\n")
	else
		local extListName, needLoad = spec.GetExtStringFuncName()
		if(needLoad) then
			for _, func in ipairs(specData.funcData.functions) do
				if(extListName == func.name) then
					extListName = func
				end
			end
			
			WriteInMainFuncLoader(hFile, extListName, spec, options)
			
			extListName = GetFuncPtrName(extListName, spec, options);
		end

		local function EnumResolve(enumName)
			return GetEnumName(specData.enumtable[enumName], spec, options)
		end
		
		hFile:write "\n"
		hFile:fmt("ProcExtsFromExtString((const char *)%s(%s));\n",
			extListName,
			spec.GetExtStringParamList(EnumResolve))
	end
	
	if(options.version) then
		hFile:fmt("numFailed = %s();\n",
			GetCoreLoaderFuncName(spec, options))
		hFile:write "\n"
		
		hFile:fmtblock([[
if(numFailed == 0)
	return %s;
else
	return %s + numFailed;
]],
			GetStatusCodeName("LOAD_SUCCEEDED", spec, options),
			GetStatusCodeName("LOAD_SUCCEEDED", spec, options))
	else
		hFile:fmt("return %s;\n",
			GetStatusCodeName("LOAD_SUCCEEDED", spec, options))
	end
	
	hFile:dec()
	hFile:write("}\n")
end

function my_style.source.WriteVersioningFuncs(hFile, specData, spec, options)
	hFile:fmt("static int g_major_version = 0;\n")
	hFile:fmt("static int g_minor_version = 0;\n")
	hFile:write "\n"
	
	if(tonumber(options.version) >= 3.0) then
		hFile:writeblock([[
static void GetGLVersion()
{
	glGetIntegerv(GL_MAJOR_VERSION, &g_major_version);
	glGetIntegerv(GL_MINOR_VERSION, &g_minor_version);
}
]])
	else
		hFile:writeblock(common.GetParseVersionFromString())
		hFile:write "\n"
		
		hFile:writeblock([[
static void GetGLVersion()
{
	ParseVersionFromString(&g_major_version, &g_minor_version, (const char*)glGetString(GL_VERSION));
}
]])
	end
	
	hFile:write "\n"
	hFile:fmt("int %s()\n", DecorateFuncName("GetMajorVersion", spec, options))
	hFile:writeblock([[
{
	if(g_major_version == 0)
		GetGLVersion();
	return g_major_version;
}
]])
	hFile:write "\n"

	hFile:fmt("int %s()\n", DecorateFuncName("GetMinorVersion", spec, options))
	hFile:writeblock([[
{
	if(g_major_version == 0) //Yes, check the major version to get the minor one.
		GetGLVersion();
	return g_minor_version;
}
]])
	hFile:write "\n"
	
	hFile:fmt("int %s(int majorVersion, int minorVersion)\n",
		DecorateFuncName("IsVersionGEQ", spec, options))
	hFile:writeblock([[
{
	if(g_major_version == 0)
		GetGLVersion();
		
	if(majorVersion > g_major_version) return 1;
	if(majorVersion < g_major_version) return 0;
	if(minorVersion >= g_minor_version) return 1;
	return 0;
}
]])

end



--------------------------------------------------
-- Style retrieval machinery

local function Create()
	return common.DeepCopyTable(my_style), common.GetStandardStructure()
end

return { Create = Create }
