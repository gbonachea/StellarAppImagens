cmake_minimum_required(VERSION 3.20)
project(StellarAppImagens VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(StellarAppImagens
    main.cpp
    mainwindow.h
    mainwindow.cpp
    appimagebuilder.h
    appimagebuilder.cpp
    resources.qrc
)

target_link_libraries(StellarAppImagens PRIVATE Qt6::Widgets)

install(TARGETS StellarAppImagens DESTINATION bin)

add_custom_target(portable
    COMMAND "${CMAKE_SOURCE_DIR}/create_portable.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generando paquete portable de StellarAppImagens..."
    VERBATIM
)
