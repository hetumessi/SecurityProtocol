project(client LANGUAGES CXX)

file(GLOB_RECURSE SRC_FILES "*.cpp" "*.h")

add_executable(client ${SRC_FILES})

target_link_libraries(client OpenSSL::Crypto)


