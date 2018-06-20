#ifndef ZDSPDPROTOBUFWRAPPER_H
#define ZDSPDPROTOBUFWRAPPER_H

#include <xiqnetwrapper.h>

class cZDSPDProtobufWrapper : public XiQNetWrapper
{
public:
  cZDSPDProtobufWrapper();


  std::shared_ptr<google::protobuf::Message> byteArrayToProtobuf(QByteArray bA) override;

  QByteArray protobufToByteArray(const google::protobuf::Message &pMessage) override;
};

#endif // ZDSPDPROTOBUFWRAPPER_H
