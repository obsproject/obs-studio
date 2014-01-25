
dofile "glsdk/links.lua"

solution "test"
	configurations {"Debug", "Release"}
	defines {"_CRT_SECURE_NO_WARNINGS", "_SCL_SECURE_NO_WARNINGS"}

local tests =
{
	{name = "ptr_cpp"},
	{name = "ptr_c"},
	{name = "ptr_cpp_comp"},
	{name = "ptr_c_comp"},
	{name = "func_cpp"},
	{name = "func_cpp_comp"},
	{name = "noload_cpp"},
	{name = "noload_cpp_noext"},
	{name = "noload_c"},
	{name = "noload_c_old"},
	{name = "noload_c_noext"},
}

local oldDir = os.getcwd()
for _, test in ipairs(tests) do
	os.chdir(path.getabsolute(test.name))
	
	project(test.name .. "_test")
		kind "ConsoleApp"
		language "c++"
		objdir("obj")
		files {"**.cpp"}
		files {"**.c"}
		files {"**.hpp"}
		files {"**.h"}
		
		if(test.include) then
			includedirs(test.include)
		end
		
		UseLibs {"freeglut"}
		
		configuration "windows"
			links {"glu32", "opengl32", "gdi32", "winmm", "user32"}
			
		configuration "linux"
			links {"GL", "GLU", "Xrandr", "X11"}
			
		configuration "Debug"
			targetsuffix "D"
			defines "_DEBUG"
			flags "Symbols"

		configuration "Release"
			defines "NDEBUG"
			flags {"OptimizeSpeed", "NoFramePointer", "ExtraWarnings", "NoEditAndContinue"};
	
	os.chdir(oldDir)
end
