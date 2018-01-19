prefix=@CMAKE_INSTALL_PREFIX@
includedir=${prefix}/include
libdir=${prefix}/@CMAKE_INSTALL_LIBDIR@

Name: libebur128
Description: EBU R 128 standard for loudness normalisation
Version: @EBUR128_VERSION@
URL: https://github.com/jiixyj/libebur128
Libs: -L${libdir} -lebur128
Libs.private: -lm
Cflags: -I${includedir}
