MSDEV := msdev
RM := rm -rf

default:
	$(MSDEV) glew.dsw /make \
	"glew_static - Win32 Release" \
	"glew_shared - Win32 Release" \
	"glewinfo - Win32 Release" \
	"visualinfo - Win32 Release" /build

mx:
	$(MSDEV) glew.dsw /make \
	"glew_static - Win32 Release MX" \
	"glew_shared - Win32 Release MX" \
	"glewinfo - Win32 Release MX" \
	"visualinfo - Win32 Release MX" /build

debug:
	$(MSDEV) glew.dsw /make \
	"glew_static - Win32 Debug" \
	"glew_shared - Win32 Debug" \
	"glewinfo - Win32 Debug" \
	"visualinfo - Win32 Debug" /build


clean:
	$(MSDEV) glew.dsw /make all /clean
	$(RM) static shared *.plg *.ncb *.opt
