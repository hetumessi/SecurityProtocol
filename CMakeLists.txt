cmake_minimum_required(VERSION 3.14)
add_compile_options(-w)
cmake_policy(SET CMP0079 NEW)

project(SecurityProtocol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@3)

find_package(OpenSSL REQUIRED)

add_subdirectory(client)
add_subdirectory(server)

install(TARGETS client server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include_directories(/usr/local/opt/openssl@3/include)

target_link_libraries(client server OpenSSL::Crypto)




