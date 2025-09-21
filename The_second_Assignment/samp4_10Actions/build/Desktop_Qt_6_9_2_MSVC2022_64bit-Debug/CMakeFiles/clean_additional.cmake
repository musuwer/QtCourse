# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\samp4_10_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\samp4_10_autogen.dir\\ParseCache.txt"
  "samp4_10_autogen"
  )
endif()
