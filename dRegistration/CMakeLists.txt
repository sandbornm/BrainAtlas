cmake_minimum_required(VERSION 2.8)

project (dRegistration)

find_package (ITK REQUIRED)
include (${ITK_USE_FILE})

add_executable (dRegistration dRegistration.cxx)

target_link_libraries (dRegistration ${ITK_LIBRARIES})
