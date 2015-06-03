#ifndef ZDSPDPROTOBUFWRAPPER_H
#define ZDSPDPROTOBUFWRAPPER_H

#include <protonetwrapper.h>

class cZDSPDProtobufWrapper : public ProtoNetWrapper
{
public:
  cZDSPDProtobufWrapper();


  google::protobuf::Message *byteArrayToProtobuf(QByteArray bA);

  QByteArray protobufToByteArray(google::protobuf::Message *pMessage);
private:
  QByteArray ba;
};

#endif // ZDSPDPROTOBUFWRAPPER_H
