TEMPLATE	= app
LANGUAGE	= C++

include(zdsp1d.user.pri)

QMAKE_CXXFLAGS += -O0

LIBS +=  -lzeranet
LIBS +=  -lzeranetclient
LIBS +=  -lzeraxmlconfig
LIBS +=  -lprotobuf
LIBS +=  -lzera-resourcemanager-protobuf



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
    debugsettings.h \
    dspsettings.h \
    rmconnection.h

SOURCES	+= main.cpp \
	parse.cpp \
	cmdinterpret.cpp \
	cmds.cpp \
	zdsp1d.cpp \
	zhserver.cpp \
	dsp.cpp \
	scpi.cpp \
    debugsettings.cpp \
    ethsettings.cpp \
    dspsettings.cpp \
    rmconnection.cpp

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

#The following line was inserted by qt3to4
QT +=  qt3support xml network

target.path = /usr/bin
INSTALLS += target

configxml.path = /etc/zera/zdsp1d
configxml.files = zdsp1d.xsd \
                  zdsp1d.xml

OTHER_FILES +=
