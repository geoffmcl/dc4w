# DC4W Project - from dc4w-52.zip 08/08/2013
# CMakeLists.txt, generated gencmake.pl, on 2020/06/17 18:55:28
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

project( dc4w )

# The version number. is SET in dc4wVers.h

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
    # foreach(warning  4251 4275 4290 4786 4305)
    foreach(warning 4244 4267 )
        set(WARNING_FLAGS "${WARNING_FLAGS} /wd${warning}")
    endforeach(warning)
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
endif()
# Total libs 0, exes 3
#################################################
##### LIBRARIES 0 #####
#################################################
#list(APPEND add_LIBS ${name})
# deal with install, if any...
#install( TARGETS ${name}
#    RUNTIME DESTINATION bin
#    LIBRARY DESTINATION lib
#    ARCHIVE DESTINATION lib )
#install(FILES ${${name}_HDRS} DESTINATION include)
# izuz EXECUTABLE from [src\izuz],
# have 18 C/C++ sources, 14 headers
###if(ADD_ZLIB_INTERNAL)
set(name izuz)
set(dir src/izuz)
set(${name}_SRCS
    ${dir}/iz_api.c
    ${dir}/iz_crc32.c
    ${dir}/iz_crctab.c
    ${dir}/iz_crpt.c
    ${dir}/iz_envargs.c
    ${dir}/iz_explode.c
    ${dir}/iz_extract.c
    ${dir}/iz_fileio.c
    ${dir}/iz_globals.c
    ${dir}/iz_inflate.c
    ${dir}/iz_list.c
    ${dir}/iz_match.c
    ${dir}/iz_nt.c
    ${dir}/iz_process.c
    ${dir}/iz_ttyio.c
    ${dir}/iz_unzip.c
    ${dir}/iz_win32.c
    ${dir}/iz_zipinfo.c
    )
set(${name}_HDRS
    ${dir}/iz_consts.h
    ${dir}/iz_crypt.h
    ${dir}/iz_ebcdic.h
    ${dir}/iz_globals.h
    ${dir}/iz_inflate.h
    ${dir}/iz_nt.h
    ${dir}/iz_timezone.h
    ${dir}/iz_ttyio.h
    ${dir}/iz_unzip.h
    ${dir}/iz_unzpriv.h
    ${dir}/iz_userfun.h
    ${dir}/iz_version.h
    ${dir}/iz_w32cfg.h
    ${dir}/iz_zip.h
    )
add_library( ${name} ${${name}_SRCS} ${${name}_HDRS} )
target_compile_definitions( ${name} PUBLIC IZDLL PUBLIC GMUZ PUBLIC NO_ZIPINFO )
list(APPEND add_LIBS ${name})
# deal with install, if any...
#install( TARGETS ${name} DESTINATION lib )
###endif(ADD_ZLIB_INTERNAL)

#################################################
##### EXECUTABLE 2 #####
#################################################

if(ADD_TAB_CTRL)
# tabctrl EXECUTABLE from [src\tabctrl],
# have 9 C/C++ sources, 1 rc, 12 headers
set(name tabctrl)
set(dir src/tabctrl)
set(${name}_SRCS
    ${dir}/CompPg.cpp
    ${dir}/ExpPag.cpp
    ${dir}/OutPg.cpp
    ${dir}/StdAfx.cpp
    ${dir}/TCIni.cpp
    ${dir}/TCLog.cpp
    ${dir}/TabCtrl.cpp
    ${dir}/TabCtrlDlg.cpp
    ${dir}/TabSheet.cpp
    ${dir}/TabCtrl.rc
    )
set(${name}_HDRS
    ${dir}/CompPg.h
    ${dir}/ExpPag.h
    ${dir}/OutPg.h
    ${dir}/StdAfx.h
    ${dir}/TCIni.h
    ${dir}/TCLog.h
    ${dir}/TCVers.h
    ${dir}/TCWork.h
    ${dir}/TabCtrl.h
    ${dir}/TabCtrlDlg.h
    ${dir}/TabSheet.h
    ${dir}/resource.h
    )
