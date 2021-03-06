######################################################################
#
#	CMake file for LDForge.
#	The original QMake file was created on Sat Sep 22 17:29:49 2012
#
######################################################################

project (ldforge)
cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(cotire)

option (TRANSPARENT_DIRECT_COLORS "Enables non-standard transparent direct colors" OFF)

find_package (Qt5Widgets REQUIRED)
find_package (Qt5Core REQUIRED)
find_package (Qt5OpenGL REQUIRED)
find_package (Qt5Network REQUIRED)

if (Qt5Widgets_VERSION VERSION_LESS 5.5.0)
	message(FATAL_ERROR "Qt5 version 5.5 required")
endif()

set (CMAKE_AUTOMOC ON)

find_package (OpenGL REQUIRED)

add_custom_target (revision_check ALL
	COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/updaterevision.py" --cwd "$(CMAKE_SOURCE_DIR)" hginfo.h
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories (${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set_source_files_properties (${CMAKE_BINARY_DIR}/configuration.cpp PROPERTIES GENERATED TRUE)
set_property(SOURCE configuration.cpp PROPERTY SKIP_AUTOMOC ON)

set (LDFORGE_SOURCES
	src/basics.cpp
	src/canvas.cpp
	src/colors.cpp
	src/crashCatcher.cpp
	src/documentmanager.cpp
	src/editHistory.cpp
	src/glcamera.cpp
	src/glcompiler.cpp
	src/glrenderer.cpp
	src/grid.cpp
	src/guiutilities.cpp
	src/headerhistorymodel.cpp
	src/hierarchyelement.cpp
	src/lddocument.cpp
	src/librariesmodel.cpp
	src/main.cpp
	src/mainwindow.cpp
	src/model.cpp
	src/partdownloader.cpp
	src/partdownloadrequest.cpp
	src/parser.cpp
	src/primitives.cpp
	src/serializer.cpp
	src/ringFinder.cpp
	src/version.cpp
	src/algorithms/geometry.cpp
	src/algorithms/invert.cpp
	src/dialogs/colortoolbareditor.cpp
	src/dialogs/colorselector.cpp
	src/dialogs/configdialog.cpp
	src/dialogs/circularprimitiveeditor.cpp
	src/dialogs/externalprogrampathdialog.cpp
	src/dialogs/generateprimitivedialog.cpp
	src/dialogs/ldrawpathdialog.cpp
	src/dialogs/shortcutsmodel.cpp
	src/dialogs/subfilereferenceeditor.cpp
	src/dialogs/newpartdialog.cpp
	src/editmodes/abstractEditMode.cpp
	src/editmodes/circleMode.cpp
	src/editmodes/curvemode.cpp
	src/editmodes/drawMode.cpp
	src/editmodes/linePathMode.cpp
	src/editmodes/magicWandMode.cpp
	src/editmodes/rectangleMode.cpp
	src/editmodes/selectMode.cpp
	src/geometry/linesegment.cpp
	src/geometry/plane.cpp
	src/linetypes/circularprimitive.cpp
	src/linetypes/comment.cpp
	src/linetypes/conditionaledge.cpp
	src/linetypes/edgeline.cpp
	src/linetypes/empty.cpp
	src/linetypes/modelobject.cpp
	src/linetypes/quadrilateral.cpp
	src/linetypes/triangle.cpp
	src/toolsets/algorithmtoolset.cpp
	src/toolsets/basictoolset.cpp
	src/toolsets/extprogramtoolset.cpp
	src/toolsets/filetoolset.cpp
	src/toolsets/movetoolset.cpp
	src/toolsets/toolset.cpp
	src/toolsets/viewtoolset.cpp
	src/types/boundingbox.cpp
	src/types/vertex.cpp
	src/widgets/circularsectioneditor.cpp
	src/widgets/colorbutton.cpp
	src/widgets/doublespinbox.cpp
	src/widgets/extendedkeysequenceeditor.cpp
	src/widgets/headeredit.cpp
	src/widgets/matrixeditor.cpp
	src/widgets/vertexobjecteditor.cpp
)

set (LDFORGE_HEADERS
	src/basics.h
	src/canvas.h
	src/colors.h
	src/crashCatcher.h
	src/documentmanager.h
	src/editHistory.h
	src/format.h
	src/glcamera.h
	src/glcompiler.h
	src/glrenderer.h
	src/glShared.h
	src/grid.h
	src/guiutilities.h
	src/headerhistorymodel.h
	src/hierarchyelement.h
	src/lddocument.h
	src/ldobjectiterator.h
	src/librariesmodel.h
	src/main.h
	src/mainwindow.h
	src/model.h
	src/partdownloader.h
	src/partdownloadrequest.h
	src/parser.h
	src/primitives.h
	src/ringFinder.h
	src/serializer.h
	src/version.h
	src/algorithms/geometry.h
	src/algorithms/invert.h
	src/dialogs/colorselector.h
	src/dialogs/colortoolbareditor.h
	src/dialogs/configdialog.h
	src/dialogs/circularprimitiveeditor.h
	src/dialogs/externalprogrampathdialog.h
	src/dialogs/generateprimitivedialog.h
	src/dialogs/ldrawpathdialog.h
	src/dialogs/newpartdialog.h
	src/dialogs/shortcutsmodel.h
	src/dialogs/subfilereferenceeditor.h
	src/editmodes/abstractEditMode.h
	src/editmodes/circleMode.h
	src/editmodes/curvemode.h
	src/editmodes/drawMode.h
	src/editmodes/linePathMode.h
	src/editmodes/magicWandMode.h
	src/editmodes/rectangleMode.h
	src/editmodes/selectMode.h
	src/generics/enums.h
	src/generics/functions.h
	src/generics/migrate.h
	src/generics/oneof.h
	src/generics/range.h
	src/generics/reverse.h
	src/generics/ring.h
	src/generics/transform.h
	src/geometry/linesegment.h
	src/geometry/plane.h
	src/linetypes/circularprimitive.h
	src/linetypes/comment.h
	src/linetypes/conditionaledge.h
	src/linetypes/edgeline.h
	src/linetypes/empty.h
	src/linetypes/modelobject.h
	src/linetypes/quadrilateral.h
	src/linetypes/triangle.h
	src/toolsets/algorithmtoolset.h
	src/toolsets/basictoolset.h
	src/toolsets/extprogramtoolset.h
	src/toolsets/filetoolset.h
	src/toolsets/movetoolset.h
	src/toolsets/toolset.h
	src/toolsets/viewtoolset.h
	src/types/boundingbox.h
	src/types/library.h
	src/types/vertex.h
	src/widgets/circularsectioneditor.h
	src/widgets/colorbutton.h
	src/widgets/doublespinbox.h
	src/widgets/extendedkeysequenceeditor.h
	src/widgets/headeredit.h
	src/widgets/matrixeditor.h
	src/widgets/vertexobjecteditor.h
)

set (LDFORGE_FORMS
	src/dialogs/aboutdialog.ui
	src/dialogs/colorselector.ui
	src/dialogs/colortoolbareditor.ui
	src/dialogs/configdialog.ui
	src/dialogs/covererdialog.ui
	src/dialogs/circularprimitiveeditor.ui
	src/dialogs/edger2dialog.ui
	src/dialogs/editrawdialog.ui
	src/dialogs/externalprogrampathdialog.ui
	src/dialogs/flipdialog.ui
	src/dialogs/generateprimitivedialog.ui
	src/dialogs/intersectordialog.ui
	src/dialogs/isecalcdialog.ui
	src/dialogs/ldrawpathdialog.ui
	src/dialogs/newpartdialog.ui
	src/dialogs/rectifierdialog.ui
	src/dialogs/replacecoordinatesdialog.ui
	src/dialogs/rotationpointdialog.ui
	src/dialogs/subfilereferenceeditor.ui
	src/dialogs/ytruderdialog.ui
	src/toolsets/fixroundingerrors.ui
	src/mainwindow.ui
	src/partdownloader.ui
	src/widgets/circularsectioneditor.ui
	src/widgets/vertexobjecteditor.ui
	src/widgets/headeredit.ui
	src/widgets/matrixeditor.ui
)

set (LDFORGE_OTHER_FILES
	src/configurationoptions.txt
	data/primitive-categories.cfg
)

set (LDFORGE_RESOURCES ldforge.qrc)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
include_directories ("${PROJECT_SOURCE_DIR}/src")

if (NOT MSVC)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
		set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
	endif()
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=all -Wextra")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-implicit-fallthrough -Wno-noexcept-type")
endif()

qt5_add_resources (LDFORGE_QRC ${LDFORGE_RESOURCES})
qt5_wrap_ui (LDFORGE_FORMS_HEADERS ${LDFORGE_FORMS})
add_executable (ldforge WIN32 ${LDFORGE_SOURCES} ${LDFORGE_HEADERS} ${LDFORGE_OTHER_FILES}
	${LDFORGE_QRC} ${LDFORGE_FORMS_HEADERS}
	${CMAKE_BINARY_DIR}/configuration.cpp)

set_source_files_properties(${LDFORGE_HEADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${LDFORGE_OTHER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_target_properties (ldforge PROPERTIES AUTOMOC 1)
target_link_libraries (ldforge Qt5::Widgets Qt5::Network Qt5::OpenGL ${OPENGL_LIBRARIES})
cotire(ldforge)

add_custom_target (config_collection ALL
	COMMAND python3
		"${CMAKE_SOURCE_DIR}/tools/configcollector.py"
		--header ${CMAKE_BINARY_DIR}/configuration.h
		--source ${CMAKE_BINARY_DIR}/configuration.cpp
		--sourcedir ${CMAKE_SOURCE_DIR}/src
		${CMAKE_SOURCE_DIR}/src/configurationoptions.txt
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(lengthcheck ALL
	COMMAND python
		"${CMAKE_SOURCE_DIR}/tools/linelength.py"
		${LDFORGE_SOURCES}
		${LDFORGE_HEADERS}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_dependencies (ldforge revision_check config_collection)
install (TARGETS ldforge RUNTIME DESTINATION bin)
