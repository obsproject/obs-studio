--[[
The function, LoadLuaSpec exposed here will load a spec.lua file. It takes a filename
pointing to a spec.lua file.

This function will also add some features to the object before returning it.

The format will be as defined in LuaFormat.md, with the following addendums:

- enumtable: A table of enumerators, indexed by their names.
- functable: A table of functions, indexed by their names.
- extdefs: This is a table of extensions, indexed by extension name.
	Each entry contains:
-- enums: An array of enumerators. These enumerators are the entries in the main enum array.
-- funcs: An array of functions. These functions are the entries in the main funcData array.
- core_profiledefs: This is a table of core versions, listing all the funcs/enums in the core profile of the spec.
-- enums: An array of enumerators. These enumerators are the entries in the main enum array.
-- funcs: An array of functions. These functions are the entries in the main funcData array.
- compat_profiledefs: This is a table of core versions, listing all the funcs/enums in the compatibility profile of the spec. As per `coredefs`.


All enumerators and functions are found in one of these two lists. Some of them are in both.

Other changes are:
- Fixes for certain extensions. Certain extensions are non-core, but the enums and functions have no suffixes as if they were core.
]]

local util = require "util"


local load = {}

function load.LoadLuaSpec(luaFilename, spec)

	local listOfCoreVersions = spec.GetCoreVersions()

	local specData = dofile(luaFilename)

	specData.extdefs = {}
	specData.coredefs = {}
	specData.enumtable = {}
	specData.functable = {}
	local extdefs = specData.extdefs
	local coredefs = specData.coredefs
	local enumtable = specData.enumtable
	local functable = specData.functable
	
	local function GetCore(version)
		if(not coredefs[version]) then
			local coredef = {}
			coredefs[version] = coredef
			coredef.enums = {}
			coredef.funcs = {}
		end
		
		return coredefs[version]
	end
	
	local function GetExt(extName)
		if(not extdefs[extName]) then
			local extdef = {}
			extdefs[extName] = extdef
			extdef.enums = {}
			extdef.funcs = {}
		end
		
		return extdefs[extName]
	end

	--Add all extensions to the extdefs.
	for	i, extName in ipairs(specData.extensions) do
		GetExt(extName)
	end
	
	for i, enum in ipairs(specData.enumerators) do
		if(not enum.name) then print(enum.value) end
		enumtable[enum.name] = enum
		
		if(enum.extensions) then
			for k, extName in ipairs(enum.extensions) do
				table.insert(GetExt(extName).enums, enum)
			end
		end
		
		if(enum.core) then
			for _, coreSpec in ipairs(enum.core) do
				if(coreSpec[2] == "core") then
					table.insert(GetCore(coreSpec[1]).enums, enum)
					break
				end
			end
		end
	end

	for i, func in ipairs(specData.funcData.functions) do
		functable[func.name] = func

		if(func.extensions) then
			for k, extName in ipairs(func.extensions) do
				table.insert(GetExt(extName).funcs, func)
			end
		end
	
		if(func.core) then
			for _, coreSpec in ipairs(func.core) do
				if(coreSpec[2] == "core") then
					table.insert(GetCore(coreSpec[1]).funcs, func)
					break
				end
			end
		end
	end
	
	--Sort functions and enums by name. Just for nicer presentation.
	for extName, data in pairs(extdefs) do
		table.sort(data.enums, function(lhs, rhs) return lhs.name < rhs.name end)
		table.sort(data.funcs, function(lhs, rhs) return lhs.name < rhs.name end)
	end

	for version, data in pairs(coredefs) do
		table.sort(data.enums, function(lhs, rhs) return lhs.name < rhs.name end)
		table.sort(data.funcs, function(lhs, rhs) return lhs.name < rhs.name end)
	end

--[[	
	local coreextsByVersion = spec.GetCoreExts()
	
	local coreexts = {}
	specData.coreexts = coreexts
	for coreVersion, coreExtList in pairs(coreextsByVersion) do
		for i, coreExt in pairs(coreExtList) do
			coreexts[coreExt] = {name = coreExt, version = coreVersion}
		end
	end
]]

	return specData
end

return load

