# Data Access Layer (DAL)
add_library(app_dal STATIC
    db/DatabaseConnection.hpp
    db/DatabaseConnection.cpp
    db/SubmissionDAO.hpp
    db/SubmissionDAO.cpp
    db/LeaderboardDAO.hpp
    db/LeaderboardDAO.cpp
    filesystem/FileSystemDAO.hpp
    filesystem/FileSystemDAO.cpp
)

target_include_directories(app_dal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(app_dal PUBLIC
    app_common
)

if(TARGET MySQL::Connector)
    target_link_libraries(app_dal PUBLIC MySQL::Connector)
endif()
