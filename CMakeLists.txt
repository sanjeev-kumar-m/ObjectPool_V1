set(This object_pool)
add_library(${This} INTERFACE)
target_include_directories(${This} INTERFACE include/)
add_subdirectory(tests)
