## muduo库设计思想


### 主要的类

#### 1. TcpConnection

这个类包含一个channel，一个channel就是一条连接
库的使用者通过set方法注册的连接事件和消息处理事件方法被保存到TcpConnection类的成员变量中，
这些函数的成员变量在TcpConnection自己的handleRead，handleWrite被调用。

1. handleRead方法，直接从套接字读取数据到缓冲，然后将缓冲给到用户自定义的消息处理方法。

2. send方法
先尝试直接写，如果因为阻塞等原因导致不能全写，就将数据放入输出缓冲区，在后续poll上出发可写事件，执行handleWrite，继续向外写。
这里还有一个水位处理，当积压在输出缓冲区的数据太多时，把这个问题丢给库的使用者来处理，考虑可谓详尽。

#### 2. Buffer
readFd方法的设计，非常巧妙，在栈空间上开非常大的空间来暂时存放数据，确保数据不会丢失，等到读完之后，如果buff空间确实不够，就会调用append方法来扩容buffer,而后将栈上数据拷到buffer里面。


#### 3. EventLoopThreadPool
和使用pthread创建线程池类似，当使用c++11提供的std::thread创建线程时，注意条件变量和互斥量的使用。


#### 4. EPollPoller和Poller
这里是cpp多态的体现，通过newDefaultPoller得到具体实现的poll


##### weak_ptr的使用
参考modern effective C++ 


#### 使用

```c++
EventLoop loop;
InetAddress listenAddr(8888);  // 监听端口 8888
TcpServer server(&loop, listenAddr, "EchoServer");

server.setConnectionCallback(onConnection);
server.setMessageCallback(onMessage);

server.start();  // 启动服务器
loop.loop();  // 进入事件循环

baseloop中监听连接，有连接建立时，将channel分配到一个loop里面，loop中处理读写。