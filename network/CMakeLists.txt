add_library(echorestore_protocol_module STATIC src/protocol/packet.cpp)
add_library(echorestore::protocol ALIAS echorestore_protocol_module)

target_include_directories(echorestore_protocol_module PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(echorestore_protocol_module PUBLIC echorestore_options)

add_library(echorestore_network_module STATIC
    src/sender.cpp
    src/receiver.cpp
    src/emulator.cpp)
add_library(echorestore::network ALIAS echorestore_network_module)

target_include_directories(echorestore_network_module PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(echorestore_network_module PUBLIC echorestore_options
    PRIVATE echorestore::protocol echorestore::audio)

foreach(app IN ITEMS sender receiver emulator)
    add_executable(echorestore_${app} apps/${app}.cpp)

    target_link_libraries(echorestore_${app} PRIVATE echorestore::network)
endforeach()
