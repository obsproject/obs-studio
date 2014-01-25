glLoadGenerator, version 1.0


Please see the documentation available on the web at https://bitbucket.org/alfonse/glloadgen/wiki/Home for detailed information on how to use this software.

The license for this distribution is available in the `License.txt` file.


Usage
-----

This loader generation system is used to create OpenGL headers and loading code for your specific needs. Rather than getting every extension and core enumerator/function all in one massive header, you get only what you actually want and ask for.

The scripts in this package are licensed under the terms of the MIT License. You will need to have Lua installed for this to work.

To use the code generator, with Lua in your path (assuming that "lua" is the name of your Lua executable), type this:

 lua LoadGen.lua -style=pointer_c -spec=gl -version=3.3 -profile=core core_3_3

This tells the system to generate a header/source pair for OpenGL ("-spec=gl", as opposed to WGL or GLX), for version 3.3, the core profile. It will generate it in the "pointer_c" style, which means that it will use function pointer-style, with C linkage and source files. Such code is usable from C and C++, or other languages that can interface with C.

The option "core_3_3" is the basic component of the filename that will be used for the generation. Since it is generating OpenGL loaders (again, as opposed to WGL or GLX), it will generate files named "gl_core_3_3.*", where * is the extension used by the particular style.

The above command line will generate "gl_core_3_3.h" and "gl_core_3_3.c" files. Simply include them in your project; there is no library to build, no unresolved extenals to filter through. They just work.

Changes
-------

Version 1.0:
 * New Noload loader. Works like GLee.
 * -stdext now works relative to the extfiles directory, not just LoadGen. So no need to do -stdext=extfiles/gl_name_of_standard_file.txt.
 * A test suite.
 * Lua Filesystem is now in use; if it's not available, then you must create the destination directory yourself.

Version 0.3:
 * Replaced the old generation system with a flexible structure system.
 * Migrated the styles to the structure system.