#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <string>
#include <unordered_map>

#include "EventLoop.h"
#include "Callbacks.h"


class RpcProvider
{
public:
    void addService(google::protobuf::Service* srv);

    void run();

private:
    EventLoop m_eventloop;

    struct ServerInfo{
        google::protobuf::Service* m_service;
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap;
    };
    std::unordered_map<std::string, ServerInfo> m_serviceMap;

    void onConnection(const TcpConnectionPtr &);
    void onMessage(const TcpConnectionPtr &, Buffer *, Timestamp);

    void sendRpcResponse(const TcpConnectionPtr &,google::protobuf::Message*);

};  