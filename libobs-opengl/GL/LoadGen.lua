assert(arg and arg[0], "You ran this script incorrectly.")

--Get the location of our modules relative to here.
local baseDir = arg[0]:match("^(.*[\\/])")
baseDir = baseDir or "./"
assert(baseDir, arg[0] .. " No directory")

--Fixup the package path to be relative to this directory.
package.path = baseDir .. "modules/?.lua;" .. package.path

--Make a function to get the correct directory name.
function SysRelPath(relativePath)
	return baseDir .. relativePath
end
FixupPath = SysRelPath --Older name.

local opts = require "GetOptions"
local Specs = require "Specs"
local Styles = require "Styles"
local LoadSpec = require "LoadLuaSpec"
local util = require "util"

--Get the options.
local status, options = pcall(opts.GetOptions, arg)

if(not status) then
	io.stdout:write(options, "\n")
	return
end

--Load the spec data.
local spec = Specs.GetSpec(options.spec)
local specData = spec.LoadSpec()

--Verify that every extension in `options.extensions` is a real extension.
local badExts = {}
for _, extName in ipairs(options.extensions) do
	if(not specData.extdefs[extName]) then
		badExts[#badExts + 1] = extName
	end
end

if(#badExts > 0) then
	io.stdout:write("The following extensions are not in the spec ", options.spec, ":\n")
	for _, extName in ipairs(badExts) do
		io.stdout:write("\t", extName, "\n")
	end
	return
end

--Extract the path and base-filename from the filename.
local simplename, dir = util.ParsePath(options.outname)
dir = dir or "./"

assert(simplename,
	"There is no filename in the path '" .. options.outname .. "'")

local style, structure = Styles.GetStyle(options.style)

--Compute the filename, minus style-specific suffix.
local basename = dir .. spec:FilePrefix() .. simplename

structure.Proc(basename, style, specData, spec, options)


