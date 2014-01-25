--[[ The function CmdLineOptions takes the command-line options and processes them according to a series of functions it is given. It can handle any options of the standard forms, such as:

- "-optName param"
- "--optName option1 option2 option3"
- "-optName=param"

It takes the following parameters:
- An array of command-line options as strings.
- A table of functions, where the key name matches the options. Note that the match will be case-sensitive.
- A value to be passed to the functions. This allows them to be a bit more independent without having to use upvalue tricks.

The return value is a list of any positional arguments, in order.

The option processor functions take the following parameters:
- The value to be passed to the processor. A candidate for a `self` parameter.
- The first parameter string of the option, if any.
- A nullary iterator function to iterate over all of the options associated with the command. It can have 0 iterations. It is OK to iterate multiple times. The iterator returns two values: the parameter and the parameter's 1-base index.

The return value from the processing function is the number of options processed. If `nil` is returned, then it is assumed that *all* available options were processed.

The processor functions are called within a `pcall`, so any errors will be assumed to be processing errors related to that option. Appropriate error messages will be emitted mentioning the option name, so it doesn't need to keep its own name. It is up to each processor to decide if it has enough or too many parameters and error out if it does. Processing of command line options will error if there is a failure.

The processor assumes that strings that begin with a `-` character is an option. If a parameter is specified with the `-option=param` syntax, then it is assumed to have exactly one parameter. Thus the next value is assumed to be an option. For all other option formats, the number of processed arguments is decided upon by the processing function. If it returns `nil`, then it assumes all arguments were processed.

Any "options" that do not conform to option syntax are assumed to be positional arguments. They are stored in an array and returned by the function.
]]

local util = require "util"

