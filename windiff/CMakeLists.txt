# WinDiff Project
# CMakeLists.txt, generated gencmake.pl, on 2020/06/22 19:37:10
cmake_minimum_required( VERSION 3.6 )

# Disable in-source builds to prevent source tree corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
message(FATAL_ERROR "
   ***FATAL***: Building in source directory '${CMAKE_SOURCE_DIR}'!
   i.e. in-source builds are NOT recommended/allowed.
   Create and use a separate folder, like 'md build'; 'cd build; 'cmake .. [options]'
   Make sure any 'CMakeCache.txt' file, and any 'CMakeFiles' folder are deleted.")
endif()

# CMakeScripts or use the ones that come by default with CMake.
# set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH})

project( windiff )

# The version number.  *** FIX ME ***
set( windiff_MAJOR 3 )
set( windiff_MINOR 0 )
set( windiff_POINT 0 )
# add_definitions( -DWINDIFF_VERSION="3.0.0" )

# Allow developer to select is Dynamic or static library built
set( LIB_TYPE STATIC )  # set default static
option( BUILD_SHARED_LIB "Set ON to build shared Library (DLL)" OFF )


if(CMAKE_COMPILER_IS_GNUCXX)
    set( WARNING_FLAGS -Wall )
endif(CMAKE_COMPILER_IS_GNUCXX)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
   set( WARNING_FLAGS "-Wall -Wno-overloaded-virtual" )
endif() 

if(WIN32 AND MSVC)
    # turn off various warnings - none needed in this compile
    set(WARNING_FLAGS "${WARNING_FLAGS} /wd4996")
    # foreach(warning 4244 4251 4267 4275 4290 4786 4305)
    #     set(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
    # endforeach(warning)
    # other flags: -DNOMINMAX  -D_SCL_SECURE_NO_WARNINGS -D__CRT_NONSTDC_NO_WARNINGS
    set( MSVC_FLAGS "-D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS" )
    #set( NOMINMAX 1 )
    # to distinguish between debug and release libs
    set( CMAKE_DEBUG_POSTFIX "d" )
    option( USE_STATIC_RUNTIME "Set ON to change /MD(DLL) to /MT(static)" ON )
    if (USE_STATIC_RUNTIME)
        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            )
        foreach(CompilerFlag ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        endforeach()    
        message(STATUS "Using /MT STATIC runtime")
    else ()
        message(STATUS "Using /MD DYNAMIC runtime")
    endif ()
else()
    # items for unix
endif()

set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} ${MSVC_FLAGS} -D_REENTRANT" )
set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}" )

# configuration file, if needed
# configure_file( ${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h )
# add_definitions( -DHAVE_CONFIG_H )
# include_directories( ${CMAKE_BINARY_DIR} )

if(BUILD_SHARED_LIB)
   set(LIB_TYPE SHARED)
   message(STATUS "*** Building DLL library ${LIB_TYPE}")
else()
   message(STATUS "*** Option BUILD_SHARED_LIB is OFF ${LIB_TYPE}")
   add_definitions( -DGUTILS_STATIC )
endif()
# Total libs 1, exes 1
#################################################
##### LIBRARIES 1 #####
#################################################

# lib LIBRARY SHARED(DLL) from [src\lib],
# have 12 C/C++ sources, 1 rc, 5 headers
set(name gutils)
set(dir src/lib)
include_directories( ${dir} )
set(${name}_SRCS
    ${dir}/GBIT.C
    ${dir}/GDATE.C
    ${dir}/GMEM.C
    ${dir}/GUTILS.C
    ${dir}/LIST.C
    ${dir}/STATUS.C
    ${dir}/TABLE.C
    ${dir}/TPAINT.C
    ${dir}/TPRINT.C
    ${dir}/TREE.C
    ${dir}/TSCROLL.C
    ${dir}/UTILS.C
    )
set(${name}_HDRS
    ${dir}/GUTILS.h
    ${dir}/GUTILSRC.h
    ${dir}/LIST.h
    ${dir}/TABLE.h
    ${dir}/TREE.h
    )
if(BUILD_SHARED_LIB)
    list(APPEND ${name}_SRCS ${dir}/GUTILS.def ${dir}/GUTILS.RC )
endif()
add_library( ${name} ${LIB_TYPE} ${${name}_SRCS} ${${name}_HDRS} )
list(APPEND add_LIBS ${name})
# deal with install, if any...
#install( TARGETS ${name}
#    RUNTIME DESTINATION bin
#    LIBRARY DESTINATION lib
#    ARCHIVE DESTINATION lib )
#install(FILES ${${name}_HDRS} DESTINATION include)
#################################################
##### EXECUTABLE 1 #####
#################################################

# src EXECUTABLE from [src],
# have 9 C/C++ sources, 1 rc, 11 headers
set(name WinDiff)
set(dir src)
include_directories( ${dir} )
set(${name}_SRCS
    ${dir}/BAR.C
    ${dir}/COMPITEM.C
    ${dir}/COMPLIST.C
    ${dir}/FILE.C
    ${dir}/LINE.C
    ${dir}/SCANDIR.C
    ${dir}/SECTION.C
    ${dir}/VIEW.C
    ${dir}/WINDIFF.C
    
    )
set(${name}_HDRS
    ${dir}/COMPITEM.h
    ${dir}/COMPLIST.h
    ${dir}/FILE.h
    ${dir}/LINE.h
    ${dir}/SCANDIR.h
    ${dir}/SECTION.h
    ${dir}/STATE.h
    ${dir}/TPRIV.h
    ${dir}/VIEW.h
    ${dir}/WDIFFRC.h
    ${dir}/WINDIFF.h
    )
if(BUILD_SHARED_LIB)
    list(APPEND ${name}_SRCS ${dir}/WINDIFF.RC)
else()
    list(APPEND ${name}_SRCS ${dir}/WINDIFF2.RC)
endif()

add_executable( ${name}  WIN32 ${${name}_SRCS} ${${name}_HDRS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
install( TARGETS ${name} DESTINATION bin )

# eof
