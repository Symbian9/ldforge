######################################################################
# Automatically generated by qmake (2.01a) Sat Sep 22 17:29:49 2012
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
OBJECTS_DIR = build/
RC_FILE = ldforge.rc

# Input
HEADERS += bbox.h \
	common.h \
	gldraw.h \
	gui.h \
	file.h \
	ldtypes.h \
	misc.h \
	str.h \
	config.h \
	colors.h \
	types.h \
	zz_addObjectDialog.h \
	zz_colorSelectDialog.h \
	zz_configDialog.h \
	zz_newPartDialog.h \
	zz_setContentsDialog.h

SOURCES += bbox.cpp \
	config.cpp \
	colors.cpp \
	gldraw.cpp \
	gui.cpp \
	file.cpp \
	ldtypes.cpp \
	main.cpp \
	misc.cpp \
	str.cpp \ 
	types.cpp \
	zz_addObjectDialog.cpp \
	zz_colorSelectDialog.cpp \
	zz_configDialog.cpp \
	zz_newPartDialog.cpp \
	zz_setContentsDialog.cpp

QMAKE_CXXFLAGS += -std=c++0x
QT += opengl
LIBS += -lGLU