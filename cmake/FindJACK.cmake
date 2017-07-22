#.rst:
# FindJACK
# -------
#
# Find jack library
#
#=============================================================================

include(FeatureSummary)
set_package_properties(JACK PROPERTIES
  URL         "http://www.jackaudio.org/"
  DESCRIPTION "JACK Audio Connection Kit")

find_package(PkgConfig QUIET REQUIRED)
pkg_check_modules(PC_JACK QUIET jack)
find_path(JACK_INCLUDE_DIR NAMES jack/jack.h HINTS ${PC_JACK_INCLUDE_DIRS})
find_library(JACK_LIBRARY NAMES jack HINTS ${PC_JACK_LIBRARY_DIRS})

set(JACK_DEFINITIONS ${PC_JACK_CFLAGS_OTHER})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JACK
  REQUIRED_VARS JACK_LIBRARY JACK_INCLUDE_DIR
  VERSION_VAR PC_JACK_VERSION
)
mark_as_advanced(JACK_LIBRARY JACK_INCLUDE_DIR)
if(JACK_FOUND)
  set(JACK_LIBRARIES "${JACK_LIBRARY}")
  set(JACK_INCLUDE_DIRS "${JACK_INCLUDE_DIR}")
  add_library(JACK SHARED IMPORTED)
  if(JACK_INCLUDE_DIR)
    set_target_properties(JACK PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${JACK_INCLUDE_DIRS}"
    )
  endif()
  if(EXISTS "${JACK_LIBRARY}")
    set_target_properties(JACK PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${JACK_LIBRARY}"
    )
  endif()
  if(JACK_DEFINITIONS)
    set_target_properties(JACK PROPERTIES
      INTERFACE_COMPILE_OPTIONS "${JACK_DEFINITIONS}"
    )
  endif()
endif()

