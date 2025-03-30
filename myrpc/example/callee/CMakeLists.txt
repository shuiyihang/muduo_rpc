
set(SRC_LIST loginservice.cc ../login.pb.cc)
add_executable(provider ${SRC_LIST})
# 依赖库应该放在被依赖库的后面
target_link_libraries(provider mymuduo protobuf myrpc)