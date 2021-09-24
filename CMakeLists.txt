cmake_minimum_required(VERSION 3.0.0)
project(instr-ms20xxc-win-visa VERSION 0.0.0)

find_package(Git)
if(Git_FOUND)
  message("Git found: ${GIT_EXECUTABLE}")
endif() 

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

################################################################################
# submodules
################################################################################
execute_process(COMMAND "${GIT_EXECUTABLE}" "config" "http.postBuffer" "524288000" WORKING_DIRECTORY ${instr-ms20xxc-win-visa_SOURCE_DIR} RESULT_VARIABLE res)
message("Executing '${GIT_EXECUTABLE} http.postBuffer 524288000' = '${res}'")
execute_process(COMMAND "${GIT_EXECUTABLE}" "submodule" "init" WORKING_DIRECTORY ${instr-ms20xxc-win-visa_SOURCE_DIR} RESULT_VARIABLE res)
message("Executing '${GIT_EXECUTABLE} submodule init' = '${res}'")
execute_process(COMMAND "${GIT_EXECUTABLE}" "submodule" "update" WORKING_DIRECTORY ${instr-ms20xxc-win-visa_SOURCE_DIR} RESULT_VARIABLE res)
message("Executing '${GIT_EXECUTABLE} submodule update' = '${res}'")

# vcpkg
if (WIN32)
  unset(VCPKG_EXE CACHE)
  find_path(VCPKG_EXE NAMES "vcpkg.exe" PATHS ${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg)
  if(VCPKG_EXE STREQUAL "VCPKG_EXE-NOTFOUND")
  execute_process(COMMAND "${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat" RESULT_VARIABLE res) 
  message("Executing '${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/bootstrap-vcpkg.bat' = '${res}'")
  find_path(VCPKG_EXE NAMES "vcpkg.exe" PATHS ${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg)
  message("VCPKG_EXE = ${VCPKG_EXE}")
  else()
    message("VCPKG_EXE = ${VCPKG_EXE}")
  endif()
  
  execute_process(COMMAND "${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/vcpkg.exe" "integrate" "install" RESULT_VARIABLE res OUTPUT_QUIET)
  message("Executing '${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/vcpkg.exe integrate install' = '${res}'")
  
  unset(JANSSON_LIB CACHE)
  find_path(JANSSON_LIB NAMES "jansson.lib" PATHS ${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/installed/x86-windows/lib/)
  if(JANSSON_LIB STREQUAL "JANSSON_LIB-NOTFOUND")
    execute_process(COMMAND "${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/vcpkg.exe" "install" "jansson" RESULT_VARIABLE res)
    message("Executing '${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/vcpkg.exe install jansson' = '${res}'")
    find_path(JANSSON_LIB NAMES "jansson.lib" PATHS ${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/installed/x86-windows/lib)
    message("JANSSON_LIB = ${JANSSON_LIB}")
  else()
    message("JANSSON_LIB = ${JANSSON_LIB}")
  endif()

  include_directories( ${instr-ms20xxc-win-visa_SOURCE_DIR}/vcpkg/installed/x86-windows/include )
endif (WIN32)


include_directories( "C:\\Program Files (x86)\\IVI Foundation\\VISA\\WinNT\\Include" )
include_directories( include )
file(GLOB SOURCES "src/*.cpp" "src/*.c")

set ( PROJECT_LINK_LIBS visa32.lib)
link_directories( "C:\\Program Files (x86)\\IVI Foundation\\VISA\\WinNT\\lib\\msc" )

add_executable(instr-anritsu-ms20xxc-win-usb main.c ${SOURCES})
target_link_libraries(instr-anritsu-ms20xxc-win-usb ${PROJECT_LINK_LIBS}) 