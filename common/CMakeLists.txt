add_library(echorestore_audio_module STATIC src/audio/wav.cpp)
add_library(echorestore::audio ALIAS echorestore_audio_module)
target_include_directories(echorestore_audio_module PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(echorestore_audio_module PUBLIC echorestore_options)

add_executable(echorestore_generate_sample apps/generate_sample.cpp)
target_link_libraries(echorestore_generate_sample PRIVATE echorestore::audio)
