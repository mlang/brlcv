#.rst:
# FindBrlAPI
# ----------
#
# Find BrlAPI library
#
#=============================================================================

include(FeatureSummary)
set_package_properties(BrlAPI PROPERTIES
  URL         "http://www.brltty.com/"
  DESCRIPTION "Braille displays")

find_path(BrlAPI_INCLUDE_DIR NAMES brlapi.h)
find_library(BrlAPI_LIBRARY NAMES brlapi)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BrlAPI
  REQUIRED_VARS BrlAPI_LIBRARY BrlAPI_INCLUDE_DIR
)
mark_as_advanced(BrlAPI_LIBRARY BrlAPI_INCLUDE_DIR)
if(BrlAPI_FOUND)
  set(BrlAPI_LIBRARIES "${BrlAPI_LIBRARY}")
  set(BrlAPI_INCLUDE_DIRS "${BrlAPI_INCLUDE_DIR}")
  add_library(BrlAPI SHARED IMPORTED)
  if(BrlAPI_INCLUDE_DIR)
    set_target_properties(BrlAPI PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${BrlAPI_INCLUDE_DIRS}"
    )
  endif()
  if(EXISTS "${BrlAPI_LIBRARY}")
    set_target_properties(BrlAPI PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION "${BrlAPI_LIBRARY}"
    )
  endif()
endif()

