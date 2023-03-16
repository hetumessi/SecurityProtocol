#pragma once
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <queue>
#include <cstdarg>
using namespace std;

/************************************************************************/
/*
const char *file：文件名称
int line：文件行号
int level：错误级别
        0 -- 没有日志
        1 -- debug级别
        2 -- info级别
        3 -- warning级别
        4 -- err级别
int status：错误码
const char *fmt：可变参数
*/
/************************************************************************/
// 日志类
class ItcastLog
{
public:
    enum LogLevel{NOLOG, DEBUG, INFO, WARNING, ERROR};
    void Log(const char *file, int line, int level, int status, const char *fmt, ...);
    ItcastLog();
    ~ItcastLog();

private:
    int ITCAST_Error_GetCurTime(char* strTime);
    int ITCAST_Error_OpenFile(int* pf);
    void ITCAST_Error_Core(const char *file, int line, int level, int status, const char *fmt, va_list args);
};

/* 用于通信的套接字类 */
// 超时的时间
static const int TIMEOUT = 1000;
class TcpSocket
{
public:
    enum ErrorType {ParamError = 3001, TimeoutError, PeerCloseError, MallocError};
    TcpSocket();
    // 使用一个可以用于通信的套接字实例化套接字对象
    TcpSocket(int connfd);
    ~TcpSocket();
    // 连接服务器
    int connectToHost(char* ip, unsigned short port, int timeout = TIMEOUT);
    // 发送数据
    int sendMsg(char* sendData, int dataLen, int timeout = TIMEOUT);
    // 接收数据
    int recvMsg(char** recvData, int &recvLen, int timeout = TIMEOUT);
    // 断开连接
    void disConnect();
    // 释放内存
    void freeMemory(char** buf);

private:
    // 设置I/O为非阻塞模式
    int blockIO(int fd);
    // 设置I/O为阻塞模式
    int noBlockIO(int fd);
    // 读超时检测函数，不含读操作
    int readTimeout(unsigned int wait_seconds);
    // 写超时检测函数, 不包含写操作
    int writeTimeout(unsigned int wait_seconds);
    // 带连接超时的connect函数
    int connectTimeout(struct sockaddr_in *addr, unsigned int wait_seconds);
    // 每次从缓冲区中读取n个字符
    int readn(void *buf, int count);
    // 每次往缓冲区写入n个字符
    int writen(const void *buf, int count);

private:
    int m_socket;		// 用于通信的套接字
};

// 初始化连接池的结构体
struct PoolParam
{
    int 	bounds; //池容量
    int 	connecttime;
    int 	sendtime;
    int 	revtime;
    string 	serverip;
    unsigned short 	serverport;
};

class PoolSocket
{
public:
    enum ErrorType {
        ParamErr = 3000 + 1,
        TimeOut,
        PeerClose,
        MallocErr,
        CreateConnErr,	// 创建连接池 （没有达到最大连接数）
        terminated,		// 已终止
        ValidIsZero,	// 有效连接数是零
        HaveExist,		// 连接已经在池中
        ValidBounds		// 有效连接数目超过了最大连接数
    };
    PoolSocket();
    ~PoolSocket();
    int poolInit(PoolParam *param);
    // 从连接池中获取一条连接
    TcpSocket* getConnect();
    // 将连接放回到连接池
    int putConnect(TcpSocket* sock, bool isValid);
    // 释放连接池资源
    void poolDestory();
    int curConnSize();

private:
    void connectServer(bool recursion = true);

private:
    void* m_handle;
};

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();

    // 服务器设置监听
    int setListen(unsigned short port);
    // 等待并接受客户端连接请求, 默认连接超时时间为10000s
    TcpSocket* acceptConn(int timeout = 10000);
    void closefd();

private:
    int acceptTimeout(int wait_seconds);

private:
    int m_lfd;	// 用于监听的文件描述符
    struct sockaddr_in m_addrCli;
    ItcastLog m_log;
};




