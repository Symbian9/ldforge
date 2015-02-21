######################################################################
#
#	CMake file for LDForge.
#	The original QMake file was created on Sat Sep 22 17:29:49 2012
#
######################################################################

project (ldforge)
add_subdirectory (codegen)
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

get_target_property (CODEGEN_EXE codegen LOCATION)
include_directories (${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set (LDFORGE_SOURCES
	src/actions.cc
	src/actionsEdit.cc
	src/addObjectDialog.cc
	src/basics.cc
	src/colors.cc
	src/colorSelector.cc
	src/configuration.cc
	src/configDialog.cc
	src/crashCatcher.cc
	src/dialogs.cc
	src/documentation.cc
	src/editHistory.cc
	src/extPrograms.cc
	src/glRenderer.cc
	src/glCompiler.cc
	src/ldConfig.cc
	src/ldDocument.cc
	src/ldObject.cc
    src/ldObjectMath.cpp
	src/main.cc
	src/mainWindow.cc
	src/messageLog.cc
	src/miscallenous.cc
	src/partDownloader.cc
	src/primitives.cc
	src/radioGroup.cc
	src/ringFinder.cc
	src/version.cc
	src/editmodes/abstractEditMode.cc
	src/editmodes/circleMode.cc
	src/editmodes/drawMode.cc
    src/editmodes/linePathMode.cpp
	src/editmodes/magicWandMode.cc
	src/editmodes/rectangleMode.cc
	src/editmodes/selectMode.cc
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
	ui/about.ui
	ui/addhistoryline.ui
	ui/bombbox.ui
	ui/colorsel.ui
	ui/config.ui
	ui/coverer.ui
	ui/downloadfrom.ui
	ui/edger2.ui
	ui/editraw.ui
	ui/extprogpath.ui
	ui/flip.ui
	ui/intersector.ui
	ui/isecalc.ui
	ui/ldforge.ui
	ui/ldrawpath.ui
	ui/makeprim.ui
	ui/newpart.ui
	ui/openprogress.ui
	ui/overlay.ui
	ui/rectifier.ui
	ui/replcoords.ui
	ui/rotpoint.ui
	ui/ytruder.ui
)

add_custom_target (codegeneration ALL
	COMMAND ${CODEGEN_EXE} ${LDFORGE_SOURCES} ${CMAKE_BINARY_DIR}/configuration.inc
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS codegen)

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
	target_link_libraries (${PROJECT_NAME} Qt5::Widgets Qt5::Network Qt5::OpenGL ${OPENGL_LIBRARIES})
else()
	target_link_libraries (${PROJECT_NAME}
		${QT_QTCORE_LIBRARY}
		${QT_QTGUI_LIBRARY}
		${QT_QTNETWORK_LIBRARY}
		${QT_QTOPENGL_LIBRARY}
		${OPENGL_LIBRARIES}
	)
endif()

add_dependencies (${PROJECT_NAME} revision_check codegeneration)
install (TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

add_custom_target (make_hginfo_h
	COMMAND python
		"${CMAKE_SOURCE_DIR}/updaterevision.py"
		"${CMAKE_CURRENT_BINARY_DIR}/hginfo.h")
add_dependencies (${PROJECT_NAME} make_hginfo_h)