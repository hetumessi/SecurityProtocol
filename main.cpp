#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <openssl/aes.h>
#include <openssl/md5.h>
using namespace std;
//获取给定字符串的MD5散列值
void getMD5(string str, string& result)
{
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

