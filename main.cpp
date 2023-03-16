#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <openssl/aes.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include "itcast_asn1_der.h"
#include "PoolSocket.h"
using namespace std;
//teacher结构体，测试编解码模块
typedef struct _Teacher
{
    char name[64];
    int age;
    char *p;
    long plen;
}Teacher;
int encodeTeacher(Teacher * p, char ** outData, int * outlen)
{
    ITCAST_ANYBUF *head = NULL;
    ITCAST_ANYBUF *temp = NULL;
    ITCAST_ANYBUF *next = NULL;
    DER_ITCAST_String_To_AnyBuf(&temp, reinterpret_cast<unsigned char*>(p->name), strlen(p->name)+1);
    DER_ItAsn1_WritePrintableString(temp, &head);
    DER_ITCAST_FreeQueue(temp);
    next = head;
    DER_ItAsn1_WriteInteger(p->age, &next->next);
    next = next->next;
    EncodeChar(p->p, strlen(p->p)+1, &next->next);
    next = next->next;
    DER_ItAsn1_WriteInteger(p->plen, &next->next);
    DER_ItAsn1_WriteSequence(head, &temp);
    *outData = reinterpret_cast<char*>(temp->pData);
    *outlen = temp->dataLen;
    DER_ITCAST_FreeQueue(head);
    return 0;
}
int decodeTeacher(char * inData, int inLen, Teacher ** p)
{
    ITCAST_ANYBUF *head = NULL;
    ITCAST_ANYBUF *temp = NULL;
    ITCAST_ANYBUF *next = NULL;
    Teacher *pt = (Teacher *)malloc(sizeof(Teacher));
    if (pt == NULL){
        return -1;
    }
    DER_ITCAST_String_To_AnyBuf(&temp, reinterpret_cast<unsigned char*>(inData), inLen);
    DER_ItAsn1_ReadSequence(temp, &head);
    DER_ITCAST_FreeQueue(temp);
    next = head;
    DER_ItAsn1_ReadPrintableString(next, &temp);
    memcpy(pt->name, temp->pData, temp->dataLen);
    next = next->next;
    DER_ITCAST_FreeQueue(temp);
    DER_ItAsn1_ReadInteger(next, reinterpret_cast<unsigned long*>(&pt->age));
    next = next->next;
    int len = 0;
    DecodeChar(next, &pt->p, &len);
    next = next->next;
    DER_ItAsn1_ReadInteger(next, reinterpret_cast<unsigned long*>(&pt->plen));
    *p = pt;
    DER_ITCAST_FreeQueue(head);
    return 0;
}

void freeTeacher(Teacher ** p)
{
    if ((*p) != NULL){
        if ((*p)->p != NULL){
            free((*p)->p);
        }
        free(*p);
    }
}

//获取给定字符串的MD5散列值
void getMD5(string str, string& result)
{
    if(!str.size())return;
    MD5_CTX ctx;    //声明一个 MD5 上下文结构体 MD5_CTX，以便在后续计算中存储中间结果。
    MD5_Init(&ctx);  //初始化 MD5 上下文结构体
    MD5_Update(&ctx, str.c_str(), str.size()); //将要计算的字符串作为输入，更新 MD5 上下文结构体
    vector<unsigned char>md(16,0);  //存储计算出的 MD5 散列值
    MD5_Final(md.data(), &ctx);         //将 MD5 上下文结构体中的最终结果存储到 md 数组中
    //将 md 数组中的 16 个 unsigned char 类型的值转换为长度为 32 的十六进制字符串，存储到 result
    for (int i = 0; i < 16; ++i){
        sprintf(&result[i * 2], "%02x", md[i]);
    }
}

