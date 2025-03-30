
#include "login.pb.h"
#include "rpcchannel.h"
#include <iostream>

int main()
{
    example::LoginServiceRpc_Stub stub(new MyRpcChannel());

    example::LoginRequest request;
    request.set_name("shuiyihang");
    request.set_pwd("123456");

    example::LoginResponse response;

    stub.Login(nullptr, &request, &response, nullptr);

    if(0 == response.result().errcode()){
        std::cout << "call rpc login method success, return: " << response.success() << "\n";
    }else{
        std::cout << "call rpc login method failed, err msg: " << response.result().errmsg() << "\n";
    }

    return 0;
}