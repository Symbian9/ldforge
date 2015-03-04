######################################################################
#
#	CMake file for LDForge.
#	The original QMake file was created on Sat Sep 22 17:29:49 2012
#
######################################################################

project (ldforge)
cmake_minimum_required (VERSION 2.6)
cmake_policy (SET CMP0020 OLD)

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

find_package (Libarchive)
find_package (OpenGL REQUIRED)
if (NOT LibArchive_FOUND)
	message (SEND_ERROR "Could not find Libarchive")
endif()

include_directories (${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set (LDFORGE_SOURCES
	src/actions.cpp
	src/actionsEdit.cpp
	src/addObjectDialog.cpp
	src/basics.cpp
	src/colors.cpp
	src/colorSelector.cpp
	src/configuration.cpp
	src/configDialog.cpp
	src/crashCatcher.cpp
	src/dialogs.cpp
	src/documentation.cpp
	src/editHistory.cpp
	src/extPrograms.cpp
	src/glRenderer.cpp
	src/glCompiler.cpp
	src/ldConfig.cpp
	src/ldDocument.cpp
	src/ldObject.cpp
	src/ldObjectMath.cpp
	src/ldproject.cpp
	src/main.cpp
	src/mainWindow.cpp
	src/messageLog.cpp
	src/miscallenous.cpp
	src/partDownloader.cpp
	src/primitives.cpp
	src/radioGroup.cpp
	src/ringFinder.cpp
	src/version.cpp
	src/editmodes/abstractEditMode.cpp
	src/editmodes/circleMode.cpp
	src/editmodes/drawMode.cpp
	src/editmodes/linePathMode.cpp
	src/editmodes/magicWandMode.cpp
	src/editmodes/rectangleMode.cpp
	src/editmodes/selectMode.cpp
)

set (LDFORGE_HEADERS
	src/macros.h
	src/crashCatcher.h
	src/colors.h
	src/ringFinder.h
	src/ldDocument.h
	src/addObjectDialog.h
	src/ldConfig.h
	src/partDownloader.h
	src/ldObject.h
    src/ldObjectMath.h
	src/primitives.h
	src/miscallenous.h
	src/messageLog.h
	src/dialogs.h
	src/radioGroup.h
	src/documentation.h
	src/main.h
	src/basics.h
	src/colorSelector.h
	src/configDialog.h
	src/glRenderer.h
	src/glCompiler.h
	src/configuration.h
	src/mainWindow.h
	src/editHistory.h
	src/format.h
	src/editmodes/abstractEditMode.h
	src/editmodes/circleMode.h
	src/editmodes/drawMode.h
    src/editmodes/linePathMode.h
	src/editmodes/magicWandMode.h
	src/editmodes/rectangleMode.h
	src/editmodes/selectMode.h
)

set (LDFORGE_FORMS
	src/about.ui
	src/addhistoryline.ui
	src/bombbox.ui
	src/colorsel.ui
	src/config.ui
	src/coverer.ui
	src/downloadfrom.ui
	src/edger2.ui
	src/editraw.ui
	src/extprogpath.ui
	src/flip.ui
	src/intersector.ui
	src/isecalc.ui
	src/ldforge.ui
	src/ldrawpath.ui
	src/makeprim.ui
	src/newpart.ui
	src/openprogress.ui
	src/overlay.ui
	src/rectifier.ui
	src/replcoords.ui
	src/rotpoint.ui
	src/ytruder.ui
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
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -DQ_COMPILER_INITIALIZER_LISTS")
endif()

include_directories (${LibArchive_INCLUDE_DIRS})
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
	add_executable (${PROJECT_NAME} WIN32 ${LDFORGE_SOURCES} ${LDFORGE_MOC}
		${LDFORGE_QRC} ${LDFORGE_FORMS_HEADERS})
else()
	qt4_wrap_cpp (LDFORGE_MOC ${LDFORGE_HEADERS})
	qt4_wrap_ui (LDFORGE_FORMS_HEADERS ${LDFORGE_FORMS})
	qt4_add_resources (LDFORGE_RCC ${LDFORGE_RESOURCES})
	add_executable (${PROJECT_NAME} WIN32 ${LDFORGE_SOURCES} ${LDFORGE_RCC}
		${LDFORGE_FORMS_HEADERS} ${LDFORGE_MOC})
endif()

if (USE_QT5)
	target_link_libraries (${PROJECT_NAME}
		Qt5::Widgets
		Qt5::Network
		Qt5::OpenGL
		${OPENGL_LIBRARIES}
		${LibArchive_LIBRARIES}
	)
else()
	target_link_libraries (${PROJECT_NAME}
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		${QT_QTNETWORK_LIBRARY}
		${QT_QTOPENGL_LIBRARY}
		${OPENGL_LIBRARIES}
		${LibArchive_LIBRARIES}
	)
endif()

install (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

#
# Code generators
#

add_custom_target (make_hginfo_h
	COMMAND python
		"${CMAKE_SOURCE_DIR}/codegen.py" "hginfo"
		"${CMAKE_CURRENT_BINARY_DIR}/hginfo.h"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target (make_config_aux
	COMMAND python
		"${CMAKE_SOURCE_DIR}/codegen.py" "configaux"
		"${CMAKE_CURRENT_BINARY_DIR}/config.aux"
		${LDFORGE_SOURCES}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_dependencies (${PROJECT_NAME} make_hginfo_h make_config_aux)
