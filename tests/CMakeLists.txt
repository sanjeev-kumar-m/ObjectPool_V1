set(This
    memory_pool_test)

set(Sources 
    src/memory_pool_tests.cpp)

add_executable(${This} ${Sources})

target_link_libraries(${This}
  gtest_main
  object_pool
)

add_test(
  NAME ${This}
  COMMAND ${This}
)
