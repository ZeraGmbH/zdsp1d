#include <netmessages.pb.h>

#include "zdspdprotobufwrapper.h"

cZDSPDProtobufWrapper::cZDSPDProtobufWrapper()
{
}


std::shared_ptr<google::protobuf::Message> cZDSPDProtobufWrapper::byteArrayToProtobuf(QByteArray bA)
{
    ProtobufMessage::NetMessage *intermediate = new ProtobufMessage::NetMessage();
    if(!intermediate->ParseFromArray(bA, bA.size()))
    {
        ProtobufMessage::NetMessage::ScpiCommand *cmd = intermediate->mutable_scpi();
        cmd->set_command(bA.data(), bA.size() );
    }
    std::shared_ptr<google::protobuf::Message> proto {intermediate};
    return proto;
}
