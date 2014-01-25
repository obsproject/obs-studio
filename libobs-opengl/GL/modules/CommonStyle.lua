--[[Useful style utility functions. This file will contain commonly useful strings and functions that generate useful data.]]

local util = require "util"
local struct = require "Structure"

local common = {}

--Creates a tabbed file.
function common.CreateFile(filename, indent)
	return util.CreateFile(filename, indent)
end

--Retrieves the common typedefs used by OpenGL 1.1.
function common.GetStdTypedefs()
	return dofile(util.GetDataFilePath() .. "style_commontypedefs.lua")
end

--Writes passthru data from the specData, with proper indentation.
function common.WritePassthruData(hFile, strArray)
	for _, str in ipairs(strArray) do
		--unindent after #endif
		if(str:match("^#endif") or str:match("^#elif")) then
			hFile:dec()
		end
	
		hFile:write(str, "\n")
		
		--Indent after #if.
		if(str:match("^#if") or str:match("^#elif")) then
			hFile:inc()
		end
	end
end

function common.WriteExternCStart(hFile)
	hFile:write("#ifdef __cplusplus\n")
	hFile:write('extern "C" {\n')
	hFile:write("#endif /*__cplusplus*/\n")
	hFile:write("\n")
end

function common.WriteExternCEnd(hFile)
	hFile:write("#ifdef __cplusplus\n")
	hFile:write('}\n')
	hFile:write("#endif /*__cplusplus*/\n")
	hFile:write("\n")
end

--Determines the value of the enumeration.
local function ResolveEnumValue(enum, enumTable)
	if(enum.copy) then
		return common.ResolveEnumValue(enumTable[enum.value], enumTable),
			enum.value;
	else
		return enum.value;
	end
end
common.ResolveEnumValue = ResolveEnumValue

function common.GetCppEnumName(enum)
	--Note: some enumerators start with characters C++ forbids as initial
	--identifiers. If we detect such an enum, prefix it with `_`.
	local enumName = enum.name
	if(not enumName:match("^[a-zA-Z_]")) then
		enumName = "_" .. enumName
	end
	
	--Also, certain identifiers can need it because they conflict.
	local badIdent = {"TRUE", "FALSE", "NO_ERROR", "WAIT_FAILED", "DOMAIN"}
	for _, ident in ipairs(badIdent) do
		if(enumName == ident) then
			enumName = enumName .. "_"
			break
		end
	end
	
	return enumName
end

function common.GetNameLengthPadding(name, length)
	local numSpaces = length - #name
	if(numSpaces < 1) then
		numSpaces = 1
	end
	
	return string.rep(" ", numSpaces)
end

--Gets the return type for a function.
function common.GetFuncReturnType(func)
	return func["return_ctype"]
end

function common.DoesFuncReturnSomething(func)
	local returnType = func["return_ctype"]
	return (returnType == "void") or (returnType == "GLvoid")
end

local bIsKindPtr ={
	value = false,
	array = true,
	reference = true,
};

--Important due to name conflicts. Some names have to re-mapped to others.
--Won't really affect things.
local paramNameRemap = {
	near = "ren_near",
	far = "ren_far",
	array = "ren_array",
};

