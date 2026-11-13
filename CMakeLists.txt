cmake_minimum_required(VERSION 3.20)
project(LocalOnlineJudge VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Output Directories
# ---------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------------------------
# Dependencies Management
# ---------------------------------------------------------------------------
include(FetchContent)

# 1. GLFW (Windowing & Context for Dear ImGui)
message(STATUS "Configuring GLFW...")
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 2. Dear ImGui
message(STATUS "Configuring Dear ImGui...")
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# ImGui Static Library Target
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# 3. MySQL / MariaDB Client Library
set(MYSQL_DIR "" CACHE PATH "Path to MySQL / MariaDB installation directory")

find_path(MYSQL_INCLUDE_DIR
    NAMES mysql.h mariadb/mysql.h mysql/mysql.h
    PATHS
        ${MYSQL_DIR}/include
        "C:/msys64/ucrt64/include/mariadb"
        "C:/msys64/ucrt64/include/mysql"
        "C:/msys64/ucrt64/include"
        "C:/Program Files/MySQL/Connector C++ 8.0/include"
        "C:/Program Files/MySQL/MySQL Server 8.0/include"
)

find_library(MYSQL_LIBRARY
    NAMES mariadb libmariadb libmariadb.dll mysqlclient libmysql mysqlcppconn
    PATHS
        ${MYSQL_DIR}/lib
        "C:/msys64/ucrt64/lib"
        "C:/Program Files/MySQL/Connector C++ 8.0/lib64"
        "C:/Program Files/MySQL/MySQL Server 8.0/lib"
)

if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    message(STATUS "Found MySQL/MariaDB include: ${MYSQL_INCLUDE_DIR}")
    message(STATUS "Found MySQL/MariaDB library: ${MYSQL_LIBRARY}")
    add_library(MySQL::Connector UNKNOWN IMPORTED)
    set_target_properties(MySQL::Connector PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MYSQL_INCLUDE_DIR}"
        IMPORTED_LOCATION "${MYSQL_LIBRARY}"
    )
    add_compile_definitions(HAS_MYSQL=1)
else()
    message(WARNING "MySQL/MariaDB client library not found. Falling back to local cache mode.")
endif()

# 4. OpenGL & Windows System Libraries
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# Project Include Paths
# ---------------------------------------------------------------------------
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# Layer Subdirectories
# ---------------------------------------------------------------------------
add_subdirectory(src/common)
add_subdirectory(src/dal)
add_subdirectory(src/bll)
add_subdirectory(src/gui)

# ---------------------------------------------------------------------------
# Main Executable
# ---------------------------------------------------------------------------
add_executable(LocalOnlineJudge src/main.cpp)

target_link_libraries(LocalOnlineJudge PRIVATE
    app_gui
    app_bll
    app_dal
    app_common
    imgui
    glfw
    OpenGL::GL
)

if(TARGET MySQL::Connector)
    target_link_libraries(LocalOnlineJudge PRIVATE MySQL::Connector)
endif()

# Windows API libraries for process management, pipes, job objects
if(WIN32)
    target_link_libraries(LocalOnlineJudge PRIVATE
        psapi      # Process memory query (GetProcessMemoryInfo)
        user32
        gdi32
        shell32
    )
endif()
