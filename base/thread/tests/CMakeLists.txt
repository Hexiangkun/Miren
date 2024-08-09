add_executable(Atomic_test Atomic_test.cpp)
target_link_libraries(Atomic_test base_thread)

add_executable(Thread_test Thread_test.cpp)
target_link_libraries(Thread_test base base_thread)