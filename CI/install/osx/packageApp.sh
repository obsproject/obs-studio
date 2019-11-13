rm -rf ./OBS.app

mkdir OBS.app
mkdir OBS.app/Contents
mkdir OBS.app/Contents/MacOS
mkdir OBS.app/Contents/Plugins
mkdir OBS.app/Contents/Resources

cp -r rundir/RelWithDebInfo/bin/ ./OBS.app/Contents/MacOS
cp -r rundir/RelWithDebInfo/data ./OBS.app/Contents/Resources
cp ../CI/install/osx/obs.icns ./OBS.app/Contents/Resources
cp -r rundir/RelWithDebInfo/obs-plugins/ ./OBS.app/Contents/Plugins
cp ../CI/install/osx/Info.plist ./OBS.app/Contents

../CI/install/osx/dylibBundler -b -cd -d ./OBS.app/Contents/Frameworks -p @executable_path/../Frameworks/ \
-s ./OBS.app/Contents/MacOS \
-s /usr/local/opt/mbedtls/lib/ \
-x ./OBS.app/Contents/Plugins/coreaudio-encoder.so \
-x ./OBS.app/Contents/Plugins/decklink-ouput-ui.so \
-x ./OBS.app/Contents/Plugins/frontend-tools.so \
-x ./OBS.app/Contents/Plugins/image-source.so \
-x ./OBS.app/Contents/Plugins/linux-jack.so \
-x ./OBS.app/Contents/Plugins/mac-avcapture.so \
-x ./OBS.app/Contents/Plugins/mac-capture.so \
-x ./OBS.app/Contents/Plugins/mac-decklink.so \
-x ./OBS.app/Contents/Plugins/mac-syphon.so \
-x ./OBS.app/Contents/Plugins/mac-vth264.so \
-x ./OBS.app/Contents/Plugins/obs-browser.so \
-x ./OBS.app/Contents/Plugins/obs-browser-page \
-x ./OBS.app/Contents/Plugins/obs-ffmpeg.so \
-x ./OBS.app/Contents/Plugins/obs-filters.so \
-x ./OBS.app/Contents/Plugins/obs-transitions.so \
-x ./OBS.app/Contents/Plugins/obs-vst.so \
-x ./OBS.app/Contents/Plugins/rtmp-services.so \
-x ./OBS.app/Contents/MacOS/obs \
-x ./OBS.app/Contents/MacOS/obs-ffmpeg-mux \
-x ./OBS.app/Contents/MacOS/obslua.so \
-x ./OBS.app/Contents/Plugins/obs-x264.so \
-x ./OBS.app/Contents/Plugins/text-freetype2.so \
-x ./OBS.app/Contents/Plugins/obs-libfdk.so
# -x ./OBS.app/Contents/Plugins/obs-outputs.so \

/usr/local/Cellar/qt/5.10.1/bin/macdeployqt ./OBS.app

mv ./OBS.app/Contents/MacOS/libobs-opengl.so ./OBS.app/Contents/Frameworks

# put qt network in here becasuse streamdeck uses it
cp -r /usr/local/opt/qt/lib/QtNetwork.framework ./OBS.app/Contents/Frameworks
chmod +w ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork
install_name_tool -change /usr/local/Cellar/qt/5.10.1/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./OBS.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork

# decklink ui qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./OBS.app/Contents/Plugins/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./OBS.app/Contents/Plugins/decklink-ouput-ui.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./OBS.app/Contents/Plugins/decklink-ouput-ui.so

# frontend tools qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./OBS.app/Contents/Plugins/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./OBS.app/Contents/Plugins/frontend-tools.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./OBS.app/Contents/Plugins/frontend-tools.so

# vst qt
install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./OBS.app/Contents/Plugins/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./OBS.app/Contents/Plugins/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./OBS.app/Contents/Plugins/obs-vst.so
install_name_tool -change /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./OBS.app/Contents/Plugins/obs-vst.so
