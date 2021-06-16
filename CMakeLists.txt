cmake_minimum_required(VERSION 3.0.0)
project(main VERSION 0.0.0)

# Switch to C++ compiler to use #import directive
# SET_SOURCE_FILES_PROPERTIES( main.c PROPERTIES LANGUAGE CXX )
# set (CMAKE_CXX_STANDARD 11)

include_directories( "C:\\Program Files (x86)\\IVI Foundation\\VISA\\WinNT\\Include" )
include_directories( include )
file(GLOB SOURCES "src/*.cpp" "src/*.c")

set ( PROJECT_LINK_LIBS visa32.lib)
link_directories( "C:\\Program Files (x86)\\IVI Foundation\\VISA\\WinNT\\lib\\msc" )

add_executable(instr-anritsu-ms20xxc-win-usb main.c ${SOURCES})
target_link_libraries(instr-anritsu-ms20xxc-win-usb ${PROJECT_LINK_LIBS}) 