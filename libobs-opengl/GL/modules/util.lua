
local TabbedFile = require "TabbedFile"

local util = {}

function util.GetSpecFilePath()
	return FixupPath("glspecs/");
end

function util.GetDataFilePath()
	return FixupPath("data/");
end

function util.InvertTable(tbl)
	local ret = {}
	for i, val in ipairs(tbl) do
		ret[val] = true
	end
	return ret
end

--Returns two values: the base filename and the directory.
--If the base filename is nil, it doesn't have a pathname.
--If the directory is nil, then there was no directory.
function util.ParsePath(pathname)
	local base = pathname:match("([^\\/]+)$")
	local dir = pathname:match("^(.*[\\/])")
	
	return base, dir
end

local function DeepCopyTable(tbl, destTbl)
	local ret = destTbl or {}
	for key, value in pairs(tbl) do
		if(type(value) == "table") then
			if(type(ret[key]) == "table") then
				ret[key] = DeepCopyTable(value, ret[key])
			else
				ret[key] = DeepCopyTable(value)
			end
		else
			ret[key] = value
		end
	end
	return ret
end

util.DeepCopyTable = DeepCopyTable

--Ensures the given path exists. Creates the directories when they don't.
--Note: Only works if LFS is available.
--`path` should end in a directory separator.
function util.EnsurePath(path)
	local status, lfs = pcall(require, "lfs")
	
	if(not status) then return end
	
	--strip the last directory separator off.
	path = path:match("^(.+)[/\\]$")
	
	local mode, err = lfs.attributes(path, "mode")
	if(not mode) then
		local creates = {}
		local currPath = path
		repeat
			table.insert(creates, 1, currPath)
			currPath = currPath:match("(.*[/\\])[^/\\]*$")
			if(currPath) then
				currPath = currPath:match("^(.+)[/\\]$")
				mode, err = lfs.attributes(currPath, "mode")
			end
		until(mode or currPath == nil)
		
		for _, newDir in ipairs(creates) do
			assert(lfs.mkdir(newDir))
		end
	end
end

function util.CreateFile(filename, indent)
	local base, dir = util.ParsePath(filename)
	util.EnsurePath(dir)
	local hFile = assert(io.open(filename, "w"))
	return TabbedFile.TabbedFile(hFile, indent)
end


return util
