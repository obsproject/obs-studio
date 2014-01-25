
local struct = require "Structure"
local common = require "CommonStruct"


local sys_functions =
{ type="block", name="System(hFile, spec, options)",
	{type="write", name="SetupFunction(hFile, specData, spec, options)", },
	{type="blank", cond="version-iter" },
	{type="write", name="VersionFunctions(hFile, specData, spec, options)", cond="version-iter"},
}

local my_struct =
{
	{ type="file", style="hdr", name="GetFilename(basename, spec, options)",
		{ type="write", name="FilePreamble", optional=true} ,
		{ type="block", name="IncludeGuard",
			{ type="blank"},
			{ type="write", name="Guards(hFile, spec, options)",},
			{ type="blank"},
			{ type="write", name="Typedefs(hFile, specData, spec, options)",},
			{ type="blank"},
			
			{ type="block", name="ExternC(hFile, spec, options)",
				{ type="write", name="LargeHeader(hFile, value, options)", value="Extension Variables", },
				{ type="blank"},
				{ type="block", name="ExtVariables(hFile, spec, options)",
					common.Extensions(),
				},
				{ type="blank"},
				{ type="block", name="Enumerators(hFile, spec, options)",
					common.Enumerators(),
				},
				{ type="blank"},
				common.Functions(),
				sys_functions,
			},
		},
	},
	
	{ type="file", style="src", name="GetFilename(basename, spec, options)",
		{ type="write", name="Includes(hFile, basename, spec, options)", },
		{ type="blank"},
		{ type="write", name="LoaderFunc(hFile, spec, options)", },
		{ type="blank"},
		{ type="block", name="ExtVariables(hFile, spec, options)",
			common.Extensions(),
		},
		{ type="blank"},
		{ type="group", style="typedefs",
			common.Functions(),
		},
		{ type="blank"},
		{ type="group", style="defs",
			common.Functions(),
		},
		{ type="blank"},
		{ type="group", style="switch",
			common.Functions(),
		},
		{ type="blank"},
		sys_functions,
	},
}

my_struct = struct.BuildStructure(my_struct)
return my_struct
