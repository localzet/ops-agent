cmake_minimum_required(VERSION 3.24)

project(ops-agent VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(OPS_AGENT_BUILD_TESTS "Build tests" ON)

include(FetchContent)

FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-30-2.tar.gz
)
FetchContent_GetProperties(asio)
if(NOT asio_POPULATED)
    FetchContent_Populate(asio)
endif()
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "Standalone Asio include path" FORCE)

set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.3.2
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.3
)

FetchContent_MakeAvailable(Crow nlohmann_json spdlog)

find_package(SQLite3)
find_package(CURL)
find_package(Threads REQUIRED)

add_library(ops_agent_lib
    src/common/Clock.cpp
    src/app/Application.cpp
    src/config/ConfigLoader.cpp
    src/features/health/HealthService.cpp
    src/features/metrics/MetricsService.cpp
    src/features/services/ServicesService.cpp
    src/features/system/SystemService.cpp
    src/features/tcp_check/TcpCheckService.cpp
    src/infrastructure/http/HttpResponse.cpp
    src/infrastructure/linux/HostInfoProvider.cpp
    src/infrastructure/linux/ProcfsSystemProbe.cpp
    src/infrastructure/linux/SocketTcpClient.cpp
    src/infrastructure/linux/SystemdServiceManager.cpp
    src/infrastructure/persistence/SQLiteConnection.cpp
)

target_include_directories(ops_agent_lib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(ops_agent_lib
    PUBLIC
        OPS_AGENT_VERSION="${PROJECT_VERSION}"
)

target_compile_options(ops_agent_lib
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

target_link_libraries(ops_agent_lib
    PUBLIC
        Crow::Crow
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
)

if(SQLite3_FOUND)
    target_compile_definitions(ops_agent_lib PUBLIC OPS_AGENT_HAS_SQLITE=1)
    target_link_libraries(ops_agent_lib PUBLIC SQLite::SQLite3)
else()
    message(WARNING "SQLite3 development package was not found. Building with SQLite placeholder.")
endif()

if(CURL_FOUND)
    target_compile_definitions(ops_agent_lib PUBLIC OPS_AGENT_HAS_CURL=1)
    target_link_libraries(ops_agent_lib PUBLIC CURL::libcurl)
else()
    message(WARNING "libcurl development package was not found. Future integration hooks are disabled.")
endif()

if(WIN32)
    target_link_libraries(ops_agent_lib PUBLIC ws2_32 mswsock)
endif()

add_executable(ops-agent src/main.cpp)
target_link_libraries(ops-agent PRIVATE ops_agent_lib)

install(TARGETS ops-agent RUNTIME DESTINATION bin)
install(FILES config/config.example.yml DESTINATION share/ops-agent)

if(OPS_AGENT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
