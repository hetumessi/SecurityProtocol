#include "PoolSocket.h"

TcpSocket::TcpSocket()
{
}

TcpSocket::TcpSocket(int connfd)
{
    m_socket = connfd;
}

TcpSocket::~TcpSocket()
{
    printf("TcpSocket 被析构...\n");
}

int TcpSocket::connectToHost(char * ip, unsigned short port, int timeout)
{
    int ret = 0;
    if (ip == NULL || port <= 0 || port > 65535 || timeout < 0)
    {
        ret = ParamError;
        return ret;
    }

    m_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket < 0)
    {
        ret = errno;
        printf("func socket() err:  %d\n", ret);
        return ret;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    ret = connectTimeout((struct sockaddr_in*) (&servaddr), (unsigned int)timeout);
    if (ret < 0)
    {
        if (ret == -1 && errno == ETIMEDOUT)
        {
            ret = TimeoutError;
            return ret;
        }
        else
        {
            //printf("func connect_timeout() err:  %d\n", ret);
        }
    }

    return ret;
}

int TcpSocket::sendMsg(char * sendData, int dataLen, int timeout)
{
    int 	ret = 0;

    if (sendData == NULL || dataLen <= 0)
    {
        ret = ParamError;
        return ret;
    }

    ret = writeTimeout(timeout);
    if (ret == 0)
    {
        int writed = 0;
        unsigned char *netdata = (unsigned char *)malloc(dataLen + 4);
        if (netdata == NULL)
        {
            ret = MallocError;
            printf("func sckClient_send() mlloc Err:%d\n ", ret);
            return ret;
        }
        int netlen = htonl(dataLen);
        memcpy(netdata, &netlen, 4);
        memcpy(netdata + 4, sendData, dataLen);

        writed = writen(netdata, dataLen + 4);
        if (writed < (dataLen + 4))
        {
            if (netdata != NULL)
            {
                free(netdata);
                netdata = NULL;
            }
            return writed;
        }

        if (netdata != NULL)  //wangbaoming 20150630 modify bug
        {
            free(netdata);
            netdata = NULL;
        }
    }

    if (ret < 0)
    {
        //失败返回-1，超时返回-1并且errno = ETIMEDOUT
        if (ret == -1 && errno == ETIMEDOUT)
        {
            ret = TimeoutError;
            printf("func sckClient_send() mlloc Err:%d\n ", ret);
            return ret;
        }
        return ret;
    }

    return ret;
}

int TcpSocket::recvMsg(char ** recvData, int & recvLen, int timeout)
{
    int		ret = 0;

    if (recvData == NULL || recvLen == NULL)
    {
        ret = ParamError;
        printf("func sckClient_rev() timeout , err:%d \n", TimeoutError);
        return ret;
    }

    ret = readTimeout(timeout); //bugs modify bombing
    if (ret != 0)
    {
        if (ret == -1 || errno == ETIMEDOUT)
        {
            ret = TimeoutError;
            return ret;
        }
        else
        {
            return ret;
        }
    }

    int netdatalen = 0;
    ret = readn(&netdatalen, 4); //读包头 4个字节
    if (ret == -1)
    {
        //printf("func readn() err:%d \n", ret);
        return ret;
    }
    else if (ret < 4)
    {
        ret = PeerCloseError;
        //printf("func readn() err peer closed:%d \n", ret);
        return ret;
    }

    int n;
    n = ntohl(netdatalen);
    char* tmpBuf = (char *)malloc(n + 1);
    if (tmpBuf == NULL)
    {
        ret = MallocError;
        return ret;
    }


    ret = readn(tmpBuf, n); //根据长度读数据
    if (ret == -1)
    {
        //printf("func readn() err:%d \n", ret);
        return ret;
    }
    else if (ret < n)
    {
        ret = PeerCloseError;
        //printf("func readn() err peer closed:%d \n", ret);
        return ret;
    }

    *recvData = tmpBuf;
    recvLen = n;
    tmpBuf[n] = '\0'; //多分配一个字节内容，兼容可见字符串 字符串的真实长度仍然为n

    return 0;
}

void TcpSocket::disConnect()
{
    if (m_socket >= 0)
    {
        close(m_socket);
    }
}

