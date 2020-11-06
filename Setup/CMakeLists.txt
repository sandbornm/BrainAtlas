cmake_minimum_required(VERSION 2.8)

project (Setup)

find_package (ITK REQUIRED)
include (${ITK_USE_FILE})

add_executable (Setup Setup.cxx)

target_link_libraries (Setup ${ITK_LIBRARIES})
