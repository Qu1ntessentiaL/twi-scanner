cmake_minimum_required(VERSION 3.15)

project(twi-scanner VERSION 0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "~/Qt/6.10.1/gcc_64/lib/cmake")
find_package(Qt6 6.10 REQUIRED COMPONENTS Quick SerialPort)

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(${PROJECT_NAME}
        cxx-src/main.cpp
        cxx-src/ft4222.cpp
        cxx-src/i2cscanner.cpp
        cxx-src/ssd1306/ssd1306.cpp
        cxx-src/ssd1306/fonts.c
)

qt_add_qml_module(${PROJECT_NAME}
        URI qt-qml
        VERSION 1.0
        QML_FILES
        qml-src/Main.qml
        qml-src/SI7210Window.qml
)

include_directories(cxx-src)
include_directories(cxx-src/ssd1306)

include_directories(/usr/local/include)   # libft4222.h
link_directories(/usr/local/lib)          # libft4222.so

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt6::Quick
        Qt6::SerialPort
        ft4222
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
