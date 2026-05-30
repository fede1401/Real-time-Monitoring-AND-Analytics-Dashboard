# Versione minima CMake
cmake_minimum_required(VERSION 3.15)

# Nome progetto
project(MyApp VERSION 1.0)

# Standard C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Eseguibile
add_executable(${PROJECT_NAME} src/main.cpp
    src/Server/listener.h src/Server/listener.cpp
    src/common.h
    src/Parser/Marshal.h src/Parser/Marshal.cpp
    src/Model/Request.h
    src/Server/Agent.h src/Server/Agent.cpp
    src/Model/SystemMetrics.h
    src/Utility/ManageString.h src/Utility/ManageString.cpp
    src/includeClass.h
    src/LogPrint/Logint.h src/LogPrint/Logint.cpp
    src/Server/Server.h src/Server/Server.cpp
    )

target_include_directories(${PROJECT_NAME} PRIVATE third_party)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WEB_ROOT="${CMAKE_SOURCE_DIR}/src/web"
)