void TcpSocket::freeMemory(char ** buf)
{
    if (*buf != NULL)
    {
        free(*buf);
        *buf = NULL;
    }
}

/////////////////////////////////////////////////
//////             子函数                   //////
/////////////////////////////////////////////////
/*
* blockIO - 设置I/O为非阻塞模式
* @fd: 文件描符符
*/
int TcpSocket::blockIO(int fd)
{
    int ret = 0;
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
    {
        ret = flags;
        return ret;
    }

    flags |= O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flags);
    if (ret == -1)
    {
        return ret;
    }
    return ret;
}

/*
* noBlockIO - 设置I/O为阻塞模式
* @fd: 文件描符符
*/
int TcpSocket::noBlockIO(int fd)
{
    int ret = 0;
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1)
    {
        ret = flags;
        return ret;
    }

    flags &= ~O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flags);
    if (ret == -1)
    {
        return ret;
    }
    return ret;
}

/*
* readTimeout - 读超时检测函数，不含读操作
* @wait_seconds: 等待超时秒数，如果为0表示不检测超时
* 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
*/
int TcpSocket::readTimeout(unsigned int wait_seconds)
{
    int ret = 0;
    if (wait_seconds > 0)
    {
        fd_set read_fdset;
        struct timeval timeout;

        FD_ZERO(&read_fdset);
        FD_SET(m_socket, &read_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        //select返回值三态
        //1 若timeout时间到（超时），没有检测到读事件 ret返回=0
        //2 若ret返回<0 &&  errno == EINTR 说明select的过程中被别的信号中断（可中断睡眠原理）
        //2-1 若返回-1，select出错
        //3 若ret返回值>0 表示有read事件发生，返回事件发生的个数

        do
        {
            ret = select(m_socket + 1, &read_fdset, NULL, NULL, &timeout);

        } while (ret < 0 && errno == EINTR);

        if (ret == 0)
        {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret == 1)
            ret = 0;
    }

    return ret;
}

/*
* writeTimeout - 写超时检测函数，不含写操作
* @wait_seconds: 等待超时秒数，如果为0表示不检测超时
* 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
*/
int TcpSocket::writeTimeout(unsigned int wait_seconds)
{
    int ret = 0;
    if (wait_seconds > 0)
    {
        fd_set write_fdset;
        struct timeval timeout;

        FD_ZERO(&write_fdset);
        FD_SET(m_socket, &write_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do
        {
            ret = select(m_socket + 1, NULL, &write_fdset, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret == 0)
        {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret == 1)
            ret = 0;
    }

    return ret;
}

/*
* connectTimeout - connect
* @addr: 要连接的对方地址
* @wait_seconds: 等待超时秒数，如果为0表示正常模式
* 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEDOUT
*/
int TcpSocket::connectTimeout(sockaddr_in *addr, unsigned int wait_seconds)
{
    int ret;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if (wait_seconds > 0)
        blockIO(m_socket);

    ret = connect(m_socket, (struct sockaddr*)addr, addrlen);
    if (ret < 0 && errno == EINPROGRESS)
    {
        //printf("11111111111111111111\n");
        fd_set connect_fdset;
        struct timeval timeout;
        FD_ZERO(&connect_fdset);
        FD_SET(m_socket, &connect_fdset);
        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do
        {
            // 一但连接建立，则套接字就可写  所以connect_fdset放在了写集合中
            ret = select(m_socket + 1, NULL, &connect_fdset, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);
        if (ret == 0)
        {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret < 0)
            return -1;
        else if (ret == 1)
        {
            //printf("22222222222222222\n");
            /* ret返回为1（表示套接字可写），可能有两种情况，一种是连接建立成功，一种是套接字产生错误，*/
            /* 此时错误信息不会保存至errno变量中，因此，需要调用getsockopt来获取。 */
            int err;
            socklen_t socklen = sizeof(err);
            int sockoptret = getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &err, &socklen);
            if (sockoptret == -1)
            {
                return -1;
            }
            if (err == 0)
            {
                //printf("3333333333333\n");
                ret = 0;
            }
            else
            {
                //printf("4444444444444444:%d\n", err);
                errno = err;
                ret = -1;
            }
        }
    }
    if (wait_seconds > 0)
    {
        noBlockIO(m_socket);
    }
    return ret;
}

/*
* readn - 读取固定字节数
* @fd: 文件描述符
* @buf: 接收缓冲区
* @count: 要读取的字节数
* 成功返回count，失败返回-1，读到EOF返回<count
*/
int TcpSocket::readn(void *buf, int count)
{
    size_t nleft = count;
    ssize_t nread;
    char *bufp = (char*)buf;

    while (nleft > 0)
    {
        if ((nread = read(m_socket, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        else if (nread == 0)
            return count - nleft;

        bufp += nread;
        nleft -= nread;
    }

    return count;
}

/*
* writen - 发送固定字节数
* @buf: 发送缓冲区
* @count: 要读取的字节数
* 成功返回count，失败返回-1
*/
int TcpSocket::writen(const void *buf, int count)
{
    size_t nleft = count;
    ssize_t nwritten;
    char *bufp = (char*)buf;

    while (nleft > 0)
    {
        if ((nwritten = write(m_socket, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
                continue;
            return -1;
        }
        else if (nwritten == 0)
            continue;

        bufp += nwritten;
        nleft -= nwritten;
    }

    return count;
}

// Socket连接池结构PoolHandle
struct PoolHandle
{
    queue<TcpSocket*> sockList;			// 存储可以通信的套接字对象
    int				bounds;				// Socket连接池的容量

    string 			serverip;
    unsigned short 	serverport;

    int 			connecttime;
    int				sTimeout; // 没有连接时，等待之间
    pthread_mutex_t foo_mutex;
};

PoolSocket::PoolSocket()
{
}

PoolSocket::~PoolSocket()
{
}

int PoolSocket::poolInit(PoolParam * param)
{
    int ret = 0;
    PoolHandle *hdl = new PoolHandle;
    m_handle = hdl;
    //初始化 句柄
    if (hdl == NULL)
    {
        ret = MallocErr;
        return ret;
    }

    // 数据初始化
    hdl->serverip = param->serverip;
    hdl->serverport = param->serverport;
    hdl->connecttime = param->connecttime;
    //处理连接数
    hdl->bounds = param->bounds;
    hdl->sTimeout = 100;

    pthread_mutex_init(&(hdl->foo_mutex), NULL);

    pthread_mutex_lock(&(hdl->foo_mutex)); //流程加锁
    // 创建用于通信的套接字对象
    connectServer();
    pthread_mutex_unlock(&(hdl->foo_mutex)); //解锁

    return ret;
}

TcpSocket* PoolSocket::getConnect()
{
    PoolHandle *hdl = static_cast<PoolHandle*>(m_handle);
    // 流程加锁 pthread_mutex_unlock(& (hdl->foo_mutex) ); //解锁
    pthread_mutex_lock(&(hdl->foo_mutex));

    // 若 有效连数 = 0
    if (hdl->sockList.size() == 0)
    {
        usleep(hdl->sTimeout); //等上几微妙
        // 还是没有可用的连接
        if (hdl->sockList.size() == 0)
        {
            return NULL;
        }
    }
    // 从对头取出一条连接, 并将该节点弹出
    TcpSocket* sock = hdl->sockList.front();
    hdl->sockList.pop();
    cout << "取出一条连接, 剩余连接数: " << curConnSize() << endl;

    pthread_mutex_unlock(&(hdl->foo_mutex)); //解锁

    return sock;
}

int PoolSocket::putConnect(TcpSocket* sock, bool isValid)
{
    int	ret = 0;
    PoolHandle *hdl = static_cast<PoolHandle*>(m_handle);
    pthread_mutex_lock(&(hdl->foo_mutex)); //流程加锁

    // 判断连接是否已经被 放进来
    // 判断该连接是否已经被释放
    if (isValid)
    {
        // 连接可用, 放入队列
        hdl->sockList.push(sock);
        cout << "放回一条连接, 剩余连接数: " << curConnSize() << endl;
    }
    else
    {
        // 套接字不可用, 析构对象, 在创建一个新的连接
        sock->disConnect();
        delete sock;
        connectServer(false);
        cout << "修复一条连接, 剩余连接数: " << curConnSize() << endl;
    }
    pthread_mutex_unlock(&(hdl->foo_mutex)); //解锁

    return ret;
}

void PoolSocket::poolDestory()
{
    PoolHandle *hdl = static_cast<PoolHandle*>(m_handle);
    // 遍历队列
    while (hdl->sockList.size() != 0)
    {
        // 取出对头元素
        TcpSocket* sock = hdl->sockList.front();
        // 弹出对头原始
        hdl->sockList.pop();
        // 释放内存
        delete sock;
    }
    delete hdl;
}

int PoolSocket::curConnSize()
{
    PoolHandle *hdl = static_cast<PoolHandle*>(m_handle);
    return hdl->sockList.size();
}

void PoolSocket::connectServer(bool recursion)
{
    PoolHandle *hdl = static_cast<PoolHandle*>(m_handle);
    if ((int)hdl->sockList.size() == hdl->bounds)
    {
        cout << "连接池对象初始化完毕, ^_^ ..." << endl;
        cout << "Poll Size: " << hdl->sockList.size() << endl;
        cout << "Poll bounds: " << hdl->bounds << endl;
        return;
    }
    TcpSocket* socket = new TcpSocket;
    char* ip = const_cast<char*>(hdl->serverip.data());
    int ret = socket->connectToHost(ip, hdl->serverport, hdl->connecttime);
    if (ret == 0)
    {
        // 成功连接服务器
        hdl->sockList.push(socket);
        cout << "Connect count: " << hdl->sockList.size() << endl;
    }
    else
    {
        // 失败
        cout << "连接服务器失败 - index: " << hdl->sockList.size()+1 << endl;
        // 释放对象
        delete socket;
    }
    if (recursion)
    {
        // 递归调用
        connectServer();
    }
}

TcpServer::TcpServer()
{
}


TcpServer::~TcpServer()
{
}

int TcpServer::setListen(unsigned short port)
{
    int 	ret = 0;
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);


    m_lfd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_lfd < 0)
    {
        ret = errno;
        m_log.Log(__FILE__, __LINE__, ItcastLog::ERROR, ret, "func socket() err");
        return ret;
    }


    int on = 1;
    ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret < 0)
    {
        ret = errno;
        m_log.Log(__FILE__, __LINE__, ItcastLog::ERROR, ret, "func setsockopt() err");
        return ret;
    }

    ret = ::bind(m_lfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (ret < 0)
    {
        ret = errno;
        m_log.Log(__FILE__, __LINE__, ItcastLog::ERROR, ret, "func bind() err");
        return ret;
    }

    ret = listen(m_lfd, SOMAXCONN);
    if (ret < 0)
    {
        ret = errno;
        m_log.Log(__FILE__, __LINE__, ItcastLog::ERROR, ret, "func listen() err");
        return ret;
    }

    return ret;
}

TcpSocket* TcpServer::acceptConn(int timeout)
{
    int connfd = acceptTimeout(timeout);
    if (connfd < 0)
    {
        if (connfd == -1 && errno == ETIMEDOUT)
        {
            //printf("func accept_timeout() timeout err:%d \n", ret);
            m_log.Log(__FILE__, __LINE__, ItcastLog::ERROR, connfd, "func acceptConn() TimeOutError");
        }
        else
        {
            m_log.Log(__FILE__, __LINE__, ItcastLog::ERROR, connfd, "func acceptConn() OtherError");
        }
        return NULL;
    }

    return new TcpSocket(connfd);
}

void TcpServer::closefd()
{
    close(m_lfd);
}

int TcpServer::acceptTimeout(int wait_seconds)
{
    int ret;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    if (wait_seconds > 0)
    {
        fd_set accept_fdset;
        struct timeval timeout;
        FD_ZERO(&accept_fdset);
        FD_SET(m_lfd, &accept_fdset);
        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do
        {
            ret = select(m_lfd + 1, &accept_fdset, NULL, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);
        if (ret == -1)
            return -1;
        else if (ret == 0)
        {
            errno = ETIMEDOUT;
            return -1;
        }
    }

    //一但检测出 有select事件发生，表示对等方完成了三次握手，客户端有新连接建立
    //此时再调用accept将不会堵塞
    ret = accept(m_lfd, (struct sockaddr*)&m_addrCli, &addrlen); //返回已连接套接字
    if (ret == -1)
    {
        ret = errno;
        m_log.Log(__FILE__, __LINE__, ItcastLog::ERROR, ret, "func accept() err");

        return ret;
    }
    return ret;
}

