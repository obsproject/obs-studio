See doc/index.html for more information.

If you downloaded the tarball from the GLEW website, you just need to:

    Unix:

        make
        
        sudo -s
        
        make install
        
        make clean

    Windows:

        use the project file in build/vc6/

If you wish to build GLEW from scratch (update the extension data from
the net or add your own extension information), you need a Unix
environment (including wget, perl, and GNU make).  The extension data
is regenerated from the top level source directory with:

        make extensions
