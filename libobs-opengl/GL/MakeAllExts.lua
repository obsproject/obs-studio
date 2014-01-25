assert(arg and arg[0], "You ran this script incorrectly.")

--Get the location of our modules relative to here.
local baseDir = arg[0]:match("^(.*[\\/])")
baseDir = baseDir or "./"

--Fixup the package path to be relative to this directory.
package.path = baseDir .. "modules/?.lua;" .. package.path

--Make a function to get the correct directory name.
function SysRelPath(relativePath)
	return baseDir .. relativePath
end
FixupPath = SysRelPath --Older name.

local cmd = require "CmdLineOptions"
local Specs = require "Specs"

local parseOpts = cmd.CreateOptionGroup()
parseOpts:enum(
	"spec",
	"spec",
	"Specification to use.",
	{"gl", "glX", "wgl"})
parseOpts:pos_opt(
	1,
	"outname",
	"Output filename to generate.",
	"outname")

local options, pos_args = parseOpts:ProcessCmdLine(arg)

local dups = {}
local exts = {}

local spec = Specs.GetSpec(options.spec)
local specData = spec.LoadSpec()

local coreExts = spec.GetCoreExts()

for _, version in ipairs(specData.versions or {}) do
	if(coreExts[version]) then
		for _, extName in ipairs(coreExts[version]) do
			if(not dups[extName]) then
				exts[#exts + 1] = extName
				dups[extName] = true
			end
		end
	end
end

local sortExts = {}

for _, extName in ipairs(specData.extensions) do
	if(not dups[extName]) then
		sortExts[#sortExts + 1] = extName
		dups[extName] = true
	end
end

table.sort(sortExts)

local hFile = io.open(options.outname, "w")
for _, extName in ipairs(exts) do
	hFile:write(extName, "\n")
end
for _, extName in ipairs(sortExts) do
	hFile:write(extName, "\n")
end
hFile:close()

	