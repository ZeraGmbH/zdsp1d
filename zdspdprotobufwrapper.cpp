#include <netmessages.pb.h>

#include "zdspdprotobufwrapper.h"

cZDSPDProtobufWrapper::cZDSPDProtobufWrapper()
{
}


google::protobuf::Message *cZDSPDProtobufWrapper::byteArrayToProtobuf(QByteArray bA)
{
    ProtobufMessage::NetMessage *proto = new ProtobufMessage::NetMessage();
    if(!proto->ParseFromArray(bA, bA.size()))
    {
        ProtobufMessage::NetMessage::ScpiCommand *cmd = proto->mutable_scpi();
        cmd->set_command(bA.data(), bA.size() );
    }
    return proto;
}


QByteArray cZDSPDProtobufWrapper::protobufToByteArray(google::protobuf::Message *pMessage)
{
    ba = QByteArray(pMessage->SerializeAsString().c_str(), pMessage->ByteSize());
    return ba;
}

