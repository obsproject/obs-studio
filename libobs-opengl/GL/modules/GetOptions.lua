--[[ The function GetOptions retrieves the list of extensions and other command-line options. It pulls data from a multitude of sources, though it begins with the command-line.

It takes the following parameters:
- An array of the command-line options.

It returns a table containing the following entries:
- spec: What specification will be generated. One of the following:
-		gl: Uses the OpenGL spec. Default
-		glX: Uses the glX spec.
-		wgl: Uses the WGL "spec".
- version: OpenGL version to export. All core features from that version and below will be exported. Will only be present when exporting "gl" loaders.
- profile: OpenGL profile to use. Default is chosen based on GL version. One of the following:
-		core: Default
-		compatibility:
- extensions: A list of OpenGL extensions to export.
- outname: The base filename of the file to create.
- style: A string containing the particular style of binding. This can be:
-		pointer_c: The default. The functions will be stored in pointers exposed to the user. #defines will be used to rename the pointers to the core GL function names.
-		pointer_cpp: The functions will be stored in pointers, but the pointers and enumerators will be placed in the namespace "gl".
- indent: A string that defines the indentation style for the output.
-		tab: Uses tabs. Default.
-		space: Uses 2 spaces.
- prefix: A prefix to be added to the names of identifiers that must be global, while avoiding name clashes. This is useful if you want to have different sets of bindings to different APIs (like a GL 3.3 and 2.1 binding). Defaults to the empty string.
]]

local cmd = require "CmdLineOptions"
local util = require "util"
local Styles = require "Styles"
local Specs = require "Specs"

local function FixupExtensionName(ext)
	return ext
end

local parseOpts = cmd.CreateOptionGroup()
parseOpts:enum(
	"spec",
	"spec",
	"Specification to use.",
	{"gl", "glX", "wgl"},
	1)
parseOpts:value(
	"version",
	"version",
	{"OpenGL version to export.", "Only use this with the 'gl' spec."},
	nil,
	true)
parseOpts:enum(
	"profile",
	"profile",
	{"OpenGL profile to use.", "Only use this with the 'gl' spec."},
	{"core", "compatibility"},
	1,
	true)
parseOpts:enum(
	"style",
	"style",
	{"Export style."},
	Styles.GetStyleList(),
	1)
parseOpts:enum(
	"indent",
	"indent",
	{"Indentation style."},
	{"tab", "space"},
	1)
parseOpts:array(
	"exts",
	"extensions",
	{"A list of extensions to export."},
	FixupExtensionName,
	true)
parseOpts:array_single(
	"ext",
	"extensions",
	{"A single extension name to export."},
	FixupExtensionName,
	true)
parseOpts:array_single(
	"extfile",
	"extfiles",
	{"A file to load extensions from.", "Files are always relative to the current directory."},
	nil,
	true)
parseOpts:array_single(
	"stdext",
	"stdexts",
	{"A file to load extensions from, within the ./extfiles directory.", "These are the standard extension files."},
	nil,
	true)
parseOpts:value(
	"prefix",
	"prefix",
	{
		"String to prefix to various globals. Set this to ",
		"prevent interference with multiple loaders."
	},
	"")
parseOpts:pos_opt(
	1,
	"outname",
	"Base filename (sans extension)",
	"outname")

local extFileLines;

local function LoadExtFile(extensions, extfilename, baseDir)
	if(baseDir) then
		extfilename = baseDir .. extfilename
	end
	local hFile = assert(io.open(extfilename, "r"), "Could not find the file " .. extfilename)
	
	for line in hFile:lines() do
		for _, test in ipairs(extFileLines) do
			local matches = {line:match(test.pttrn)}
			if(#matches ~= 0) then
				test.proc(extensions, baseDir, unpack(matches))
				break
			end
		end
	end
	
	hFile:close()
end

--Function gets the list of extensions, the base directory of the currently
--processing file, and whatever matches came from the pattern.
extFileLines =
{
	{
		pttrn = '^%s*%#include [%"](.+)[%"]',
		proc = function(extensions, basedir, file)
			local name, dir = util.ParsePath(file)
			if(baseDir and dir) then
				dir = baseDir .. dir
			elseif(baseDir) then
				dir = baseDir
			end
			
			file = name
			
			LoadExtFile(extensions, file, dir)
		end,
	},
	{
		pttrn = '^%s*%#include [%<](.+)[%>]',
		proc = function(extensions, basedir, file)
			local name, dir = util.ParsePath(SysRelPath(file))
			--Ignore the base directory; we start with the system directory.
			
			LoadExtFile(extensions, name, dir)
		end,
	},
	{
		pttrn = '^%s*%-%-',
		proc = function(extensions, basedir) --[[Ignore the line. Comment]] end,
	},
	{
		pttrn = '^%s*%/%/',
		proc = function(extensions, basedir) --[[Ignore the line. Comment]] end,
	},
	{
		pttrn = '(%S+)',
		proc = function(extensions, basedir, ext)
			table.insert(extensions, ext)
		end,
	},
}


local function FixupExtname(ext)
	--Cull the (W)GL(X)_ part of the name, if any.
	
	local bareName = ext:match("W?GLX?_(.+)")
	
	return bareName or ext
end

local optTbl = {}

function optTbl.GetOptions(cmd_line)
	local options, pos_args = parseOpts:ProcessCmdLine(cmd_line)
	
	local spec = Specs.GetSpec(options.spec)
	
	if(options.version) then
		--Check the version against the allowed versions.
		local versionTest = util.InvertTable(spec.GetCoreVersions())
		parseOpts:AssertParse(versionTest[options.version], "The version " .. options.version .. " is not a legal version number.")
	else
		--Check to see that no versions are offered.
		parseOpts:AssertParse(#spec.GetCoreVersions() == 0, "You must specify a version for the specification " .. options.spec)
	end
	
	spec.VerifyOptions(options, parseOpts)
	
	--Load and collate the extensions.
	options.extensions = options.extensions or {}
	options.extfiles = options.extfiles or {}
	options.stdexts = options.stdexts or {}
	
	for _, file in ipairs(options.extfiles) do
		LoadExtFile(options.extensions, util.ParsePath(file)) --vararg
	end
	
	--Local extension files.
	for _, file in ipairs(options.stdexts) do
		LoadExtFile(options.extensions, util.ParsePath(SysRelPath("extfiles/" .. file))) --vararg
	end
	
	--Fixup names and remove duplicates.
	local dups = {}
	local exts = {}
	for _, ext in ipairs(options.extensions) do
		local fixExt = FixupExtname(ext)
		if(not dups[fixExt]) then
			exts[#exts + 1] = fixExt
			dups[fixExt] = true
		end
	end
	
	options.extensions = exts
	options.extfiles = nil
	options.stdexts = nil

	return options
end

return optTbl
