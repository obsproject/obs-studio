--[[ Exports a table containing one function: TabbedFile. It wraps a regular Lua file-handle, providing extra functions. Tabs can either be a number of spaces or one tab. It provides the following functions, in addition to (or replacing) the Lua standard ones:

- inc: Increments the tab count by the number given, or by 1 if nothing is given.
- dec: Decrements the tab count by the number given, or by 1 if nothing is given.
- push: Preserves the current tab count.
- pop: Restores the previously preserved tab count.
- write: An indented write; everything is written after the indent.
- writeblock: Takes a single string, breaks it down into multiple lines, and writes each line indented.
- fmt: As string.format followed by an indented write
- fmtblock: As string.format, followed by calling `writeblock`.
- rawfmt: As string.format followed by a NON-indented write.
- rawwrite: hFile:write.


Each call to one of the non-raw writing functions will indent the text.
]]

local members = {}

function members:inc(count)
	count = count or 1
	rawset(self, "_indent", self._indent + count)
end

function members:dec(count)
	count = count or 1
	rawset(self, "_indent", self._indent - count)
end

function members:push()
	local stack = rawget(self, "_indentStack")
	stack[#stack + 1] = rawget(self, "_indent")
end

function members:pop()
	local stack = rawget(self, "_indentStack")
	assert(#stack > 0, "Tab stack underflow.")
	rawset(self, "_indent", stack[#stack])
	stack[#stack] = nil
end

function members:fmt(fmt, ...)
	self:_Indent()
	local str = fmt:format(...)
	rawget(self, "_hFile"):write(str)
end

function members:rawfmt(fmt, ...)
	local str = fmt:format(...)
	rawget(self, "_hFile"):write(str)
end

function members:write(...)
	self:_Indent()
	rawget(self, "_hFile"):write(...)
end

function members:writeblock(block, ...)
	assert(#{...} == 0, "writeblock takes one argument")
	for line in block:gmatch("([^\n]*)\n") do
		self:write(line, "\n")
	end
	
	local last = block:match("\n([^\n]*)$")
	if(last and #last ~= 0) then
		self:write(last)
	end
end

function members:fmtblock(block, ...)
	self:writeblock(block:format(...))
end

function members:rawwrite(...)
	rawget(self, "_hFile"):write(...)
end

-----------------------------------------------------
-- Standard members
local fileMembers =
{
	"close",
	"flush",
	"lines",
	"read",
	"seek",
	"setvbuf",
	"write",
}

for _, fileMem in ipairs(fileMembers) do
	if(not members[fileMem]) then
		members[fileMem] = function(self, ...)
			local hFile = rawget(self, "_hFile")
			return hFile[fileMem](hFile, ...)
		end
	end
end

local metatable = {}

function metatable.__index(self, key)
	return members[key]
end

metatable.__newindex = {}

local function IndentFunc(self)
	self._hFile:write(string.rep(self._indentStr, self._indent))
end

local function TabbedFile(hFile, style, numSpaces)
	numSpaces = numSpaces or 2
	
	local proxy =
	{
		_hFile = hFile,
		_indent = 0,
		_Indent = IndentFunc,
		_indentStack = {}
	}
	
	if(style == "tab") then
		proxy._indentStr = "\t"
	elseif(style == "space") then
		proxy._indentStr = string.rep(" ", numSpaces)
	else
		error("Unknown indent style " .. style)
	end
	
	setmetatable(proxy, metatable)
	
	return proxy
end

return { TabbedFile = TabbedFile }