--Returns the parameter list as a string.
--Parameter list does not include parenthesis.
function common.GetFuncParamList(func, bWriteVarNames)
	local paramList = {}
	for i, param in ipairs(func.parameters) do
		local paramType = param.ctype;
		if(bWriteVarNames) then
			local paramName = param.name
			if(paramNameRemap[paramName]) then paramName = paramNameRemap[paramName]end
			paramList[#paramList + 1] = string.format("%s %s", paramType, paramName)
		else
			paramList[#paramList + 1] = paramType
		end
	end
	
	return table.concat(paramList, ", ");
end

--Get the list of parameter names, as a string ready to be put into ().
function common.GetFuncParamCallList(func)
	local paramList = {}
	for i, param in ipairs(func.parameters) do
		local paramName = param.name
		if(paramNameRemap[paramName]) then
			paramName = paramNameRemap[paramName]
		end

		paramList[#paramList + 1] = paramName
	end
	
	return table.concat(paramList, ", ");
end

--Retrieves the name of the function according to OpenGL.
function common.GetOpenGLFuncName(func, spec)
	return spec.FuncNamePrefix() .. func.name
end

function common.GetProcAddressName(spec)
	return spec.GetPtrLoaderFuncName()
end

function common.FixupIndexedList(specData, indexed)
	assert(indexed)
	for _, func in ipairs(specData.funcData.functions) do
		if(indexed[1] == func.name) then
			indexed[1] = func
		end
		if(indexed[3] == func.name) then
			indexed[3] = func
		end
	end
	for _, enum in ipairs(specData.enumerators) do
		if(indexed[2] == enum.name) then
			indexed[2] = enum
		end
		if(indexed[4] == enum.name) then
			indexed[4] = enum
		end
	end
end

function common.GetProcExtsFromExtListFunc(hFile, specData, spec, options,
							indexed, GetFuncPtrName, GetEnumName)
	return [[
static void ProcExtsFromExtList()
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
]]
end

--You give it a function that takes a const char*.
function common.GetProcessExtsFromStringFunc(funcFormat, arguments)
	return [[
static void ProcExtsFromExtString(const char *strExtList]] .. (arguments or "") .. [[)
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

		]] .. funcFormat:format("strWorkBuff") ..[[;

		strCurrPos = strEndStr + 1;
		if(iStop) break;
	}
}
]]
end

function common.WriteProcessExtsFromStringFunc(hFile, ...)
	hFile:writeblock(common.GetProcessExtsFromStringFunc(...))
end

function common.GetParseVersionFromString()
	return [[
static void ParseVersionFromString(int *pOutMajor, int *pOutMinor, const char *strVersion)
{
	const char *strDotPos = NULL;
	int iLength = 0;
	char strWorkBuff[10];
	*pOutMinor = 0;
	*pOutMajor = 0;

	strDotPos = strchr(strVersion, '.');
	if(!strDotPos)
		return;

	iLength = (int)((ptrdiff_t)strDotPos - (ptrdiff_t)strVersion);
	strncpy(strWorkBuff, strVersion, iLength);
	strWorkBuff[iLength] = '\0';

	*pOutMajor = atoi(strWorkBuff);
	strDotPos = strchr(strVersion + iLength + 1, ' ');
	if(!strDotPos)
	{
		/*No extra data. Take the whole rest of the string.*/
		strcpy(strWorkBuff, strVersion + iLength + 1);
	}
	else
	{
		/*Copy only up until the space.*/
		int iLengthMinor = (int)((ptrdiff_t)strDotPos - (ptrdiff_t)strVersion);
		iLengthMinor = iLengthMinor - (iLength + 1);
		strncpy(strWorkBuff, strVersion + iLength + 1, iLengthMinor);
		strWorkBuff[iLengthMinor] = '\0';
	}

	*pOutMinor = atoi(strWorkBuff);
}
]]
end

local function DeepCopyTable(tbl)
	local ret = {}
	for key, value in pairs(tbl) do
		if(type(value) == "table") then
			ret[key] = DeepCopyTable(value)
		else
			ret[key] = value
		end
	end
	return ret
end

function common.WriteCMappingTable(hFile, specData, spec,
							options, structName, varName, GetExtVariableName, GetExtLoaderFuncName)
	--Write the struct for the mapping table.
	hFile:write("typedef int (*PFN_LOADFUNCPOINTERS)();\n")
	hFile:fmt("typedef struct %s%sStrToExtMap_s\n",
		options.prefix, spec.DeclPrefix())
	hFile:write("{\n")
	hFile:inc()
	hFile:write("char *extensionName;\n")
	hFile:write("int *extensionVariable;\n")
	hFile:write("PFN_LOADFUNCPOINTERS LoadExtension;\n")
	hFile:dec()
	hFile:fmt("} %s;\n", structName)
	hFile:write "\n"
	
	--Write the mapping table itself.
	local arrayLength = #options.extensions
	if(arrayLength == 0) then arrayLength = 1 end
	hFile:fmt("static %s %s[%i] = {\n",
		structName,
		varName,
		arrayLength)
	hFile:inc()
	for _, extName in ipairs(options.extensions) do
		if(#specData.extdefs[extName].funcs > 0) then
			hFile:fmt('{"%s", &%s, %s},\n',
				spec.ExtNamePrefix() .. extName,
				GetExtVariableName(extName, spec, options),
				GetExtLoaderFuncName(extName, spec, options))
		else
			hFile:fmt('{"%s", &%s, NULL},\n',
				spec.ExtNamePrefix() .. extName,
				GetExtVariableName(extName, spec, options))
		end
	end
	--Because C is stupid, write bogus entry.
	if(#options.extensions == 0) then
		hFile:fmt('{"", NULL, NULL},\n')
	end
	hFile:dec()
	hFile:write("};\n")

	hFile:write "\n"
	hFile:fmt("static int g_extensionMapSize = %i;\n", #options.extensions);
end

function common.WriteCFindExtEntryFunc(hFile, specData, spec,
							options, structName, varName, sizeName)
	hFile:fmt("static %s *FindExtEntry(const char *extensionName)\n",
		structName)
	hFile:write("{\n")
	hFile:inc()
	hFile:write("int loop;\n")
	hFile:fmt("%s *currLoc = %s;\n",
		structName,
		varName)
	hFile:writeblock([[
for(loop = 0; loop < g_extensionMapSize; ++loop, ++currLoc)
{
	if(strcmp(extensionName, currLoc->extensionName) == 0)
		return currLoc;
}

return NULL;
]])
	hFile:dec()
	hFile:write("}\n")
end

function common.WriteCClearExtensionVarsFunc(hFile, specData, spec,
							options, GetExtVariableName, clearValue)
	hFile:fmt("static void ClearExtensionVars()\n")
	hFile:write("{\n")
	hFile:inc()
	for _, extName in ipairs(options.extensions) do
		hFile:fmt('%s = %s;\n',
			GetExtVariableName(extName, spec, options),
			clearValue)
	end
	hFile:dec()
	hFile:write("}\n")
	hFile:write "\n"
end

--Write a function that loads an extension by name. It is called when
--processing, so it should also set the extension variable based on the load.
function common.WriteCLoadExtByNameFunc(hFile, specData, spec,
							options, structName, successValue)
	hFile:writeblock([[
static void LoadExtByName(const char *extensionName)
{
	]] .. structName .. [[ *entry = NULL;
	entry = FindExtEntry(extensionName);
	if(entry)
	{
		if(entry->LoadExtension)
		{
			int numFailed = entry->LoadExtension();
			if(numFailed == 0)
			{
				*(entry->extensionVariable) = ]] ..
				successValue ..
				[[;
			}
			else
			{
				*(entry->extensionVariable) = ]] ..
				successValue ..
				[[ + numFailed;
			}
		}
		else
		{
			*(entry->extensionVariable) = ]] ..
			successValue ..
			[[;
		}
	}
}
]])
end

function common.WriteNamespaceBegin(hFile, namespace)
	hFile:fmt("namespace %s\n", namespace)
	hFile:write("{\n")
	hFile:inc()
end

function common.WriteNamespaceEnd(hFile)
	hFile:dec()
	hFile:write("}\n")
end


common.DeepCopyTable = DeepCopyTable

local my_struct =
{
	{ type="file", style="header", name="GetFilename",
		{ type="write", name="FilePreamble", optional=true} ,
		{ type="block", name="IncludeGuard(hFile, spec, options)",
			{ type="blank"},
			{ type="write", name="Init(hFile, spec, options)", },
			{ type="blank"},
			{ type="write", name="StdTypedefs(hFile, specData, options)",},
			{ type="blank"},
			{ type="write", name="SpecTypedefs(hFile, specData, options)",},
			{ type="blank"},
			{ type="block", name="Decl(hFile, spec, options)",
				{ type="block", name="ExtVarDecl(hFile, spec, options)",
					{ type="ext-iter",
						{ type="write", name="ExtVariableDecl(hFile, extName, specData, spec, options)" },
						{ type="blank", last=true },
					},
				},
				{ type="block", name="EnumDecl(hFile, spec, options)",
					{ type="enum-seen",
						{ type="ext-iter",
							{type="enum-iter",
								{ type="write", name="EnumDecl(hFile, enum, enumTable, spec, options, enumSeen)", },
								{ type="blank", last=true },
							},
						},
						{ type="version-iter",
							{type="enum-iter",
								{ type="write", name="EnumDecl(hFile, enum, enumTable, spec, options, enumSeen)", },
								{ type="blank", last=true },
							},
						},
					},
				},
				{ type="block", name="FuncDecl(hFile, spec, options)",
					{ type="func-seen",
						{ type="ext-iter",
							{ type="block", name="ExtFuncDecl(hFile, extName, spec, options)", cond="func-iter",
								{type="func-iter",
									{ type="write", name="FuncDecl(hFile, func, spec, options, funcSeen)", },
								},
							},
							{ type="blank"},
						},
						{ type="version-iter",
							{type="func-iter",
								{ type="write", name="FuncDecl(hFile, func, spec, options, funcSeen)", },
								{ type="blank", last=true },
							},
						},
					},
				},
				{ type="block", name="SysDecl(hFile, spec, options)",
					{ type="write", name="UtilityDecls(hFile, spec, options)",},
					{ type="blank" },
					{ type="write", name="MainLoaderFuncDecl(hFile, spec, options)",},
					{ type="blank" },
					{ type="write", name="VersioningFuncDecls(hFile, spec, options)",},
					{ type="blank" },
				},
			},
		},
	},
	{ type="file", style="source", name="GetFilename",
		{ type="write", name="FilePreamble", optional=true} ,
		{ type="write", name="Includes(hFile, basename, spec, options)",},
		{ type="blank"},
		{ type="write", name="LoaderData(hFile, spec, options)",},
		{ type="blank"},
		{ type="block", name="Def(hFile, spec, options)",
			{ type="block", name="ExtVarDef(hFile, spec, options)",
				{ type="ext-iter",
					{ type="write", name="ExtVariableDef(hFile, extName, specData, spec, options)",},
					{ type="blank", last=true},
				},
			},
			{ type="func-seen",
				{ type="ext-iter",
					{ type="block", name="ExtFuncDef(hFile, extName, spec, options)", cond="func-iter",
						{ type="func-iter",
							{ type="write", name="FuncDef(hFile, func, spec, options, funcSeen)", },
						},
						{ type="blank"},
						{ type="block", name="ExtLoader(hFile, extName, spec, options)",
							{ type="func-iter",
								{ type="write", name="ExtFuncLoader(hFile, func, spec, options)", }
							}
						},
						{ type="blank"},
					},
				},
				{ type="block", name="CoreFuncDef(hFile, spec, options)",
					cond="core-funcs",
					{ type="version-iter",
						{type="func-iter",
							{ type="write", name="FuncDef(hFile, func, spec, options, funcSeen)", },
							{ type="blank", last=true },
						},
					},
					{ type="block", name="CoreLoader(hFile, spec, options)",
						{ type="version-iter",
							{type="func-iter",
								{ type="write", name="CoreFuncLoader(hFile, func, spec, options)", },
							},
						},
					},
					{ type="blank"},
				},
				{ type="write", name="ExtStringFuncDef(hFile, specData, spec, options, funcSeen)"},
			},
			{ type="block", name="SysDef(hFile, spec, options)",
				{ type="write", name="UtilityDefs(hFile, specData, spec, options)",},
				{ type="blank" },
				{ type="write", name="MainLoaderFunc(hFile, specData, spec, options)",},
				{ type="blank" },
				{ type="write", name="VersioningFuncs(hFile, specData, spec, options)", cond="version-iter"},
				{ type="blank", cond="version-iter" },
			},
		},
	},
}


my_struct = struct.BuildStructure(my_struct)

function common.GetStandardStructure()
	return my_struct
end

return common
