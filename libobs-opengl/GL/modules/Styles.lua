--[[This module is the interface to all of the style-based code generation facilities.

The module has a function called GetStyleList, which returns a list of all available styles.

This module has a function called GetStyle, which is given a style name. It will return a table of functions that can be evaluated to do different code generation tasks.

SampleStyle.lua contains an example, with documentation for what's going on. Every function you need to define will be there, with comments. Just copy and use as needed.

If you want to extend this to new styles, then create a file in this directory called "UserStyles.lua". In that file, return a table, where the keys in that table are string names for the command-line style name, and the value is the style data itself. The style data is a table containing a Create function, which takes no parameters and returns a style and a structure (two return values).
]]

local style_registry =
{
	pointer_c = require("PointerC_Style"),
	pointer_cpp = require("PointerCPP_Style"),
	func_cpp = require("FuncCpp_Style"),
	noload_cpp = require("NoloadCpp_Style"),
	noload_c = require("NoloadC_Style"),
}

local default_style = "pointer_c"

local status, userStyles = pcall(require, "UserStyles")

if(status and type(userStyles) == "table") then
	for styleName, style in pairs(userStyles) do
		if(style_registry[styleName] ~= nil) then
			print("User-defined style name " .. styleName .. " conflicts with an existing style.")
		else
			style_registry[styleName] = style
		end
	end
end

local function GetStyleList()
	--Make sure the default is first.
	local list = {default_style}
	assert(style_registry[default_style], "Bad default style.")
	
	for style, data in pairs(style_registry) do
		if(style ~= default_style) then
			list[#list + 1] = style
		end
	end
	
	return list
end

local function GetStyle(name)
	assert(style_registry[name], "Unknown style named " .. name)
	
	return style_registry[name].Create()
end

return{
	GetStyleList = GetStyleList,
	GetStyle = GetStyle,
}
