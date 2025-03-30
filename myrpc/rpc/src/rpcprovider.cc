#include "rpcprovider.h"

#include "TcpServer.h"
#include "InetAddress.h"
#include "rpcheader.pb.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <google/protobuf/stubs/callback.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <utility>

void RpcProvider::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected()){
        conn->shutdown();
    }
}

/**
读取caller发送来的序列化请求数据，进行解析
调用本地发布出去的rpc方法
*/
void RpcProvider::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp)
{
    auto recv_str = buffer->retrieveAllAsString();

    uint32_t header_size;
    recv_str.copy(reinterpret_cast<char*>(&header_size), sizeof(header_size));

    std::string rpc_header_str = recv_str.substr(sizeof(header_size),header_size);
    
    std::string service_name;
    std::string method_name;
    uint32_t args_size;

    myrpc::RpcHeader rpc_header;
    if(rpc_header.ParseFromString(rpc_header_str)){
        service_name = rpc_header.service_name();
        method_name = rpc_header.method_name();
        args_size = rpc_header.args_size();
    }else{
        std::cout << "rpc_header ParseFromString failed\n";
        return;
    }

    std::string rpc_args_str = recv_str.substr(sizeof(header_size) + header_size,args_size);

    //打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl; 
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl; 
    std::cout << "service_name: " << service_name << std::endl; 
    std::cout << "method_name: " << method_name << std::endl; 
    std::cout << "args_str: " << rpc_args_str << std::endl; 
    std::cout << "============================================" << std::endl;


    std::unordered_map<std::string, ServerInfo>::const_iterator srv_iter;
    if((srv_iter = m_serviceMap.find(service_name)) == m_serviceMap.end()){
        std::cout << service_name << " is not exist\n";
        return;
    }

    std::unordered_map<std::string, const google::protobuf::MethodDescriptor*>::const_iterator method_iter;
    if((method_iter = srv_iter->second.m_methodMap.find(method_name)) == srv_iter->second.m_methodMap.end()){
        std::cout << method_name << " is not exist\n";
        return;
    }

    // 获取service对象
    auto rpc_service = srv_iter->second.m_service;
    // 获取method对象
    const google::protobuf::MethodDescriptor* rpc_method = method_iter->second;

    // 生成调用服务时传递的已经反序列完成的request
    // request & response 的基类都是Message
    google::protobuf::Message* request = rpc_service->GetRequestPrototype(rpc_method).New();
    if(!request->ParseFromString(rpc_args_str)){
        std::cout << "request ParseFromString failed\n";
        return;
    }

    google::protobuf::Message* response = rpc_service->GetResponsePrototype(rpc_method).New();

    // 生成一个回调对象
    google::protobuf::Closure* done = google::protobuf::NewCallback<RpcProvider,
                                                const TcpConnectionPtr &,
                                                google::protobuf::Message*>
                                                (this,
                                                &RpcProvider::sendRpcResponse,
                                                conn,response);
    // 服务端调用本地方法
    rpc_service->CallMethod(rpc_method, nullptr, request, response, done);

    delete done;
}

void RpcProvider::sendRpcResponse(const TcpConnectionPtr &conn,google::protobuf::Message* response)
{
    std::string response_str;
    
    if(response->SerializeToString(&response_str)){
        conn->send(response_str);
    }
    conn->shutdown();// 类似http的短链接服务，服务器主动断开
}


void RpcProvider::addService(google::protobuf::Service* srv)
{
    const google::protobuf::ServiceDescriptor *p_srv_desc = srv->GetDescriptor();

    std::string srv_name = p_srv_desc->name();
    int method_cnt = p_srv_desc->method_count();

    ServerInfo service_info;
    service_info.m_service = srv;

    for(int i = 0; i < method_cnt; i++){
        const google::protobuf::MethodDescriptor *p_method_desc = p_srv_desc->method(i);
        std::string method_name = p_method_desc->name();
        service_info.m_methodMap.emplace(method_name,p_method_desc);
    }
    m_serviceMap.emplace(srv_name,service_info);
}

void RpcProvider::run()
{
    uint16_t port = 9611;
    std::string host = "127.0.0.1";

    InetAddress addr(port,host);
    TcpServer server(&m_eventloop,addr,"rpcProvider");

    server.setConnectionCallback(std::bind(&RpcProvider::onConnection, this,std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::onMessage, this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));

    server.setThreadNums(3);
    server.start();
    m_eventloop.loop();
}