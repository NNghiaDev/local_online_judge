# Business Logic Layer (BLL)
add_library(app_bll STATIC
    JudgeEngine.hpp
    JudgeEngine.cpp
    sandbox/ProcessSandbox.hpp
    sandbox/ProcessSandbox.cpp
    compiler/CompilerService.hpp
    compiler/CompilerService.cpp
    checker/CheckerService.hpp
    checker/CheckerService.cpp
    generator/ModelCodeRunner.hpp
    generator/ModelCodeRunner.cpp
    strategies/IGradingStrategy.hpp
    strategies/ICPCGradingStrategy.hpp
    strategies/ICPCGradingStrategy.cpp
    strategies/IOIGradingStrategy.hpp
    strategies/IOIGradingStrategy.cpp
    strategies/EqualGradingStrategy.hpp
    strategies/EqualGradingStrategy.cpp
)

target_include_directories(app_bll PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(app_bll PUBLIC
    app_common
    app_dal
)

if(WIN32)
    target_link_libraries(app_bll PUBLIC psapi)
endif()
