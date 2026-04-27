cmake_minimum_required(VERSION 3.16)
project(OOPLabProject VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    SerialPort
    Sql
    PrintSupport
)

add_executable(OOPLabProject
    main.cpp
    database.h
    database.cpp
    arduinocontroller.h
    arduinocontroller.cpp
    loginwindow.h
    loginwindow.cpp
    registerwindow.h
    registerwindow.cpp
    edithousewindow.h
    edithousewindow.cpp
    dashboardwindow.h
    dashboardwindow.cpp
    scheduledialog.h
    scheduledialog.cpp
    devicecard.h
    devicecard.cpp
)

target_link_libraries(OOPLabProject PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Sql
    Qt6::PrintSupport
)