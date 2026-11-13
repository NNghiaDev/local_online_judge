# Presentation Layer (GUI)
add_library(app_gui STATIC
    AppWindow.hpp
    AppWindow.cpp
    views/DashboardView.hpp
    views/DashboardView.cpp
    views/TestcaseManagerView.hpp
    views/TestcaseManagerView.cpp
    views/LeaderboardView.hpp
    views/LeaderboardView.cpp
    views/ConsoleLogView.hpp
    views/ConsoleLogView.cpp
)

target_include_directories(app_gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(app_gui PUBLIC
    app_common
    app_bll
    app_dal
    imgui
    glfw
    OpenGL::GL
)
