project(server LANGUAGES CXX)

file(GLOB_RECURSE SRC_FILES "*.cpp" "*.h")

add_executable(server ${SRC_FILES})

target_link_libraries(server OpenSSL::Crypto)
