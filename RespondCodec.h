#pragma once
#include "BaseASN1.h"
#include <string>

class SequenceASN1  : public BaseASN1
{
public:
    SequenceASN1();

    // 添加头结点
    int writeHeadNode(int iValue);
    int writeHeadNode(char* sValue, int len);
    // 添加后继结点
    int writeNextNode(int iValue);
    int writeNextNode(char* sValue, int len);

    // 读头结点数据
    int readHeadNode(int &iValue);
    int readHeadNode(char* sValue);
    // 读后继结点数据
    int readNextNode(int &iValue);
    int readNextNode(char* sValue);

    // 打包链表
    int packSequence(char** outData, int &outLen);
    // 解包链表
    int unpackSequence(char* inData, int inLen);

    // 释放链表
    void freeSequence(ITCAST_ANYBUF* node = NULL);

private:
    ITCAST_ANYBUF* m_header = NULL;
    ITCAST_ANYBUF* m_next   = NULL;
    ITCAST_ANYBUF* m_temp   = NULL;
};
struct  RespondMsg
{
    int	rv;		// 返回值
    char	clientId[12];	// 客户端编号
    char	serverId[12];	// 服务器编号
    char	r2[64];		// 服务器端随机数
    int	seckeyid;	// 对称密钥编号    keysn
    RespondMsg() {}
    RespondMsg(char* clientID, char* serverID, char* r2, int rv, int seckeyID)
    {
        this->rv = rv;
        this->seckeyid = seckeyid;
        strcpy(this->clientId, clientID);
        strcpy(this->serverId, serverID);
        strcpy(this->r2, r2);
    }
};

// 编解码的父类
class Codec : public SequenceASN1
{
public:
    Codec();
    virtual ~Codec();

    // 数据编码
    virtual int msgEncode(char** outData, int &len);
    // 数据解码
    virtual void* msgDecode(char *inData, int inLen);
};

class RespondCodec : public Codec
{
public:
    RespondCodec();
    RespondCodec(RespondMsg *msg);
    ~RespondCodec();

    // 函数重载
    int msgEncode(char** outData, int &len);
    void* msgDecode(char *inData, int inLen);

private:
    RespondMsg m_msg;
};

struct RequestMsg
{
    //1 密钥协商  	//2 密钥校验; 	// 3 密钥注销
    int		cmdType;		// 报文类型
    char	clientId[12];	// 客户端编号
    char	authCode[65];	// 认证码
    char	serverId[12];	// 服务器端编号
    char	r1[64];			// 客户端随机数
};

// 使用: 创建一个类对象
// 1. 编码 - 需要将外界数据插入当前对象中
// 2. 解码 - 需要待解码的字符串, 从中解析出一系列的数据
class RequestCodec : public Codec
{
public:
    enum CmdType{NewOrUpdate=1, Check, Revoke, View};
    // 解码时候使用的构造
    RequestCodec();
    // 编码时候使用的构造函数
    RequestCodec(RequestMsg* msg);
    ~RequestCodec();

    // 重写父类函数
    int msgEncode(char** outData, int &len);
    void* msgDecode(char *inData, int inLen);

private:
    RequestMsg m_msg;
};

class CodecFactory
{
public:
    CodecFactory();
    virtual ~CodecFactory();

    virtual Codec* createCodec();
};

class RespondFactory :
    public CodecFactory
{
public:
    RespondFactory();
    RespondFactory(RespondMsg *msg);
    ~RespondFactory();

    Codec* createCodec();

private:
    bool m_flag;
    RespondMsg * m_respond;
};

class RequestFactory :
    public CodecFactory
{
public:
    RequestFactory();
    RequestFactory(RequestMsg* msg);
    ~RequestFactory();

    Codec* createCodec();

private:
    bool m_flag = false;
    RequestMsg * m_request;
};



