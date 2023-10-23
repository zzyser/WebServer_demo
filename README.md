# WebServer_demo

##### 	功能：

- 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
- 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
- 利用标准库容器封装char，实现自动增长的缓冲区；
- 基于小根堆实现的定时器，关闭超时的非活动连接；
- 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；
- 利用RAII机制实现了数据库连接池，减少数据库连接建立与关闭的开销，同时实现了用户注册登录功能。

##### 项目环境：

Linux/Ubuntu 18

C++14

MySql



​		Webserver是一个B\S架构，浏览器为客户端，webserver为服务器。主要功能是通过HTTP写一与客户端（Browser）进行通信，来接收、存储、处理来自客户端的HTTP请求，并对其请求做出HTTP响应，这个响应是指发送响应报文（文件、网页等）或者是一个Error信息。

​		一个WebServer如何和用户进行通信，就需要在浏览器输入**域名或者IP地址:端口号**，浏览器会先将你的域名解析成响应的IP地址(DNS)或者直接根据你的IP:Port向对应的Web服务器发送一个HTTP请求。这个过程需要TCP协议的三次握手建立和目标Web服务器的连接，然后HTTP协议生成针对该Web服务器的HTTP请求报文，这个报文里面包括了请求行、请求头部、空行和请求数据四个部分（后面再细讲），通过TCP/IP协议发送到Web服务器上。

一个高并发的WebServer，肯定要涉及到I/O多路复用技术。

Web服务器端通过socket监听来自用户的请求。远端的很多用户会尝试去connect()这个Web Server上正在listen的这个port，而监听到的这些连接会排队等待被accept()。由于用户连接请求是随机到达的异步事件，每当监听socket（listenfd）listen到新的客户连接并且放入监听队列，我们都需要告诉我们的Web服务器有连接来了，accept这个连接，并分配一个逻辑单元来处理这个用户请求。而且，我们在处理这个请求的同时，还需要继续监听其他客户的请求并分配其另一逻辑单元来处理（并发，同时处理多个事件，后面会提到使用线程池实现并发）。

这里，服务器通过epoll这种I/O复用技术（还有select和poll）来实现对监听socket（listenfd）和连接socket（客户请求）的同时监听。注意I/O复用虽然可以同时监听多个文件描述符，但是它本身是阻塞的，并且当有多个文件描述符同时就绪的时候，如果不采取额外措施，程序则只能按顺序处理其中就绪的每一个文件描述符，所以为提高效率，我们将在这部分通过线程池来实现并发（多线程并发），为每个就绪的文件描述符分配一个逻辑单元（线程）来处理。

服务器程序通常需要处理三类事件：I/O事件，信号及定时事件。

有两种事件处理模式：

- Reactor模式：要求主线程（I/O处理单元）只负责监听文件描述符上是否有事件发生（可读、可写），若有，则立即通知工作线程（逻辑单元），将socket可读可写事件放入请求队列，交给工作线程处理。这个过程是同步的，读取完数据后应用进程才能处理数据。
- Proactor模式：将所有的I/O操作都交给主线程和内核来处理（进行读、写），工作线程仅负责处理逻辑，如主线程读完成后users[sockfd].read()，选择一个工作线程来处理客户请求pool->append(users + sockfd)。

理论上来说Proactor是更快的。由于Proactor模式需要异步I/O的一套接口，而在Linux环境下，没有异步IO接口，想要实现只有用同步I/O模拟实现Proactor，即主线程完成读写后通知工作线程。但意义不大，所以大部分都是采用`Reactor`。

​		一个完整的WebServer肯定得包括日志系统，无论是在代码调试还是服务器运行记录上，都是必不可少的一部分；同时还要实现数据库的连接与断开。服务器的能力是有限的，Socket的连接与断开、线程的不断创建与释放等这些都会造成一定的性能损失，这个时候就需要线程池与数据库连接池了。

​		还能进行进一步优化，用**定时器**对一些非活动连接关闭掉，如果某一用户connect()到服务器之后，长时间不交换数据，一直占用服务器端的文件描述符，导致连接资源的浪费。这时候就应该利用定时器把这些**超时的**非活动连接**释放**掉，关闭其占用的文件描述符。这种情况也很常见，当你登录一个网站后长时间没有操作该网站的网页，再次访问的时候你会发现需要重新登录。
