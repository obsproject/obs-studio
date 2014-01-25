local util = require "util"
local struct = require "NoloadC_Struct"
local common = require "CommonStyle"

--------------------------------------
-- Common functions.
local function GetIncludeGuard(spec, options)
	local temp = 
		options.prefix .. spec.GetIncludeGuardString() .. "_NOLOAD_STYLE_H"
	return temp:upper()
end

local function GetEnumName(enum, spec, options)
	return options.prefix .. spec.EnumNamePrefix() .. enum.name
end

local function GetFuncPtrName(func, spec, options)
	return options.prefix .. "_ptrc_".. spec.FuncNamePrefix() .. func.name
end

local function GetFuncName(func, spec, options)
	return options.prefix .. spec.FuncNamePrefix() .. func.name
end

local function GetFuncPtrTypedefName(func, spec, options)
	return "PFN" .. GetFuncPtrName(func, spec, options):upper() .. "PROC"
end

local function WriteFuncPtrTypedefStmt(hFile, func, spec, options)
	hFile:fmt("typedef %s (%s *%s)(%s);\n",
		common.GetFuncReturnType(func),
		spec.GetCodegenPtrType(),
		GetFuncPtrTypedefName(func, spec, options),
		common.GetFuncParamList(func))
end

local function GetFuncPtrDefDirect(func, spec, options)
	return string.format("%s (%s *%s)(%s)",
		common.GetFuncReturnType(func),
		spec.GetCodegenPtrType(),
		GetFuncPtrName(func, spec, options),
		common.GetFuncParamList(func, true))
end

local function GetFuncPtrDefTypedef(func, spec, options)
	return string.format("%s %s",
		GetFuncPtrTypedefName(func, spec, options),
		GetFuncPtrName(func, spec, options))
end

--------------------------------------
-- All style functions.
local my_style = {}

function my_style.WriteLargeHeader(hFile, value, options)
	local len = #value
	hFile:write("///", string.rep("/", len), "///\n")
	hFile:write("// ", value, "\n")
end

function my_style.WriteSmallHeader(hFile, value, options)
	hFile:write("// ", value, "\n")
end

function my_style.WriteBlockBeginExtVariables(hFile, spec, options)
end

function my_style.WriteBlockEndExtVariables(hFile, spec, options)
end

function my_style.WriteBlockBeginSystem(hFile, spec, options)
end

function my_style.WriteBlockEndSystem(hFile, spec, options)
end


---------------------------------------------
-- Header functions.
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
	hFile:fmt("#endif //%s\n", GetIncludeGuard(spec, options))
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

function hdr.WriteExtension(hFile, extName, spec, options)
	hFile:fmt("extern int %s%sext_%s;\n", options.prefix, spec.DeclPrefix(), extName)
end

function hdr.WriteBlockBeginEnumerators(hFile, spec, options)
end

function hdr.WriteBlockEndEnumerators(hFile, spec, options)
end

function hdr.WriteEnumerator(hFile, enum, enumTable, spec, options, enumSeen)
	local name = GetEnumName(enum, spec, options)
	if(enumSeen[enum.name]) then
		hFile:fmt("//%s seen in %s\n", name, enumSeen[enum.name])
	else
		hFile:fmt("#define %s%s%s\n",
			name,
			common.GetNameLengthPadding(name, 33),
			common.ResolveEnumValue(enum, enumTable))
	end
end

function hdr.WriteBlockBeginExternC(hFile, spec, options)
	common.WriteExternCStart(hFile)
end

function hdr.WriteBlockEndExternC(hFile, spec, options)
	common.WriteExternCEnd(hFile)
end

function hdr.WriteFunction(hFile, func, spec, options, funcSeen)
	if(funcSeen[func.name]) then return end
	
	hFile:write("extern ", GetFuncPtrDefDirect(func, spec, options), ";\n")
	hFile:fmt("#define %s %s\n", GetFuncName(func, spec, options),
		GetFuncPtrName(func, spec, options))
end

function hdr.WriteSetupFunction(hFile, specData, spec, options)
	hFile:fmt("void %sCheckExtensions(%s);\n", spec.DeclPrefix(), spec.GetLoaderParams())
end

function hdr.WriteVersionFunctions(hFile, specData, spec, options)
end


----------------------------------------
-- Source file.
local src = {}
my_style.src = src

function src.GetFilename(basename, spec, options)
	return basename .. ".c"
end

function src.WriteIncludes(hFile, basename, spec, options)
	hFile:writeblock([[
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
]])

	local base = util.ParsePath(hdr.GetFilename(basename, spec, options))
	hFile:fmt('#include "%s"\n', base)

