


#### 集群和分布式
集群是将一个程序部署到多个机器上
分布式是将一个程序划分为多个部分，不同部分通过网络通信部署到不同机器上，即使某一个部分不能正常工作，整个服务仍然是正常的


生成proto .cc 代码在当前目录
```sh
protoc login.proto --cpp_out=./


# mymuduo 和 myrpc 需要使用相同的 C++ 标准

与grpc使用基本一样


可以通过zookeeper 提供的C API向zk服务器注册结点：/服务名/方法名  --> 内容为 IP:Port
caller使用rpc方法时候根据服务名和方法名得到IP和端口，向方法服务发起tcp连接