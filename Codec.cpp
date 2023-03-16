#include "Codec.h"

SequenceASN1::SequenceASN1()
{
}

int SequenceASN1::writeHeadNode(int iValue)
{
    DER_ItAsn1_WriteInteger(iValue, &m_header);
    m_next = m_header;

    return 0;
}

int SequenceASN1::writeHeadNode(char * sValue, int len)
{
    EncodeChar(sValue, len, &m_header);
    m_next = m_header;
    return 0;
}

int SequenceASN1::writeNextNode(int iValue)
{
    DER_ItAsn1_WriteInteger(iValue, &m_next->next);
    m_next = m_next->next;
    return 0;
}

int SequenceASN1::writeNextNode(char * sValue, int len)
{
    EncodeChar(sValue, len, &m_next->next);
    m_next = m_next->next;
    return 0;
}

int SequenceASN1::readHeadNode(int & iValue)
{
    DER_ItAsn1_ReadInteger(m_header, (ITCAST_UINT32 *)&iValue);
    m_next = m_header->next;
    return 0;
}

int SequenceASN1::readHeadNode(char * sValue)
{
    DER_ItAsn1_ReadPrintableString(m_header, &m_temp);
    memcpy(sValue, m_temp->pData, m_temp->dataLen);
    DER_ITCAST_FreeQueue(m_temp);
    m_next = m_header->next;
    return 0;
}

int SequenceASN1::readNextNode(int & iValue)
{
    DER_ItAsn1_ReadInteger(m_next, (ITCAST_UINT32 *)&iValue);
    m_next = m_next->next;
    return 0;
}

int SequenceASN1::readNextNode(char * sValue)
{
    DER_ItAsn1_ReadPrintableString(m_next, &m_temp);
    memcpy(sValue, m_temp->pData, m_temp->dataLen);
    DER_ITCAST_FreeQueue(m_temp);
    m_next = m_next->next;
    return 0;
}

int SequenceASN1::packSequence(char ** outData, int & outLen)
{
    DER_ItAsn1_WriteSequence(m_header, &m_temp);
    *outData = (char *)m_temp->pData;
    outLen = m_temp->dataLen;
    DER_ITCAST_FreeQueue(m_header);
    return 0;
}

int SequenceASN1::unpackSequence(char * inData, int inLen)
{
    DER_ITCAST_String_To_AnyBuf(&m_temp, (unsigned char *)inData, inLen);
    DER_ItAsn1_ReadSequence(m_temp, &m_header);
    DER_ITCAST_FreeQueue(m_temp);
    return 0;
}

void SequenceASN1::freeSequence(ITCAST_ANYBUF * node)
{

}

Codec::Codec()
{
}

Codec::~Codec()
{
}

int Codec::msgEncode(char ** outData, int & len)
{
    return 0;
}

void * Codec::msgDecode(char * inData, int inLen)
{
    return NULL;
}

RespondCodec::RespondCodec() : Codec()
{
}

RespondCodec::RespondCodec(RespondMsg *msg) : Codec()
{
    m_msg.rv = msg->rv;
    m_msg.seckeyid = msg->seckeyid;
    strcpy(m_msg.clientId, msg->clientId);
    strcpy(m_msg.serverId, msg->serverId);
    strcpy(m_msg.r2, msg->r2);
}

RespondCodec::~RespondCodec()
{
    cout << "RespondCodec destruct ..." << endl;
}

int RespondCodec::msgEncode(char** outData, int &len)
{
    writeHeadNode(m_msg.rv);
    writeNextNode(m_msg.clientId, strlen(m_msg.clientId) + 1);
    writeNextNode(m_msg.serverId, strlen(m_msg.serverId) + 1);
    writeNextNode(m_msg.r2, strlen(m_msg.r2) + 1);
    writeNextNode(m_msg.seckeyid);
    packSequence(outData, len);

    return 0;
}

void* RespondCodec::msgDecode(char * inData, int inLen)
{
    unpackSequence((char*)inData, inLen);
    readHeadNode(m_msg.rv);
    readNextNode(m_msg.clientId);
    readNextNode(m_msg.serverId);
    readNextNode(m_msg.r2);
    readNextNode(m_msg.seckeyid);

    return &m_msg;
}

RequestCodec::RequestCodec() : Codec()
{
}

RequestCodec::RequestCodec(RequestMsg * msg)
{
    memcpy(&m_msg, msg, sizeof(RequestMsg));
}

RequestCodec::~RequestCodec()
{
}

int RequestCodec::msgEncode(char ** outData, int & len)
{
    writeHeadNode(m_msg.cmdType);
    writeNextNode(m_msg.clientId, strlen(m_msg.clientId)+1);
    writeNextNode(m_msg.authCode, strlen(m_msg.authCode) + 1);
    writeNextNode(m_msg.serverId, strlen(m_msg.serverId) + 1);
    writeNextNode(m_msg.r1, strlen(m_msg.r1) + 1);
    packSequence(outData, len);

    return 0;
}

void * RequestCodec::msgDecode(char * inData, int inLen)
{
    unpackSequence(inData, inLen);
    readHeadNode(m_msg.cmdType);
    readNextNode(m_msg.clientId);
    readNextNode(m_msg.authCode);
    readNextNode(m_msg.serverId);
    readNextNode(m_msg.r1);
    return &m_msg;
}

CodecFactory::CodecFactory()
{
}

CodecFactory::~CodecFactory()
{
}

Codec * CodecFactory::createCodec()
{
    return NULL;
}

RespondFactory::RespondFactory()
{
    m_flag = false;
}

RespondFactory::RespondFactory(RespondMsg *msg)
{
    m_flag = true;
    m_respond = msg;
}


RespondFactory::~RespondFactory()
{
}

Codec * RespondFactory::createCodec()
{
    Codec* codec = NULL;
    if (m_flag)
    {
        codec = new RespondCodec(m_respond);
    }
    else
    {
        codec = new RespondCodec();
    }
    return codec;
}


RequestFactory::RequestFactory()
{
    m_flag = false;
}

RequestFactory::RequestFactory(RequestMsg * msg)
{
    m_request = msg;
    m_flag = true;
}

RequestFactory::~RequestFactory()
{
}

Codec * RequestFactory::createCodec()
{
    if (m_flag == true){
        return new RequestCodec(m_request);
    }else{
        return new RequestCodec();
    }
}


