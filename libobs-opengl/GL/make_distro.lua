--[[
Will automatically generate all files from the current sources.
Takes three parameters:
- The destination directory, as a relative directory. This will create that directory and put the distro in that directory
- The Mercurial revision number to archive to the destination directory.
- The version number of the loader.
]]--

require "lfs"
require "ufs"
require "_FindFileInPath"

local baseDir, hgChangelist, versionNum = ...

if(#({...}) ~= 3) then
	print("Not enough commandline parameters. You provided: " .. #({...}));
	print("Paramters:")
	print("\tDestination dir, relative to this path.")
	print("\tMercurial revision to archive.")
	print("\tVersion number of the SDK.")
	return
end

local buildDirname = "glLoadGen_" .. versionNum:gsub("%.", "_")

lfs.mkdir(baseDir);
local pathDestDir = ufs.path(baseDir) / buildDirname;
local destDir = tostring(pathDestDir);
lfs.mkdir(destDir);

local pathCurrent = ufs.current_path()
local pathDest = pathCurrent / destDir;
local pathBase = pathCurrent / baseDir;

-----------------------------------------------------------
-- Step 1: Copy the Mercurial repo number to the location.

local clone = [[hg archive -r "%s" "%s"]];
clone = clone:format(hgChangelist, destDir);

print(clone);
os.execute(clone);

------------------------------------------------------------
-- Step 2: Delete select files from the destination location.
local toDelete =
{
	--files
	"make_distro.lua", ".hgignore", ".hgtags",
	"_FindFileInPath.lua", ".hg_archival.txt",
	--directories
}


for i, filename in ipairs(toDelete) do
	local pathFile = pathDest / filename;
	print("deleting:", pathFile);
	ufs.remove_all(pathFile);
end

------------------------------------------------------------
-- Step 3: Create Zip archive of the distro.
local szFilename = "7z.exe"
local archiveName = buildDirname .. ".7z"
local pathSZ = ufs.path(FindFileInPath(szFilename))

if(pathSZ:empty()) then
	print("Could not find 7zip.");
	return;
end

ufs.current_path(pathBase);

local depProc = ex.spawn(tostring(pathSZ),
	{args={"a", "-r", archiveName, buildDirname}});
depProc:wait(depProc);

------------------------------------------------------------
-- Step 4: Destroy the directory.
ufs.remove_all(pathDest);
