cmake_minimum_required(VERSION 3.5)
add_compile_options(-w)
project(SecurityProtocol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@3)

find_package(OpenSSL REQUIRED)

add_executable(SecurityProtocol main.cpp itcastderlog.h itcast_asn1_der.h keymng_msg.h itcastderlog.cpp itcast_asn1_der.cpp)

install(TARGETS SecurityProtocol
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

include_directories(/usr/local/opt/openssl@3/include)

target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)



