#include <string>
#include <iostream>

#include "login.pb.h"
#include "rpcprovider.h"

class LoginService : public example::LoginServiceRpc
{
public:
    bool Login(std::string name,std::string pwd)
    {
        std::cout << "name: " << name << ", pwd: "<< pwd << "\n";

        return true;
    }

    void Login(::google::protobuf::RpcController* controller,
                       const ::example::LoginRequest* request,
                       ::example::LoginResponse* response,
                       ::google::protobuf::Closure* done) final
    {
        // 由框架做好网络字节流反序列化存放到request中，在rpcprovider.cc  onMessage line75进行反序列化处理
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 本地业务处理
        bool res = Login(name, pwd);

        // 响应
        example::ResultCode* result = response->mutable_result();
        result->set_errcode(0);
        result->set_errmsg("");

        response->set_success(res);

        // 执行回调，序列化&网络发送，框架来完成
        done->Run();

    }
};


int main()
{
    // 框架初始化，获取配置信息

    // 将LoginService发布到rpc结点
    RpcProvider rpc_service;
    rpc_service.addService(new LoginService());
    
    // 启动rpc服务
    rpc_service.run();
    return 0;
}