add_executable( ${name}  WIN32 ${${name}_SRCS} ${${name}_HDRS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
#install( TARGETS ${name} DESTINATION bin )
endif(ADD_TAB_CTRL)

# src EXECUTABLE from [src],
# have 46 C/C++ sources, 2 rc, 42 headers
# ${dir}/dc4w-ok.rc ${dir}/browse2.c ${dir}/Browse.cpp
set(name dc4w)
set(dir src)
set(${name}_SRCS
    ${dir}/FilDlg.cpp
    ${dir}/bar.c
    ${dir}/compitem.c
    ${dir}/complist.c
    ${dir}/dc4w.c
    ${dir}/dc4wAbt.c
    ${dir}/dc4wBrowse.c
    ${dir}/dc4wContext.c
    ${dir}/dc4wDel1.c
    ${dir}/dc4wDlg.c
    ${dir}/dc4wDlgR.c
    ${dir}/dc4wDlgcf.c
    ${dir}/dc4wHelp.c
    ${dir}/dc4wIni.c
    ${dir}/dc4wList.c
    ${dir}/dc4wLog.c
    ${dir}/dc4wMenu.c
    ${dir}/dc4wOpts.c
    ${dir}/dc4wSB.c
    ${dir}/dc4wSize.c
    ${dir}/dc4wTT.c
    ${dir}/dc4wTime.c
    ${dir}/dc4wUtil.c
    ${dir}/dc4wZip.c
    ${dir}/file.c
    ${dir}/font.cpp
    ${dir}/gbit.c
    ${dir}/gdate.c
    ${dir}/gmem.c
    ${dir}/gutils.c
    ${dir}/line.c
    ${dir}/list.c
    ${dir}/picture.cpp
    ${dir}/scandir.c
    ${dir}/section.c
    ${dir}/sstabctl.cpp
    ${dir}/status.c
    ${dir}/table.c
    ${dir}/tpaint.c
    ${dir}/tprint.c
    ${dir}/tree.c
    ${dir}/tscroll.c
    ${dir}/view.c
    ${dir}/wmdiag.c
    ${dir}/dc4w.rc
    )
set(${name}_HDRS
    ${dir}/Browse.h
    ${dir}/FilDlg.h
    ${dir}/browse2.h
    ${dir}/compitem.h
    ${dir}/complist.h
    ${dir}/dc4w.h
    ${dir}/dc4wDel1.h
    ${dir}/dc4wDlg.h
    ${dir}/dc4wHelp.h
    ${dir}/dc4wIni.h
    ${dir}/dc4wLV.h
    ${dir}/dc4wList.h
    ${dir}/dc4wLog.h
    ${dir}/dc4wSB.h
    ${dir}/dc4wStgs.h
    ${dir}/dc4wTT.h
    ${dir}/dc4wTime.h
    ${dir}/dc4wUser.h
    ${dir}/dc4wUtil.h
    ${dir}/dc4wVers.h
    ${dir}/dc4wWork.h
    ${dir}/dc4wWorkF.h
    ${dir}/dc4wZip.h
    ${dir}/dc4wcomp.h
    ${dir}/file.h
    ${dir}/font.h
    ${dir}/gutils.h
    ${dir}/line.h
    ${dir}/list.h
    ${dir}/picture.h
    ${dir}/resource-ok.h
    ${dir}/resource.h
    ${dir}/scandir.h
    ${dir}/section.h
    ${dir}/sstabctl.h
    ${dir}/state.h
    ${dir}/stdafx.h
    ${dir}/table.h
    ${dir}/tpriv.h
    ${dir}/tree.h
    ${dir}/view.h
    ${dir}/wmdiag.h
    )
add_executable( ${name}  WIN32 ${${name}_SRCS} ${${name}_HDRS} )
if (add_LIBS)
    target_link_libraries( ${name} ${add_LIBS} )
endif ()
if (MSVC)
    set_target_properties( ${name} PROPERTIES DEBUG_POSTFIX d )
endif ()
# deal with install, if any...
#install( TARGETS ${name} DESTINATION bin )

# eof
