
require("ex")
require "ufs"

function FindFileInPath(filename)
	local path = ex.getenv("PATH");

	for pathname in path:gmatch("([^%;%\"]+)%;?") do
		pathname = ufs.path(pathname);
		
		local testName = pathname / filename;
		
		if(ufs.exists(testName)) then
			return tostring(testName);
		end
	end
	
	return nil;
end


