######################################################################
#
#	CMake file for LDForge.
#	The original QMake file was created on Sat Sep 22 17:29:49 2012
#
######################################################################

project (ldforge)
cmake_minimum_required (VERSION 2.6)

option (TRANSPARENT_DIRECT_COLORS "Enables non-standard transparent direct colors" OFF)
option (USE_QT5 "Use Qt5 instead of Qt4" OFF)

if (USE_QT5)
	find_package (Qt5Widgets REQUIRED)
	find_package (Qt5Core REQUIRED)
	find_package (Qt5OpenGL REQUIRED)
	find_package (Qt5Network REQUIRED)
	set (CMAKE_AUTOMOC ON)
else()
	find_package (Qt4 REQUIRED)
endif()

find_package (OpenGL REQUIRED)

add_custom_target (revision_check ALL
	COMMAND python "${CMAKE_SOURCE_DIR}/tools/updaterevision.py" hginfo.h
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories (${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties (${CMAKE_BINARY_DIR}/configurationvaluebag.cpp PROPERTIES GENERATED TRUE)

set (LDFORGE_SOURCES
	src/addObjectDialog.cpp
	src/basics.cpp
	src/colors.cpp
	src/crashCatcher.cpp
	src/dialogs.cpp
	src/documentation.cpp
	src/documentloader.cpp
	src/editHistory.cpp
	src/glRenderer.cpp
	src/glCompiler.cpp
	src/guiutilities.cpp
	src/hierarchyelement.cpp
	src/ldDocument.cpp
	src/ldObject.cpp
	src/ldObjectMath.cpp
	src/ldpaths.cpp
	src/main.cpp
	src/mainwindow.cpp
	src/messageLog.cpp
	src/miscallenous.cpp
	src/partDownloader.cpp
	src/primitives.cpp
	src/radioGroup.cpp
	src/ringFinder.cpp
	src/version.cpp
	src/dialogs/colorselector.cpp
	src/dialogs/configdialog.cpp
	src/dialogs/ldrawpathdialog.cpp
	src/dialogs/newpartdialog.cpp
	src/dialogs/openprogressdialog.cpp
	src/editmodes/abstractEditMode.cpp
	src/editmodes/circleMode.cpp
	src/editmodes/drawMode.cpp
    src/editmodes/linePathMode.cpp
	src/editmodes/magicWandMode.cpp
	src/editmodes/rectangleMode.cpp
	src/editmodes/selectMode.cpp
	src/toolsets/algorithmtoolset.cpp
	src/toolsets/basictoolset.cpp
	src/toolsets/extprogramtoolset.cpp
	src/toolsets/filetoolset.cpp
	src/toolsets/movetoolset.cpp
	src/toolsets/toolset.cpp
	src/toolsets/viewtoolset.cpp
)

set (LDFORGE_HEADERS
	src/addObjectDialog.h
	src/basics.h
	src/colors.h
	src/crashCatcher.h
	src/dialogs.h
	src/documentation.h
	src/documentloader.h
	src/doublemap.h
	src/editHistory.h
	src/format.h
	src/glCompiler.h
	src/glRenderer.h
	src/guiutilities.h
	src/hierarchyelement.h
	src/ldDocument.h
	src/ldObject.h
	src/ldObjectMath.h
	src/ldpaths.h
	src/macros.h
	src/main.h
	src/mainwindow.h
	src/messageLog.h
	src/miscallenous.h
	src/partDownloader.h
	src/primitives.h
	src/radioGroup.h
	src/ringFinder.h
	src/dialogs/colorselector.h
	src/dialogs/configdialog.h
	src/dialogs/ldrawpathdialog.h
	src/dialogs/newpartdialog.h
	src/dialogs/openprogressdialog.h
	src/editmodes/abstractEditMode.h
	src/editmodes/circleMode.h
	src/editmodes/drawMode.h
	src/editmodes/linePathMode.h
	src/editmodes/magicWandMode.h
	src/editmodes/rectangleMode.h
	src/editmodes/selectMode.h
	src/toolsets/algorithmtoolset.h
	src/toolsets/basictoolset.h
	src/toolsets/extprogramtoolset.h
	src/toolsets/filetoolset.h
	src/toolsets/movetoolset.h
	src/toolsets/toolset.h
	src/toolsets/viewtoolset.h
)

set (LDFORGE_FORMS
	ui/about.ui
	ui/addhistoryline.ui
	ui/coverer.ui
	ui/downloadfrom.ui
	ui/edger2.ui
	ui/editraw.ui
	ui/extprogpath.ui
	ui/flip.ui
	ui/intersector.ui
	ui/isecalc.ui
	ui/makeprim.ui
	ui/overlay.ui
	ui/rectifier.ui
	ui/replcoords.ui
	ui/rotpoint.ui
	ui/ytruder.ui
	src/mainwindow.ui
	src/dialogs/colorselector.ui
	src/dialogs/configdialog.ui
	src/dialogs/ldrawpathdialog.ui
	src/dialogs/newpartdialog.ui
	src/dialogs/openprogressdialog.ui
)

set (LDFORGE_RESOURCES ldforge.qrc)
# set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lGLU")

if (NOT MSVC)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")
endif()

if (TRANSPARENT_DIRECT_COLORS)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRANSPARENT_DIRECT_COLORS")
endif()

# With clang, we need to set -Wno-deprecated since Qt headers seem to use the
# register keyword which clang doesn't seem to like. It also appears that Qt
# doesn't set Q_COMPILER_INITIALIZER_LISTS when compiling with clang? What's
# up with that?
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
endif()

include_directories ("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_BINARY_DIR}/src")
include_directories ("${PROJECT_BINARY_DIR}/src/misc")

if (NOT MSVC)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
	endif()
endif()

if (USE_QT5)
	qt5_generate_moc (LDFORGE_MOC ${LDFORGE_HEADERS})
	qt5_add_resources (LDFORGE_QRC ${LDFORGE_RESOURCES})
	qt5_wrap_ui (LDFORGE_FORMS_HEADERS ${LDFORGE_FORMS})
	add_executable (ldforge WIN32 ${LDFORGE_SOURCES} ${LDFORGE_HEADERS} ${LDFORGE_MOC}
		${LDFORGE_QRC} ${LDFORGE_FORMS_HEADERS}
		${CMAKE_BINARY_DIR}/configurationvaluebag.cpp)
else()
	qt4_wrap_cpp (LDFORGE_MOC ${LDFORGE_HEADERS})
	qt4_wrap_ui (LDFORGE_FORMS_HEADERS ${LDFORGE_FORMS})
	qt4_add_resources (LDFORGE_RCC ${LDFORGE_RESOURCES})
	add_executable (ldforge WIN32 ${LDFORGE_SOURCES} ${LDFORGE_HEADERS} ${LDFORGE_RCC}
		${LDFORGE_FORMS_HEADERS} ${LDFORGE_MOC}
		${CMAKE_BINARY_DIR}/configurationvaluebag.cpp)
endif()

if (USE_QT5)
	target_link_libraries (ldforge Qt5::Widgets Qt5::Network Qt5::OpenGL ${OPENGL_LIBRARIES})
else()
	target_link_libraries (ldforge
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		${QT_QTNETWORK_LIBRARY}
		${QT_QTOPENGL_LIBRARY}
		${OPENGL_LIBRARIES}
	)
endif()

add_custom_target (config_collection ALL
	COMMAND python
		"${CMAKE_SOURCE_DIR}/tools/configcollector.py"
		--header ${CMAKE_BINARY_DIR}/configurationvaluebag.h
		--source ${CMAKE_BINARY_DIR}/configurationvaluebag.cpp
		--sourcedir ${CMAKE_SOURCE_DIR}/src
		${LDFORGE_SOURCES}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_dependencies (ldforge revision_check config_collection)
install (TARGETS ldforge RUNTIME DESTINATION bin)
