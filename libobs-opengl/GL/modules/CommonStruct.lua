
local common = {}

--Iterates over all requested extensions
--Calls Extension(hFile, extName, spec, options)
local extensions = 
{ type="group",
	{ type="ext-iter",
		{ type="write", name="Extension(hFile, extName, spec, options)", },
	},
}

function common.Extensions() return extensions end

--Iterates over every enumerator, in order:
-- Requested extension enums.
-- For each version:
--  Core extension enumerators from version X
--  Core enumerators from version X
-- Calls Enumerator(hFile, enum, enumTable, spec, options, enumSeen)
-- Optional small headers
local enumerators =
{ type="group",
{ type="enum-seen",
	{ type="ext-iter",
		{type="enum-iter",
			{ type="write", name="SmallHeader(hFile, value, options)", value="Extension: %extName", first=true, optional=true},
			{ type="write", name="Enumerator(hFile, enum, enumTable, spec, options, enumSeen)", },
			{ type="blank", last=true },
		},
	},
	{ type="version-iter",
		{type="enum-iter",
			{ type="write", name="SmallHeader(hFile, value, options)", value="Version: %version", first=true, optional=true},
			{ type="write", name="Enumerator(hFile, enum, enumTable, spec, options, enumSeen)", },
			{ type="blank", last=true },
		},
	},
},
}

function common.Enumerators() return enumerators end

--Iterates over each function, in order:
-- Requested extension functions.
-- For each version:
--  Core extensions from for version X
--  Core functions from version X
-- Calls Function(hFile, func, spec, options, funcSeen)
-- Optional small headers.
-- Can provide an optional ending table, that will be placed within
-- the "func-seen" block.
function common.Functions(ending)
	ending = ending or { type="group" }
	return
	{ type="func-seen",
		{ type="ext-iter",
			{type="func-iter",
				{ type="write", name="SmallHeader(hFile, value, options)", value="Extension: %extName", first=true, optional=true},
				{ type="write", name="Function(hFile, func, spec, options, funcSeen)", },
				{ type="blank", last=true },
			},
		},
		{ type="version-iter",
			{type="func-iter",
				{ type="write", name="SmallHeader(hFile, value, options)", value="Extension: %version", first=true, optional=true},
				{ type="write", name="Function(hFile, func, spec, options, funcSeen)", },
				{ type="blank", last=true },
			},
		},
		ending,
	}
end

return common
