#include "rpcchannel.h"
#include "rpcheader.pb.h"
#include <arpa/inet.h>
#include <cstdint>
#include <cstdlib>
#include <netinet/in.h>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>


/*
 caller发出的所有rpc调用请求全部转到这里，经过序列化打包发送到服务方
 
 +--------------------------------------------------------------+
 |  头长(固定4字节) |  服务名  |  方法名  |    参数长度   | 参数  |
 +--------------------------------------------------------------+

*/

void MyRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller,
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* done)
{
    const google::protobuf::ServiceDescriptor *p_srv_desc = method->service();

    std::string service_name = p_srv_desc->name();
    std::string method_name = method->name();

    std::string args_str;// 序列化之后的参数
    uint32_t args_size;// 序列化之后参数的长度

    if(request->SerializeToString(&args_str)){
        args_size = args_str.size();
    }else{
        std::cout << "args SerializeToString failed\n";
        return;
    }

    myrpc::RpcHeader rpc_req_header;
    rpc_req_header.set_service_name(service_name);
    rpc_req_header.set_method_name(method_name);
    rpc_req_header.set_args_size(args_size);


    uint32_t header_size;
    std::string rpc_header_str;
    if(rpc_req_header.SerializeToString(&rpc_header_str)){
        header_size = rpc_header_str.size();
    }else{
        std::cout << "rpc_req_header SerializeToString failed\n";
        return;
    }

    // 最后组织要发送的数据
    std::string rpc_send_str;
    rpc_send_str += std::string(reinterpret_cast<char*>(&header_size),sizeof(uint32_t));
    rpc_send_str += rpc_header_str;
    rpc_send_str += args_str;


    //打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl; 
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl; 
    std::cout << "service_name: " << service_name << std::endl; 
    std::cout << "method_name: " << method_name << std::endl; 
    std::cout << "args_str: " << args_str << std::endl; 
    std::cout << "============================================" << std::endl;


    int clientfd;
    if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std::cout << "create socket fd err!\n";
        exit(EXIT_FAILURE);
    }

    sockaddr_in service_addr;
    service_addr.sin_family = AF_INET;
    service_addr.sin_port = htons(9611);
    service_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(-1 == connect(clientfd, reinterpret_cast<struct sockaddr*>(&service_addr), sizeof(service_addr))){
        close(clientfd);

        std::cout << "connect failed\n";
        return;
    }

    if(-1 == send(clientfd, rpc_send_str.c_str(), rpc_send_str.size(), 0)){
        close(clientfd);

        std::cout << "send failed\n";
        return;
    }

    char recv_buf[1024];
    int recv_size;
    if(-1 == (recv_size = recv(clientfd, recv_buf, sizeof(recv_buf), 0))){
        close(clientfd);

        std::cout << "recv failed\n";
        return;
    }

    if(!response->ParseFromArray(recv_buf, recv_size)){
        close(clientfd);
        std::cout << "ParseFromArray failed\n";
        return;
    }

    close(clientfd);
}