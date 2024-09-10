cmake_minimum_required (VERSION 3.8)

project ("eaxas")

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (SOURCES
    AsfWriter.cpp
    EaXaEncoder.cpp
    eaxas.cpp
    MaxisXaWriter.cpp
    WavParser.cpp
    XasWriter.cpp

    AsfWriter.h
    EaXaEncoder.h
    MaxisXaWriter.h
    utility.h
    WavParser.h
    Writer.h
    XasWriter.h
    )

add_executable (eaxas ${SOURCES})
