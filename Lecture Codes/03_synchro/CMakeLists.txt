cmake_minimum_required(VERSION 3.10)

link_libraries(pthread rt)

add_executable(03synchro_01_base 01_base.c)
add_executable(03synchro_02_peterson 02_peterson.c)
add_executable(03synchro_03_peterson_atomic 03_peterson_atomic.c)
add_executable(03synchro_04_atomic_swap 04_atomic_swap.c)
add_executable(03synchro_05_atomic_test_and_set 05_atomic_test_and_set.c)
add_executable(03synchro_06_mutex 06_mutex.c)
add_executable(03synchro_07_semaphore 07_semaphore.c)
add_executable(03synchro_08_bounded_buffer 08_bounded_buffer.c)
add_executable(03synchro_09_readers_writers 09_reades_writers.c)
add_executable(03synchro_11_sem_open 11_sem_open.c)
add_executable(03synchro_12_sem_in_shm 12_sem_in_shm.c)
add_executable(03synchro_13_mtx_attributes 13_mtx_attributes.c)
add_executable(03synchro_14_mtx_in_shm 14_mtx_in_shm.c)
add_executable(03synchro_15_cv 15_cv.c)
add_executable(03synchro_16_dining_philosophers 16_dining_philosophers.c)

add_executable(03synchro_99_reordering 99_reordering.c)
