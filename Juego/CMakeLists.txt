cmake_minimum_required(VERSION 3.16)

project(OperacionMacondo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    Juego.h
    Juego.cpp
    Nivel.h
    Nivel.cpp
    Entidades.h
    Entidades.cpp
    PersonajePrincipal.h
    PersonajePrincipal.cpp
    Anciano.h
    Anciano.cpp
    ObjetosEstaticos.h
    ObjetosEstaticos.cpp
    ObjetosArmas.h
    ObjetosArmas.cpp
    EventoJuego.h
    GestorAudio.h
    GestorAudio.cpp
    recursos.qrc
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(ProyectoFinal
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(ProyectoFinal ${PROJECT_SOURCES})
endif()

target_link_libraries(ProyectoFinal PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ProyectoFinal)
endif()
