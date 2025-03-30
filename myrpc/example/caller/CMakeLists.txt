set(SRC_LIST callloginservice.cc ../login.pb.cc)
add_executable(loginer ${SRC_LIST})

target_link_libraries(loginer mymuduo protobuf myrpc)