local util = require "util"
local struct = require "NoloadCpp_Struct"
local common = require "CommonStyle"

--------------------------------------
-- Common functions.
local function GetIncludeGuard(spec, options)
	local temp = 
		options.prefix .. spec.GetIncludeGuardString() .. "_NOLOAD_STYLE_HPP"
	return temp:upper()
end

local function GetFuncPtrName(func, spec, options)
	return func.name
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

function my_style.WriteBlockBeginMainNamespace(hFile, spec, options)
	if(#options.prefix > 0) then
		common.WriteNamespaceBegin(hFile, options.prefix)
	end
	
	common.WriteNamespaceBegin(hFile, spec.FuncNamePrefix())
end

function my_style.WriteBlockEndMainNamespace(hFile, spec, options)
	common.WriteNamespaceEnd(hFile, spec.FuncNamePrefix())
	
	if(#options.prefix > 0) then
		common.WriteNamespaceEnd(hFile, options.prefix)
	end
end

function my_style.WriteBlockBeginExtVariables(hFile, spec, options)
	common.WriteNamespaceBegin(hFile, "exts")
end

function my_style.WriteBlockEndExtVariables(hFile, spec, options)
	common.WriteNamespaceEnd(hFile, "exts")
end

function my_style.WriteBlockBeginSystem(hFile, spec, options)
	common.WriteNamespaceBegin(hFile, "sys")
end

function my_style.WriteBlockEndSystem(hFile, spec, options)
	common.WriteNamespaceEnd(hFile, "sys")
end


---------------------------------------------
-- Header functions.
local hdr = {}
my_style.hdr = hdr

function hdr.GetFilename(basename, spec, options)
	return basename .. ".hpp"
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
	hFile:fmt("extern bool var_%s;\n", extName)
end

function hdr.WriteBlockBeginEnumerators(hFile, spec, options)
	hFile:writeblock("enum\n{\n")
	hFile:inc()
end

function hdr.WriteBlockEndEnumerators(hFile, spec, options)
	hFile:dec()
	hFile:write("};\n")
end

function hdr.WriteEnumerator(hFile, enum, enumTable, spec, options, enumSeen)
	if(enumSeen[enum.name]) then
		hFile:fmt("//%s seen in %s\n", enum.name, enumSeen[enum.name])
	else
		local name = common.GetCppEnumName(enum)
		hFile:fmt("%s%s= %s,\n",
			name,
			common.GetNameLengthPadding(name, 33),
			common.ResolveEnumValue(enum, enumTable))
	end
end

function hdr.WriteFunction(hFile, func, spec, options, funcSeen)
	if(funcSeen[func.name]) then return end
	
	hFile:write("extern ", GetFuncPtrDefDirect(func, spec, options), ";\n")
end

function hdr.WriteSetupFunction(hFile, specData, spec, options)
	hFile:fmt("void CheckExtensions(%s);\n", spec.GetLoaderParams())
end

function hdr.WriteVersionFunctions(hFile, specData, spec, options)
end


----------------------------------------
-- Source file.
local src = {}
my_style.src = src

function src.GetFilename(basename, spec, options)
	return basename .. ".cpp"
end

function src.WriteIncludes(hFile, basename, spec, options)
	hFile:writeblock([[
#include <algorithm>
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
	hFile:fmt("bool var_%s = false;\n", extName)
end

function src.WriteSetupFunction(hFile, specData, spec, options)
	common.WriteNamespaceBegin(hFile, "")
	
	hFile:write "void ClearExtensionVariables()\n"
	hFile:write "{\n"
	hFile:inc()
	
	for _, extName in ipairs(options.extensions) do
		hFile:fmt("exts::var_%s = false;\n", extName)
	end
	
	hFile:dec()
	hFile:write "}\n"
	hFile:write "\n"
	
	hFile:writeblock[[
struct MapEntry
{
	const char *extName;
	bool *extVariable;
};

struct MapCompare
{
	MapCompare(const char *test_) : test(test_) {}
	bool operator()(const MapEntry &other) { return strcmp(test, other.extName) == 0; }
	const char *test;
};

struct ClearEntry
{
  void operator()(MapEntry &entry) { *(entry.extVariable) = false;}
};

]]
	local arrayLength = #options.extensions
	if(arrayLength == 0) then arrayLength = 1 end

	hFile:fmt("MapEntry g_mappingTable[%i]", arrayLength)
	if(arrayLength == 1) then
		hFile:rawwrite "; //This is intensionally left uninitialized. \n"
	else
		hFile:rawwrite " =\n"
		hFile:write "{\n"
		hFile:inc()
		for _, extName in ipairs(options.extensions) do
			hFile:fmt('{"%s%s", &exts::var_%s},\n',
				spec.ExtNamePrefix(),
				extName,
				extName)
		end
		hFile:dec()
		hFile:write "};\n"
	end
	
	hFile:write "\n"
	hFile:fmtblock([[
void LoadExtByName(const char *extensionName)
{
	MapEntry *tableEnd = &g_mappingTable[%i];
	MapEntry *entry = std::find_if(&g_mappingTable[0], tableEnd, MapCompare(extensionName));
	
	if(entry != tableEnd)
		*(entry->extVariable) = true;
}
]], #options.extensions)

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
	]] .. indexed[1].name
	.. [[(]] .. indexed[2].name
	.. [[, &iNumExtensions);

	for(iLoop = 0; iLoop < iNumExtensions; iLoop++)
	{
		const char *strExtensionName = (const char *)]] ..
		indexed[3].name ..
		[[(]] .. indexed[4].name .. [[, iLoop);
		LoadExtByName(strExtensionName);
	}
}
]])
	else
		hFile:writeblock(
			common.GetProcessExtsFromStringFunc("LoadExtByName(%s)"))
	end

	common.WriteNamespaceEnd(hFile, "")
	
	hFile:fmt("void CheckExtensions(%s)\n", spec.GetLoaderParams())
	hFile:write "{\n"
	hFile:inc()
	hFile:write "ClearExtensionVariables();\n"
	hFile:fmt("std::for_each(&g_mappingTable[0], &g_mappingTable[%i], ClearEntry());\n", #options.extensions)
	hFile:write "\n"
	if(indexed) then
		hFile:write("ProcExtsFromExtList();\n")
	else
		--First, check if the GetExtStringFuncName is in the specData.
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
			hFile:write "\n"
		end
		
		hFile:fmt("ProcExtsFromExtString((const char *)%s(%s));\n",
			funcName,
			spec.GetExtStringParamList(
				function (name) return spec.FuncNamePrefix() .. "::" .. name end))
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
end

local defs = {}
src.defs = defs

function defs.WriteFunction(hFile, func, spec, options, funcSeen)
	if(funcSeen[func.name]) then return end
	hFile:write(GetFuncPtrDefTypedef(func, spec, options), ";\n")
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
	common.WriteNamespaceBegin(hFile, "")
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
	common.WriteNamespaceEnd(hFile, "")
end

function init.WriteFunction(hFile, func, spec, options, funcSeen)
	if(funcSeen[func.name]) then return end

	hFile:fmt("%s = Switch_%s;\n", func.name, func.name)
end


local function Create()
	return util.DeepCopyTable(my_style), struct
end

return { Create = Create }