--Returns nil if not an option. Otherwise returns the option and a possible
--parameter name if it is of the form "--option=foo".
local function GetOptionName(option)
	local option, param = string.match(option, "^%-%-?([^%-%=][^%=]*)%=?(.*)")
	if(param and #param == 0) then
		param = nil
	end
	
	return option, param
end

--Returns a nullary function that iterates over a single parameter. Namely, this one.
local function GetParamIterator(param)
	return function()
		return function(s, var)
			if(var) then
				return nil, nil
			else
				return param, 1
			end
		end, nil, nil
	end
end

--Returns a nullary function that iterates over all parameters from the given
--index to the next option.
local function GetParamListIterator(params, startIx)
	return function()
		local state = {startIx}
		return function(state, var)
			--Stop if out of parameters
			if(state[1] > #params) then
				return nil
			end
			
			--Stop if the parameter is an option name.
			if(GetOptionName(params[state[1]])) then
				return nil
			end
			
			state[1] = state[1] + 1
			return params[state[1] - 1], state[1] - startIx
		end, state, nil
	end
end

local function CountNumOptions(iter)
	local numOpts = 0
	for _ in iter() do
		numOpts = numOpts + 1
	end
	return numOpts
end

local function CallProcessor(func, option, value, param, iter)
	local status, nargs = pcall(func, value, param, iter)
	if(not status) then
		error("The option '" .. option .. "' had an error:\n" .. nargs)
	end
	
	return nargs or CountNumOptions(iter)
end

local modTbl = {}

function modTbl.CmdLineOptions(cmd_line, processors, value)
	local posArgs = {}
	local optIx = 1
	local numOpts = #cmd_line
	while(optIx <= numOpts) do
		local option, param = GetOptionName(cmd_line[optIx])
		
		if(not option) then
			posArgs[#posArgs + 1] = cmd_line[optIx]
			optIx = optIx + 1
		else
			assert(processors[option], "The option '" .. option .. "' is not a valid option for this program.")
			
			if(param) then
				CallProcessor(processors[option], option, value,
					param, GetParamIterator(param))
			else
				local paramIter = GetParamListIterator(cmd_line, optIx + 1)
				local numOpts = CountNumOptions(paramIter)
				if(numOpts > 0) then
					param = cmd_line[optIx + 1]
				end
				local nargs = CallProcessor(processors[option], option, value,
					param, paramIter)
				
				optIx = optIx + nargs
				
			end
			optIx = optIx + 1
		end
	end
	
	return posArgs
end


--------------------------------------------------
-- Option group logic.
local group = {}

local function ExtractDescArray(desc)
	if(type(desc) == "table") then
		local descArray = {}
		for i, val in ipairs(desc) do
			descArray[#descArray + 1] = val
		end
		return descArray
	else
		return { desc }
	end
end

function group:value(optName, tblName, desc, default, optional)
	table.insert(self._doc_order, optName)
	self._procs[optName] = {
		desc = desc,
		tableName = tblName,
		default = default,
		optional = optional,
		 --self is the destination table, where the data goes
		proc = function(self, param, iter)
			assert(param, "This option needs a single parameter")
			assert(not self[tblName], "Cannot specify the option twice")
			self[tblName] = param
			return 1
		end,
		
		document = function(self)
			local docs = ExtractDescArray(self.desc)
			if(self.default) then
				docs[#docs + 1] = "Default value: " .. self.default
			else
				if(self.optional) then
					docs[#docs + 1] = "This option is not required."
				end
			end
			
			return docs
		end,
	}
end

function group:enum(optName, tblName, desc, values, defaultIx, optional)
	table.insert(self._doc_order, optName)
	local valuesInv = util.InvertTable(values)
	self._procs[optName] = {
		desc = desc,
		tableName = tblName,
		values = values,
		valuesInv = valuesInv,
		optional = optional,
		proc = function(self, param, iter)
			assert(param, "This option needs a parameter")
			assert(valuesInv[param], param .. " is not a valid value.")
			assert(not self[tblName], "Cannot specify this option twice.");
			self[tblName] = param
			return 1
		end,
		
		document = function(self)
			local docs = ExtractDescArray(self.desc)
			docs[#docs + 1] = "Allowed values:"
			docs[#docs + 1] = table.concat(self.values, ", ")
			if(self.default) then
				docs[#docs + 1] = "Default value: " .. self.default
			else
				if(self.optional) then
					docs[#docs + 1] = "This option is not required."
				end
			end
			
			return docs
		end,
	}
	
	if(defaultIx) then
		self._procs[optName].default = values[defaultIx]
	end
end

function group:array(optName, tblName, desc, modifier, optional)
	table.insert(self._doc_order, optName)
	self._procs[optName] = {
		desc = desc,
		tableName = tblName,
		optional = optional,
		proc = function(self, param, iter)
			self[tblName] = self[tblName] or {}
			
			local bFound = false
			for ext in iter() do
				if(modifier) then
					ext = modifier(ext)
				end
				table.insert(self[tblName], ext)
				bFound = true
			end
			
			assert(bFound, "Must provide at least one value.");
		end,
		
		document = function(self)
			local docs = ExtractDescArray(self.desc)
			return docs
		end,
	}
end

--Stores its data in an array, but it only takes one parameter.
function group:array_single(optName, tblName, desc, modifier, optional)
	table.insert(self._doc_order, optName)
	self._procs[optName] = {
		desc = desc,
		tableName = tblName,
		optional = optional,
		proc = function(self, param, iter)
			assert(param, "This option needs a single parameter")
			self[tblName] = self[tblName] or {}

			if(modifier) then
				param = modifier(param)
			end
			table.insert(self[tblName], param)
			return 1
		end,
		
		document = function(self)
			local docs = ExtractDescArray(self.desc)
			docs[#docs + 1] = "Can be used multiple times."
			return docs
		end,
	}
end

function group:pos_opt(index, tblName, desc, optName, default, optional)
	assert(not self._pos_opts[index],
		"Positional argument " .. index .. " is already in use")

	self._pos_opts[index] = {
		desc = desc,
		tableName = tblName,
		optName = optName,
		default = default,
		optional = optional,
	}
end

function group:AssertParse(test, msg)
	if(not test) then
		io.stdout:write(msg, "\n")
		self:DisplayHelp()
		error("", 0)
	end
end

function group:ProcessCmdLine(cmd_line)
	local procs = {}
	
	for option, data in pairs(self._procs) do
		procs[option] = data.proc
	end
	
	local options = {}
	
	local status, posOpts = 
		pcall(modTbl.CmdLineOptions, cmd_line, procs, options)

	self:AssertParse(status, posOpts)
	
	--Apply positional arguments.
	for ix, pos_arg in pairs(self._pos_opts) do
		if(posOpts[ix]) then
			options[pos_arg.tableName] = posOpts[ix]
		elseif(pos_arg.default) then
			options[pos_arg.tableName] = default
		else
			self:AssertParse(pos_arg.optional,
				"Missing positional argument " .. pos_arg.optName)
		end
	end

	--Apply defaults.
	for option, data in pairs(self._procs) do
		if(not options[data.tableName]) then
			if(data.default) then
				options[data.tableName] = data.default
			else
				self:AssertParse(data.optional,
					"Option " .. option .. " was not specified.")
			end
		end
	end
	
	return options, posOpts
end

function group:DisplayHelp()
	local hFile = io.stdout
	
	local function MaxVal(tbl)
		local maxval = 0
		for ix, val in pairs(tbl) do
			if(ix > maxval) then
				maxval = ix
			end
		end
		
		return maxval
	end

	--Write the command-line, including positional arguments.
	hFile:write("Command Line:")
	local maxPosArg = MaxVal(self._pos_opts)
	
	for i = 1, maxPosArg do
		if(self._pos_opts[i]) then
			hFile:write(" <", self._pos_opts[i].optName, ">")
		else
			hFile:write(" <something>")
		end
	end
	
	hFile:write(" <options>\n")
	
	--Write each option.
	hFile:write("Options:\n")
	for _, option in ipairs(self._doc_order) do
		local data = self._procs[option]
		hFile:write("-", option, ":\n")
		
		local docs = data:document()
		
		for _, str in ipairs(docs) do
			hFile:write("\t", str, "\n")
		end
	end
end

function modTbl.CreateOptionGroup()
	local optGroup = {}
	
	for key, func in pairs(group) do
		optGroup[key] = func
	end
	
	optGroup._procs = {}
	optGroup._pos_opts = {}
	optGroup._doc_order = {}
	
	return optGroup
end

return modTbl
