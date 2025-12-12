# This is essentially cmake's BasicConfigVersion-SameMajorVersion.cmake.in but
# without the 32/64-bit check.  Since json is a header-only library, it doesn't
# matter if it was built on a different platform than what it is used on (see
# https://github.com/nlohmann/json/issues/1697).
set(PACKAGE_VERSION "@PROJECT_VERSION@")

if(PACKAGE_VERSION VERSION_LESS PACKAGE_FIND_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE FALSE)
else()

  if(PACKAGE_FIND_VERSION_MAJOR STREQUAL "@PROJECT_VERSION_MAJOR@")
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
  else()
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
  endif()

  if(PACKAGE_FIND_VERSION STREQUAL PACKAGE_VERSION)
      set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
