--[[ This module contains all of the spec-specific constructs (except where specs and styles overlap. That is, where styles have to do spec-specific work).

This module has a function called GetSpec which is given the spec name and returns a table containing functions/data that can be evaluated to do different jobs. This "class" contains:

- FilePrefix: nullary function that returns the filename prefix for this spec type.

- PlatformSetup: Takes a file and writes out platform-specific setup stuff.

- GetHeaderInit: Nullary function that returns a string to be written to the beginning of a header, just after the include guards.

- DeclPrefix: nullary function that returns the name of a prefix string for declarations.
]]

local util = require "util"
local LoadSpec = require "LoadLuaSpec"


local gl_spec = {}
local wgl_spec = {}
local glx_spec = {}

local specTbl =
{
	gl = gl_spec,
	wgl = wgl_spec,
	glX = glx_spec,
}

-------------------------------------------------
-- Spec-specific functions.
--Validate the options.
function gl_spec.VerifyOptions(options, parseOpts)
	if(options.profile == "compatibility") then
		parseOpts:AssertParse(tonumber(options.version) >= 3.0, "The OpenGL compatibility profile cannot be used with version " .. options.version)
	end
end

function wgl_spec.VerifyOptions(options, parseOpts)
	return "wgl_"
end
function glx_spec.VerifyOptions(options, parseOpts)
	return "glx_"
end

--Loads the appropriate Lua .spec file.
function gl_spec.LoadSpec() return LoadSpec.LoadLuaSpec(util.GetSpecFilePath() .. "glspec.lua", gl_spec) end
function wgl_spec.LoadSpec() return LoadSpec.LoadLuaSpec(util.GetSpecFilePath() .. "wglspec.lua", wgl_spec) end
function glx_spec.LoadSpec() return LoadSpec.LoadLuaSpec(util.GetSpecFilePath() .. "glxspec.lua", glx_spec) end

--Name for displaying.
function gl_spec.DisplayName() return "OpenGL" end
function wgl_spec.DisplayName() return "WGL" end
function glx_spec.DisplayName() return "GLX" end

---FilePrefix
function gl_spec.FilePrefix() return "gl_" end
function wgl_spec.FilePrefix() return "wgl_" end
function glx_spec.FilePrefix() return "glx_" end

--Include-guard string.
function gl_spec.GetIncludeGuardString() return "OPENGL" end
function wgl_spec.GetIncludeGuardString() return "WINDOWSGL" end
function glx_spec.GetIncludeGuardString() return "GLXWIN" end

--Declaration prefix.
function gl_spec.DeclPrefix() return "ogl_" end
function wgl_spec.DeclPrefix() return "wgl_" end
function glx_spec.DeclPrefix() return "glx_" end

--Extension name prefix.
function gl_spec.ExtNamePrefix() return "GL_" end
function wgl_spec.ExtNamePrefix() return "WGL_" end
function glx_spec.ExtNamePrefix() return "GLX_" end

--Enumerator name prefix. This is for defining "proper" GL enumerators.
function gl_spec.EnumNamePrefix() return "GL_" end
function wgl_spec.EnumNamePrefix() return "WGL_" end
function glx_spec.EnumNamePrefix() return "GLX_" end

--Function name prefix. This is for defining "proper" GL function names.
function gl_spec.FuncNamePrefix() return "gl" end
function wgl_spec.FuncNamePrefix() return "wgl" end
function glx_spec.FuncNamePrefix() return "glX" end

--Parameters given to the loader. No (), just the internals.
function gl_spec.GetLoaderParams() return "" end
function wgl_spec.GetLoaderParams() return "HDC hdc" end
function glx_spec.GetLoaderParams() return "Display *display, int screen" end

--CodeGen function pointer type. For APIFUNC and so forth.
function gl_spec.GetCodegenPtrType() return "CODEGEN_FUNCPTR" end
function wgl_spec.GetCodegenPtrType() return "CODEGEN_FUNCPTR" end
function glx_spec.GetCodegenPtrType() return "CODEGEN_FUNCPTR" end

--Name of the function that loads pointers
function gl_spec.GetPtrLoaderFuncName() return "IntGetProcAddress" end
function wgl_spec.GetPtrLoaderFuncName() return "IntGetProcAddress" end
function glx_spec.GetPtrLoaderFuncName() return "IntGetProcAddress" end

--Name of extension string function. Also returns true if this function needs to be loaded. If false is returned, then use the string name *exactly as is*.
function gl_spec.GetExtStringFuncName() return "GetString", true end
function wgl_spec.GetExtStringFuncName() return "GetExtensionsStringARB", true end
function glx_spec.GetExtStringFuncName() return "glXQueryExtensionsString", false end

--Gets the list of parameters that the extension string function will use. No (), just the internals. Pass a function used to resolve enumerator names into actual enumerator identifiers.
function gl_spec.GetExtStringParamList(enumResolve)
	return enumResolve("EXTENSIONS")
end
function wgl_spec.GetExtStringParamList(enumResolve) return "hdc" end
function glx_spec.GetExtStringParamList(enumResolve) return "display, screen" end

--Returns a table if it should use the indexed extension string functions. Returns false/nil otherwise.
-- The table is an array of:
--	Function name used to get the number of extensions.
--	Enumerator name used to get the number of extensions.
--	Function name used to get an extension string.
--	Enumerator name used to get an extension string.
function gl_spec.GetIndexedExtStringFunc(options)
	if(tonumber(options.version) >= 3.0) then
		return {"GetIntegerv", "NUM_EXTENSIONS", "GetStringi", "EXTENSIONS"}
	end
	return nil
end
function wgl_spec.GetIndexedExtStringFunc(options) return nil end
function glx_spec.GetIndexedExtStringFunc(options) return nil end

local fileProps =
{
	{"GetHeaderInit", "init"},
--	{"GetVersions", "versions"},
	{"GetCoreVersions", "coreversions"},
	{"GetCoreExts", "coreexts"},
	{"GetLoaderFunc", "loaderfunc"},
}

--Header initialization.
for key, spec in pairs(specTbl) do
	for _, props in ipairs(fileProps) do
		spec[props[1]] = function()
			return dofile(util.GetDataFilePath() .. spec:FilePrefix() ..
				"spec" .. props[2] .. ".lua")
		end
	end
end


--------------------------------------------------
-- Spec retrieval machinery
local function CopyTable(tbl)
	local ret = {}
	for key, value in pairs(tbl) do
		ret[key] = value
	end
	return ret
end

local function GetSpec(spec)
	local spec_tbl = specTbl[spec]
	assert(spec_tbl, "Unknown specification " .. spec)
	return CopyTable(spec_tbl)
end

return { GetSpec = GetSpec }
