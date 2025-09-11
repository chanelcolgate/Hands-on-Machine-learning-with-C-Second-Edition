include(FindPackageHandleStandardArgs)

message(INFO "Looking mlpack in ${CMAKE_PREFIX_PATH}")
find_path(MLPACK_INCLUDE_DIR
	NAMES mlpack/core.hpp mlpack/prereqs.hpp
	PATH "${CMAKE_PREFIX_PATH}/include/"
)

find_package_handle_standard_args(mlpack
	REQUIRED_VARS MLPACK_INCLUDE_DIR
)

if(mlpack_FOUND)
	set(MLPACK_INCLUDE_DIRS ${MLPACK_INCLUDE_DIR})
endif()

# Hide internal variables
mark_as_advanced(MLPACK_INCLUDE_DIR)
