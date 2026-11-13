# Common layer
add_library(app_common STATIC
    Types.hpp
    Logger.hpp
    Logger.cpp
)

target_include_directories(app_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)