end

function src.WriteLoaderFunc(hFile, spec, options)
	hFile:writeblock(spec.GetLoaderFunc())
end

function src.WriteExtension(hFile, extName, spec, options)
	hFile:fmt("int %s%sext_%s = 0;\n", options.prefix, spec.DeclPrefix(), extName)
end

function src.WriteSetupFunction(hFile, specData, spec, options)
	hFile:write "static void ClearExtensionVariables()\n"
	hFile:write "{\n"
	hFile:inc()
	
	for _, extName in ipairs(options.extensions) do
		hFile:fmt("%s%sext_%s = 0;\n", options.prefix, spec.DeclPrefix(), extName)
	end
	
	hFile:dec()
	hFile:write "}\n"
	hFile:write "\n"
	
	local mapTableName = options.prefix .. spec.DeclPrefix() .. "MapTable"
	
	hFile:writeblock([[
typedef struct ]] .. mapTableName .. [[_s
{
	char *extName;
	int *extVariable;
}]] .. mapTableName .. [[;

]])

	local arrayLength = #options.extensions
	if(arrayLength == 0) then arrayLength = 1 end

	hFile:fmt("static %s g_mappingTable[%i]", mapTableName, arrayLength)
	if(arrayLength == 1) then
		hFile:rawwrite "; //This is intensionally left uninitialized. \n"
	else
		hFile:rawwrite " = \n"
		hFile:write "{\n"
		hFile:inc()
		for _, extName in ipairs(options.extensions) do
			hFile:fmt('{"%s%s", &%s%sext_%s},\n',
				spec.ExtNamePrefix(),
				extName,
				options.prefix,
				spec.DeclPrefix(),
				extName)
		end
		hFile:dec()
		hFile:write "};\n"
	end
	
	hFile:write "\n"
	hFile:fmtblock([[
static void LoadExtByName(const char *extensionName)
{
	%s *tableEnd = &g_mappingTable[%i];
	%s *entry = &g_mappingTable[0];
	for(; entry != tableEnd; ++entry)
	{
		if(strcmp(entry->extName, extensionName) == 0)
			break;
	}
	
	if(entry != tableEnd)
		*(entry->extVariable) = 1;
}
]], mapTableName, #options.extensions, mapTableName)

	hFile:write "\n"
	
	local indexed = spec.GetIndexedExtStringFunc(options);
	if(indexed) then
		indexed[1] = specData.functable[indexed[1]]
		indexed[3] = specData.functable[indexed[3]]
		for _, enum in ipairs(specData.enumerators) do
			if(indexed[2] == enum.name) then
				indexed[2] = enum
			end
			if(indexed[4] == enum.name) then
				indexed[4] = enum
			end
		end

		hFile:writeblock([[
void ProcExtsFromExtList()
{
	GLint iLoop;
	GLint iNumExtensions = 0;
	]] .. GetFuncPtrName(indexed[1], spec, options)
	.. [[(]] .. GetEnumName(indexed[2], spec, options)
	.. [[, &iNumExtensions);

	for(iLoop = 0; iLoop < iNumExtensions; iLoop++)
	{
		const char *strExtensionName = (const char *)]] ..
		GetFuncPtrName(indexed[3], spec, options) ..
		[[(]] .. GetEnumName(indexed[4], spec, options) .. [[, iLoop);
		LoadExtByName(strExtensionName);
	}
}
]])
	else
		hFile:writeblock(
			common.GetProcessExtsFromStringFunc("LoadExtByName(%s)"))
	end

	hFile:write "\n"
	hFile:fmt("void %sCheckExtensions(%s)\n", spec.DeclPrefix(), spec.GetLoaderParams())
	hFile:write "{\n"
	hFile:inc()
	hFile:write "ClearExtensionVariables();\n"
	hFile:write "\n"
	if(indexed) then
		hFile:write("ProcExtsFromExtList();\n")
	else
		--First, check if the GetExtStringFuncName is in the specData.
		hFile:write "{\n"
		hFile:inc()
		
		local funcName = spec.GetExtStringFuncName()
		if(specData.functable[funcName]) then
			--Create a function pointer and load it.
			local func = specData.functable[funcName]
			funcName = "InternalGetExtensionString"

				hFile:fmt("typedef %s (%s *MYGETEXTSTRINGPROC)(%s);\n",
					common.GetFuncReturnType(func),
					spec.GetCodegenPtrType(),
					common.GetFuncParamList(func))
				hFile:fmt('MYGETEXTSTRINGPROC %s = (MYGETEXTSTRINGPROC)%s("%s%s");\n',
					funcName,
					spec.GetPtrLoaderFuncName(),
					spec.FuncNamePrefix(),
					func.name)
				hFile:fmt("if(!%s) return;\n", funcName)
		end
		
		hFile:fmt("ProcExtsFromExtString((const char *)%s(%s));\n",
			funcName,
			spec.GetExtStringParamList(
				function (name) return options.prefix .. spec.EnumNamePrefix() .. name end))
		hFile:dec()
		hFile:write "}\n"
	end
	hFile:dec()
	hFile:write "}\n"
end

function src.WriteVersionFunctions(hFile, specData, spec, options)
end

local typedefs = {}
src.typedefs = typedefs

function typedefs.WriteFunction(hFile, func, spec, options, funcSeen)
	if(funcSeen[func.name]) then return end

	WriteFuncPtrTypedefStmt(hFile, func, spec, options)
	hFile:fmt("static %s %s Switch_%s(%s);\n",
		common.GetFuncReturnType(func),
		spec.GetCodegenPtrType(),
		func.name,
		common.GetFuncParamList(func, true))
end

local defs = {}
src.defs = defs

function defs.WriteFunction(hFile, func, spec, options, funcSeen)
	if(funcSeen[func.name]) then return end

	hFile:fmt("%s = Switch_%s;\n",
		GetFuncPtrDefTypedef(func, spec, options),
		func.name)
end

local switch = {}
src.switch = switch

function switch.WriteFunction(hFile, func, spec, options, funcSeen)
	if(funcSeen[func.name]) then return end

	hFile:fmt("static %s %s Switch_%s(%s)\n",
		common.GetFuncReturnType(func),
		spec.GetCodegenPtrType(),
		func.name,
		common.GetFuncParamList(func, true))
	hFile:write "{\n"
	hFile:inc()
	hFile:fmt('%s = (%s)%s("%s%s");\n',
		GetFuncPtrName(func, spec, options),
		GetFuncPtrTypedefName(func, spec, options),
		spec.GetPtrLoaderFuncName(),
		spec.FuncNamePrefix(),
		func.name)
		
	if(common.DoesFuncReturnSomething(func)) then
		hFile:fmt('%s(%s);\n',
			GetFuncPtrName(func, spec, options),
			common.GetFuncParamCallList(func))
	else
		hFile:fmt('return %s(%s);\n',
			GetFuncPtrName(func, spec, options),
			common.GetFuncParamCallList(func))
	end
	hFile:dec()
	hFile:write "}\n\n"
end

function switch.WriteGetExtString(hFile, specData, spec, options, funcSeen)
	if(funcSeen[spec.GetExtStringFuncName()]) then
		return
	end

	local func = specData.funcdefs[spec.GetExtStringFuncName()]
	if(func) then
		hFile:write "\n"
		hFile:fmt("static %s %s(%s)\n",
			common.GetFuncReturnType(func),
			func.name,
			common.GetFuncParamList(func, true))
		hFile:write "{\n"
		hFile:inc()
		hFile:fmt('%s = (%s)%s("%s%s");\n',
			GetFuncPtrName(func, spec, options),
			GetFuncPtrTypedefName(func, spec, options),
			spec.GetPtrLoaderFuncName(),
			spec.FuncNamePrefix(),
			func.name)
			
		if(common.DoesFuncReturnSomething(func)) then
			hFile:fmt('%s(%s);\n',
				GetFuncPtrName(func, spec, options),
				common.GetFuncParamCallList(func))
		else
			hFile:fmt('return %s(%s);\n',
				GetFuncPtrName(func, spec, options),
				common.GetFuncParamCallList(func))
		end
		hFile:dec()
		hFile:write "}\n\n"
	end
end

local init = {}
src.init = init

function init.WriteBlockBeginStruct(hFile, spec, options)
	hFile:write("struct InitializeVariables\n")
	hFile:write "{\n"
	hFile:inc()

	hFile:write("InitializeVariables()\n")
	hFile:write "{\n"
	hFile:inc()
end

function init.WriteBlockEndStruct(hFile, spec, options)
	hFile:dec()
	hFile:write "}\n"
	hFile:dec()
	hFile:write "};\n\n"
	hFile:write("InitializeVariables g_initVariables;\n")
end

function init.WriteFunction(hFile, func, spec, options, funcSeen)
	hFile:fmt("%s = Switch_%s;\n", func.name, func.name)
end


local function Create()
	return util.DeepCopyTable(my_style), struct
end

return { Create = Create }

