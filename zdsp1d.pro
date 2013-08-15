TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release
CONFIG	+= debug_and_release

HEADERS	+= zdspglobal.h \
	parse.h \
	scpi.h \
	zdsp1d.h \
	cmdinterpret.h \
	zhserver.h \
	dsp.h \
	zeraglobal.h \
	dsp1scpi.h \
    xmlsettings.h \
    ethsettings.h \
    debugsettings.h

SOURCES	+= main.cpp \
	parse.cpp \
	cmdinterpret.cpp \
	cmds.cpp \
	zdsp1d.cpp \
	zhserver.cpp \
	dsp.cpp \
	scpi.cpp \
    debugsettings.cpp \
    ethsettings.cpp

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

#The following line was inserted by qt3to4
QT +=  qt3support 

target.path = /usr/bin
INSTALLS += target

