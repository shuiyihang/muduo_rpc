
aux_source_directory(. SRC_LIST)
add_library(myrpc SHARED ${SRC_LIST})
target_link_libraries(myrpc mymuduo)