int main()
{
    PoolSocket pool;
    PoolParam param;
    param.bounds = 10;
    param.connecttime = 100;
    param.revtime = 100;
    param.sendtime = 100;
    param.serverip = "127.0.0.1";
    param.serverport = 9999;
    pool.poolInit(&param);
    queue<TcpSocket*> list;
    while (pool.curConnSize())
    {
        static int i = 0;
        TcpSocket* sock = pool.getConnect();
        string str = "hello, server ... " + to_string(i++);
        sock->sendMsg((char*)str.c_str(), str.size());
        list.push(sock);
    }
    while (!list.empty())
    {
        TcpSocket* t = list.front();
        pool.putConnect(t, false);
        list.pop();
    }
    cout << "max value: " << pool.curConnSize() << endl;
    while (1);

    char data[1024] = "xiaowu,hello world";
    int len = strlen(data);
    unsigned char md[SHA512_DIGEST_LENGTH] = {0};
    //int SHA512_Init(SHA512_CTX *c);
    //int SHA512_Update(SHA512_CTX *c, const void *data, size_t len);
    //int SHA512_Final(unsigned char *md, SHA512_CTX *c);
    SHA512_CTX c;
    SHA512_Init(&c);
    SHA512_Update(&c, data, len);
    SHA512_Final(md, &c);
    //cout << md << endl;
    char buf[SHA512_DIGEST_LENGTH * 2 + 1] = { 0 };
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
    {
        sprintf(&buf[i*2], "%02x", md[i]);
    }
    cout << buf << endl;
    //unsigned char *SHA512(const unsigned char *d, size_t n, unsigned char *md);
    memset(md, 0x00, sizeof(md));
    unsigned char data1[1024]= "xiaowu,hello world";
    SHA512(data1, len, md);
    memset(buf, 0x00, sizeof(buf));
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
    {
        sprintf(&buf[i * 2], "%02x", md[i]);
    }
    cout << buf << endl;
    Teacher tea;
    memset(&tea, 0x00, sizeof(Teacher));
    strcpy(tea.name, "abc");
    tea.age = 20;
    tea.p = (char*)malloc(100);
    strcpy(tea.p, "hello hello hello");
    tea.plen = strlen(tea.p);
    char* outData;
    int outlen;
    encodeTeacher(&tea, &outData, &outlen);
    //===============================================
    Teacher* pt;
    decodeTeacher(outData, outlen, &pt);
    printf("name:	%s\n", pt->name);
    printf("age:	%d\n", pt->age);
    printf("p:	%s\n", pt->p);
    printf("plen:	%d\n", pt->plen);
    freeTeacher(&pt);
    string result(33,0);
    getMD5("hello, md5", result);
    printf("md5 value: %s\n", result.c_str());
    // aes数据加密
    string mykey = "0123456789abcdef";
    vector<unsigned char>mykey_vec(mykey.begin(),mykey.end());
    AES_KEY aes;
    // void AES_set_encrypt_key(const unsigned char *userKey, const int bits, AES_KEY *key);
    AES_set_encrypt_key(mykey_vec.data(), 128, &aes);
    // 需要加密的字符串
    int length;
    string mystr = "hello world, how are you, i am fine, thank you";
    vector<unsigned char>mystr_vec(mystr.begin(),mystr.end());
    // 计算第三个参数length的长度, 包含了字符串末尾的\0
    if (mystr.size() % 16 == 0){
        // 长度刚好合适
        length = mystr.size();
    }else{
        length = (mystr.size()/16+1)*16;
    }
    unsigned char *encrypt = (unsigned char*)calloc(length, 1);
    vector<unsigned char>iv(16,'a');
    for (int i = 0; i < 16; i++){
        printf("iv[%d]==[%c]\n", i, iv[i]);
    }
    AES_cbc_encrypt(mystr_vec.data(), encrypt, length, &aes, iv.data(), AES_ENCRYPT);
    for (int i = 0; i < 16; i++){
        printf("iv[%d]==[%c]\n", i, iv[i]);
    }
    // aes数据解密
    // 1. 秘钥
    AES_set_decrypt_key(mykey_vec.data(), 128, &aes);
    memset(&iv[0], 'a', sizeof(iv));
    // 2. 解密
    unsigned char *decrypt = (unsigned char*)calloc(length, 1);
    AES_cbc_encrypt(encrypt, decrypt, length, &aes, iv.data(), AES_DECRYPT);
    printf("解密之后的数据: %s\n", decrypt);
    getchar();
    return 0;
}

