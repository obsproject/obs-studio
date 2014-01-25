
local util = require "util"
local TabbedFile = require "TabbedFile"

local actionTypes = {}
local conditionals = {}

-------------------------------
-- Action base-class
local action = {}

function action:Process(context)
	--Allow start-of-iteration only data to parse.
	if(self.first) then
		--Note that it's *specifically* equal to false. Being 'nil' isn't enough.
		if(context._first == false) then
			return
		end
	end
	
	--Allow end-if-iteration only data to parse.
	if(self.last) then
		--Note that it's *specifically* equal to false. Being 'nil' isn't enough.
		if(context._last == false) then
			return
		end
	end
	
	--Conditional
	if(self._cond) then
		if(not conditionals[self._cond](context)) then
			return
		end
	end
	
	--NO MORE RETURNS FROM THIS POINT FORWARD!
	if(self.newStyle) then
		context:PushStyle(self.newStyle)
	end
	
	local noChildren = nil
	if(self.PreProcess) then
		noChildren = self:PreProcess(context)
	end

	if(not noChildren) then
		self:ProcessChildren(context)
	end

	if(self.PostProcess) then
		self:PostProcess(context)
	end
	
	if(self.newStyle) then
		context:PopStyle()
	end
end

function action:ProcessChildren(context)
	for _, action in ipairs(self) do
		--Preserve the first value.
		local oldFirst = context._first
		local oldLast = context._last
		action:Process(context)
		context._first = oldFirst
		context._last = oldLast
	end
end

local valueResolvers =
{
	enum = function(enum) return enum.name end,
	func = function(func) return func.name end,
	spec = function(spec) return spec.FuncNamePrefix() end,
}

local function ResolveValue(context, value)
	--Find every occurrance of %chars, and try to turn that into a context variable.
	local possibleVars = {}
	for var in value:gmatch("%%([_%a][_%w]*)") do
		possibleVars[var] = true
	end
	
	for var, _ in pairs(possibleVars) do
		if(not context[var]) then
			return nil, "The variable " .. var .. " from the value string was not found.\n" .. value
		end
		
		local replace = context[var]
		if(type(replace) ~= "string") then
			local str = tostring(replace)
			if(str) then
				replace = str
			elseif(valueResolvers[var]) then
				replace = valueResolvers[var](replace)
			elseif(type(replace) == "table" and replace._ValueResolve) then
				replace = replace:_ValueResolve()
			end
		end
		
		if(type(replace) ~= "string") then
			return nil, "Could not convert the variable " .. var .. " into a string."
		end
		
		value = value:gsub("%%" .. var, replace)
	end
	
	return value
end

