TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on release

HEADERS	+= zdspglobal.h \
	parse.h \
	scpi.h \
	zdsp1d.h \
	cmdinterpret.h \
	zhserver.h \
	dsp.h \
	zeraglobal.h \
	dsp1scpi.h

SOURCES	+= main.cpp \
	parse.cpp \
	cmdinterpret.cpp \
	cmds.cpp \
	zdsp1d.cpp \
	zhserver.cpp \
	dsp.cpp \
	scpi.cpp

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

