cmake_minimum_required(VERSION 3.1)

#set project name and language
project(zdsp1d LANGUAGES CXX)

#Automoc and RCC ON. Needed in QT projects
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#C++ 11 Flag
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Set project Version
set(zdsp1d_VERSION_MAJOR "1")
set(zdsp1d_VERSION_MINOR "12")
set(zdsp1d_VERSION_PATCH "0")

set(PROJECT_VERSION "${zdsp1d_VERSION_MAJOR}.${zdsp1d_VERSION_MINOR}.${zdsp1d_VERSION_PATCH}")

option(useSystemd "Enable systemd notification" ON)

#Adding cmake functions like feature_summary()
include(FeatureSummary)
include(GNUInstallDirs)

#Find dependecies
find_package(PkgConfig REQUIRED)
find_package(Qt5 COMPONENTS Core Xml Network CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(zeraprotobuf REQUIRED)
find_package(xiqnet REQUIRED)
find_package(zeraxmlconfig REQUIRED)

if(${useSystemd})
    pkg_check_modules(Systemd REQUIRED libsystemd)
    add_definitions(-DSYSTEMD_NOTIFICATION)
endif()

#sum up project Files 
file(GLOB SOURCES
    ${PROJECT_SOURCE_DIR}/*.cpp 
    )


file(GLOB HEADER
    ${PROJECT_SOURCE_DIR}/*.h  
    )

file(GLOB RESOURCES 
    ${PROJECT_SOURCE_DIR}/*.qrc
    )

set(OTHER_FILES 
    ${PROJECT_SOURCE_DIR}/zdsp1d.xsd
    ${PROJECT_SOURCE_DIR}/zdsp1d.xml
)

#create executable
add_executable(zdsp1d
    ${SOURCES}
    ${HEADER}
    ${RESOURCES}
    )

#link dependencies
target_link_libraries(zdsp1d
    PUBLIC
    Qt5::Core
    VeinMeta::xiqnet
    ${Protobuf_LIBRARIES}
    VeinMeta::zeraprotobuf
    ZeraClasses::zeraxmlconfig
    PRIVATE
    Qt5::Xml
    Qt5::Network
    )

if(${Systemd_FOUND})
    target_link_libraries(zdsp1d
        PRIVATE
        systemd
    )
endif()



# announce headers - target perspective
target_include_directories(zdsp1d
    PUBLIC
    $<BUILD_INTERFACE:${MY_LIBRARY_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/zdsp1d>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    )



#install zdsp1dto bin 
install(TARGETS zdsp1d
    EXPORT zdsp1dExport
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

install(
    FILES ${OTHER_FILES}
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/zera/zdsp1d
    )


# spawn out some info on configuration
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