function action:CallFunction(context, name)
	name = name or self.name
	self:Assert(name, "Unknown function name.")
	local style = context:FindStyleForFunc(name)
	
	if(not style) then
		if(self.optional) then
			return
		else
			self:Assert(nil, "The style does not have a function " .. name)
		end
	end
	
	if(self.value) then
		context.value = self:Assert(ResolveValue(context, self.value))
	end
	
	local paramList = {}
	for _, param in ipairs(self.params) do
		assert(context[param], "The function " .. name ..
			" need a parameter " .. param .. " which doesn't exist at this point")
		paramList[#paramList + 1] = context[param]
	end
	
	local rets = { style[name](unpack(paramList)) }

	if(self.value) then
		context.value = nil
	end

	return unpack(rets)
end

function action:Assert(...)
	local test, text = ...
	if(not test) then
		local msg = ": " .. text
		if(self.name) then
			msg = self._actionType .. "." .. self.name .. msg
		else
			msg = self._actionType .. msg
		end
		assert(test, msg)
	end
	
	return ...
end

--Iterates over the list, setting the second element returned from the iterator
--as the given context table key.
function action:IterateChildren(context, list, key, PostProc)
	PostProc = PostProc or function() end
	
	local oldVal = context[key]
	for _, val in ipairs(list) do
		context[key] = val
		context._first = (_ == 1)
		context._last = (_ == #list)
		self:ProcessChildren(context)
		PostProc(context, val)
	end
	context[key] = oldVal
end

local function CreateAction(data, actionType)
	local act = {}
	util.DeepCopyTable(action, act)
	
	assert(actionType, "No name given for action type")
	
	--Create custom param list.
	if(data.name) then
		local name, params = data.name:match("([_%w]+)%s*%((.*)%)")
		if(name) then
			local paramList = {}
			for param in params:gmatch("([_%a][_%w]*)") do
				paramList[#paramList + 1] = param
			end
			params = paramList
		else
			name = data.name
		end
		
		act.name = name
		act.params = params
	end
	
	if(data.cond) then
		assert(conditionals[data.cond], "Unknown conditional " .. data.cond)
		act._cond = data.cond
	end
	
	act.newStyle = data.style
	act.optional = data.optional
	act.value = data.value
	
	--Make child actions recursively.
	for _, child in ipairs(data) do
		assert(actionTypes[child.type], "Unknown command type " .. child.type)
		act[#act + 1] = actionTypes[child.type](child)
	end
	
	if(data.first) then
		act.first = true
	end
	
	if(data.last) then
		act.last = true
	end
	
	act._actionType = actionType
	
	return act
end

local function MakeActionType(typeName, typeTable, PostInitFunc)
	actionTypes[typeName] = function(data)
		local act = CreateAction(data, typeName)
		util.DeepCopyTable(typeTable, act)
		
		PostInitFunc(act, data)
		
		return act
	end
end


-------------------------------------
-- Group Action
local groupAction = {}

MakeActionType("group", groupAction, function(self, data)
end)


-------------------------------------
-- Call Action
local callAction = {}

function callAction:PreProcess(context)
	self:CallFunction(context, self.name)
end

MakeActionType("call", callAction, function(self, data)
	self.params = self.params or {}
end)


-------------------------------------
-- Context Action
local contextAction = {}

function contextAction:PreProcess(context)
	self:Assert(context[self.key] == nil,
		"Attempt to nest the context variable " .. self.key)

	if(self.data) then
		context[self.key] = self.data
	else
		context[self.key] = self:CallFunction(context, self.name)
	end
end

function contextAction:PostProcess(context)
	if(self.dispose) then
		local style = context:FindStyleForFunc(self.dispose)
		self:Assert(style,
			string.format("Could not find the disposal function %s for %s.",
			self.dispose, self.key))
			
		style[self.dispose](context[self.key])
	end
	context[self.key] = nil
end

MakeActionType("context", contextAction, function(self, data)
	assert(data.key, "Context actions must have a `key`.")
	assert(data.key:match("%_$"), "Context action keys must end in `_`.")
	self.key = data.key
	self.data = data.data
	if(self.name) then
		self.name = "State" .. self.name
	end
	self.dispose = data.dispose
	if(self.dispose) then
		self.dispose = "Dispose" .. self.dispose
	end

	assert(self.data or self.name, "Context actions must have either `data` or `name`.")

	self.params = self.params or {}
end)


-------------------------------------------
-- Filter Action
local filterAction = {}

function filterAction:PreProcess(context)
	local shouldFilter = self:CallFunction(context, self.name)
	if(self.neg) then
		shouldFilter = not shouldFilter
	end
	return not shouldFilter
end

MakeActionType("filter", filterAction, function(self, data)
	assert(data.name, "Filter actions must have a `name`")
	self.name = "Filter" .. self.name
	self.neg = data.neg
	self.params = self.params or {}
end)


----------------------------
-- File Action
local fileAction = {}

function fileAction:PreProcess(context)
	self:Assert(context.hFile == nil, "You cannot nest `file` blocks.")

	local filename = self:CallFunction(context)
	
	context.hFile = util.CreateFile(filename, context.options.indent)
end

function fileAction:PostProcess(context)
	context.hFile:close()
	context.hFile = nil
end

MakeActionType("file", fileAction, function(self, data)
	assert(data.style, "File actions must have a `style`")
	assert(data.name, "File actions need a name to call.")

	self.params = self.params or {"basename", "options"}
end)


-------------------------------------
-- Block Action
local blockAction = {}

function blockAction:PreProcess(context)
	assert(context.hFile, "Cannot write a block outside of a file. " .. self.name)
	self:CallFunction(context, "WriteBlockBegin" .. self.name)
end

function blockAction:PostProcess(context)
	self:CallFunction(context, "WriteBlockEnd" .. self.name)
end

MakeActionType("block", blockAction, function(self, data)
	assert(data.name, "Block actions must have a `name`")

	self.params = self.params or {"hFile", "spec", "options"}
end)


------------------------------------------
-- Write Action
local writeAction = {}

function writeAction:PreProcess(context)
	assert(context.hFile, "Cannot write data outside of a file.")
	self:CallFunction(context)
end

MakeActionType("write", writeAction, function(self, data)
	assert(data.name, "Write actions must have a `name`")
	self.name = "Write" .. self.name
	self.params = self.params or {"hFile", "specData", "spec", "options"}
end)


------------------------------------------
-- Blank Action
local blankAction = {}

function blankAction:PreProcess(context)
	self:Assert(context.hFile, "Blanks must be in files.")
	context.hFile:write("\n")
end

MakeActionType("blank", blankAction, function(self, data)
end)


---------------------------------------------
-- Extension Iterator Action
local extIterAction = {}

function extIterAction:PreProcess(context)
	self:Assert(context.extName == nil, "Cannot nest ext-iter actions.")
	self:IterateChildren(context, context.options.extensions, "extName")
	return true --Stops regular child processing.
end

MakeActionType("ext-iter", extIterAction, function(self, data)
end)

conditionals["ext-iter"] = function(context)
	return #context.options.extensions ~= 0
end


-----------------------------------------------
-- Version Iterator
local versionIterAction = {}

function versionIterAction:PreProcess(context)
	self:Assert(context.version == nil, "Cannot nest version-iter actions.")
	local rawVersionList = context.specData.versions or {}
	local versionList = {}
	for _, version in ipairs(rawVersionList) do
		if(tonumber(version) <= tonumber(context.options.version)) then
			versionList[#versionList + 1] = version
		end
	end

	self:IterateChildren(context, versionList, "version")
	return true --Stops regular child processing.
end

MakeActionType("version-iter", versionIterAction, function(self, data)
end)

conditionals["version-iter"] = function(context)
	return context.specData.versions ~= nil
end


-----------------------------------------------
-- Sub-Version Iterator
local subVersionIterAction = {}

function subVersionIterAction:PreProcess(context)
	self:Assert(context.sub_version == nil, "Cannot nest sub-version-iter actions.")
	self:Assert(context.version, "Must put sub-version-iter inside versions.")
	local rawVersionList = context.specData.versions or {}
	local versionList = {}
	for _, version in ipairs(rawVersionList) do
		if(tonumber(version) <= tonumber(context.version)) then
			versionList[#versionList + 1] = version
		end
	end

	self:IterateChildren(context, versionList, "sub_version")
	return true --Stops regular child processing.
end

MakeActionType("sub-version-iter", subVersionIterAction, function(self, data)
end)

---------------------------------------------
-- Core Extension Iterator Action
local coreExtIterAction = {}

function coreExtIterAction:PreProcess(context)
	self:Assert(context.version, "Must put this in a version iterator")
	self:Assert(context.extName == nil, "Cannot nest core-ext-iter actions.")
	local coreExts = context._coreExts
	if(coreExts[context.version]) then
		self:IterateChildren(context, coreExts[context.version], "extName")
	end
	return true --Stops regular child processing.
end

MakeActionType("core-ext-iter", coreExtIterAction, function(self, data)
end)

conditionals["core-ext-iter"] = function(context)
	assert(context.version, "Cannot have a core-ext-iter conditional outside of a version.")
	return context._coreExts[context.version] ~= nil
end


--[==[
---------------------------------------------
-- Core Extension Iterator Action, culled against the requested extensions.
local coreExtCullIterAction = {}

local function BuildCulledExtList(context)
	local coreExts = context._coreExts
	if(coreExts[context.version]) then
		local extList = {}
		for _, ext in ipairs(coreExts[context.version]) do
			if(not context._extTbl[ext]) then
				extList[#extList + 1] = ext
			end
		end
		return extList
	else
		return {}
	end
end

function coreExtCullIterAction:PreProcess(context)
	self:Assert(context.version, "Must put core-ext-cull-iters in a version")
	self:Assert(context.extName == nil, "Cannot nest core-ext-cull-iter actions.")
	local extList = BuildCulledExtList(context)
	if(#extList > 0) then
		self:IterateChildren(context, extList, "extName")
	end
	return true --Stops regular child processing.
end

MakeActionType("core-ext-cull-iter", coreExtCullIterAction, function(self, data)
end)

conditionals["core-ext-cull-iter"] = function(context)
	assert(context.version, "Cannot have a core-ext-cull-iter conditional outside of a version.")
	return #BuildCulledExtList(context) > 0
end

]==]
----------------------------------------------
-- Enum Seen Action
local enumSeenAction = {}

function enumSeenAction:PreProcess(context)
	self:Assert(context.enumSeen == nil, "Cannot nest enum-seen actions.")
	context.enumSeen = {}
end

function enumSeenAction:PostProcess(context)
	context.enumSeen = nil
end

MakeActionType("enum-seen", enumSeenAction, function(self, data)
end)


-----------------------------------------------
-- Enumerator Iterator
local enumIterAction = {}

local function GetEnumList(context)
	if(context.extName) then
		--Get enum list for the extension.
		return context.specData.extdefs[context.extName].enums, context.extName
	else
		--Get enum list from core version.
		if(context.options.profile ~= "core") then
			return context.specData.coredefs[context.version].enums, context.version
		end
		
		local defList = {}
		local targetVersion = tonumber(context.options.version)
		
		for _, def in ipairs(context.specData.coredefs[context.version].enums) do
			for ix = #def.core, 1, -1 do
				if(tonumber(def.core[ix][1]) <= targetVersion) then
					if(def.core[ix][2] == "core") then
						table.insert(defList, def)
					end
					break;
				end
			end
		end
		
		return defList, context.version
	end
end

function enumIterAction:PreProcess(context)
	self:Assert(context.version or context.extName, "Enumeration iterators must go within a version or extension iterator.")

	local enumList, source = GetEnumList(context)
	
	if(not source) then
		print(context.version, context.extName)
	end

	context.enumTable = context.specData.enumtable
	self:IterateChildren(context, enumList, "enum",
		function(context, enum)
			if(context.enumSeen) then
				context.enumSeen[enum.name] = source
			end
		end)
	context.enumTable = nil
	return true --Stops regular child processing.
end

MakeActionType("enum-iter", enumIterAction, function(self, data)
end)

conditionals["enum-iter"] = function(context)
	assert(context.version or context.extName, "Cannot have an enum-iter conditional outside of a version or extension iterator.")

	return #GetEnumList(context) > 0
end

----------------------------------------------
-- Func Seen Action
local funcSeenAction = {}

function funcSeenAction:PreProcess(context)
	self:Assert(context.funcSeen == nil, "Cannot nest func-seen actions.")
	context.funcSeen = {}
end

function funcSeenAction:PostProcess(context)
	context.funcSeen = nil
end

MakeActionType("func-seen", funcSeenAction, function(self, data)
end)


-----------------------------------------------
-- Function Iterator
local funcIterAction = {}

local function GetFuncList(context)
	if(context.extName) then
		--Get function list for the extension.
		return context.specData.extdefs[context.extName].funcs, context.extName
	else
		--Get function list from core version.
		if(context.options.profile ~= "core") then
			return context.specData.coredefs[context.version].funcs, context.version
		end
		
		local defList = {}
		local targetVersion = tonumber(context.options.version)
		
		for _, def in ipairs(context.specData.coredefs[context.version].funcs) do
			for ix = #def.core, 1, -1 do
				if(tonumber(def.core[ix][1]) <= targetVersion) then
					if(def.core[ix][2] == "core") then
						table.insert(defList, def)
					end
					break;
				end
			end
		end
		
		return defList, context.version

	end
end

function funcIterAction:PreProcess(context)
	self:Assert(context.version or context.extName, "Function iterators must go within a version or extension iterator.")

	local funcList, source = GetFuncList(context)

	self:IterateChildren(context, funcList, "func",
		function(context, func)
			if(context.funcSeen) then
				context.funcSeen[func.name] = source
			end
		end)
	return true --Stops regular child processing.
end

MakeActionType("func-iter", funcIterAction, function(self, data)
end)

conditionals["func-iter"] = function(context)
	assert(context.version or context.extName, "Cannot have a func-iter conditional outside of a version or extension iterator.")

	return #GetFuncList(context) > 0
end

conditionals["core-funcs"] = function(context)
	return context.options.spec == "gl"
end



local struct = {}

function struct.BuildStructure(structure)
	local actions = {}
	for _, data in ipairs(structure) do
		assert(actionTypes[data.type], "Unknown command type " .. data.type)
		actions[#actions + 1] = actionTypes[data.type](data)
	end
	
	actions.Proc = function(basename, style, specData, spec, options)
		local context = {}
		context.basename = basename
		context.style = style
		context.specData = specData
		context.spec = spec
		context.options = options
		
		context._coreExts = spec.GetCoreExts()
		context._extTbl = util.InvertTable(options.extensions)
		context._styles = { style }
		
		function context:GetStyle()
			return context._styles[#context._styles]
		end
		
		function context:FindStyleForFunc(funcName)
			for i = #context._styles, 1, -1 do
				if(context._styles[i][funcName]) then
					return context._styles[i]
				end
			end
			
			return nil
		end
		
		function context:PushStyle(newStyleName)
			--Find the style in the stack, from top to bottom.
			local ix = nil
			for styleIx = #context._styles, 1, -1 do
				if(context._styles[styleIx][newStyleName]) then
					ix = styleIx
					break;
				end
			end
			assert(ix, "Could not find a style named " .. newStyleName)
			
			table.insert(context._styles, context._styles[ix][newStyleName])
			context.style = context._styles[#context._styles]
			
			if(context.style._init) then
				context.style._init()
			end
		end
		
		function context:PopStyle()
			local ret = context._styles[#context._styles]
			context._styles[#context._styles] = nil
			context.style = context._styles[#context._styles]

			if(ret._exit) then
				ret._exit()
			end
			return ret
		end
		
		for _, action in ipairs(actions) do
			action:Process(context)
		end
	end
	
	return actions
end

return struct
