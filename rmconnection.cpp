#include <syslog.h>
#include <QString>
#include <zeraclientnetbase.h>
#include <netmessages.pb.h>

#include "zdspglobal.h"
#include "rmconnection.h"


cRMConnection::cRMConnection(QString ipadr, quint16 port, quint8 dlevel)
    :m_sIPAdr(ipadr), m_nPort(port), m_nDebugLevel(dlevel)
{
}


void cRMConnection::connect2RM()
{
    m_pResourceManagerClient = new Zera::NetClient::cClientNetBase(this);
    connect(m_pResourceManagerClient, SIGNAL(tcpError(QAbstractSocket::SocketError)), this, SLOT(tcpErrorHandler(QAbstractSocket::SocketError)));
    connect(m_pResourceManagerClient, SIGNAL(connected()), this, SIGNAL(connected()));
    connect(m_pResourceManagerClient, SIGNAL(connectionLost()), this, SIGNAL(connectionRMError()));
    connect(m_pResourceManagerClient, SIGNAL(messageAvailable(QByteArray)), this, SLOT(responseHandler(QByteArray)));
    m_pResourceManagerClient->startNetwork(m_sIPAdr, m_nPort);
}


void cRMConnection::SendCommand(QString &cmd, QString &par)
{
    m_sCommand = cmd;
    ProtobufMessage::NetMessage envelope;
    ProtobufMessage::NetMessage::ScpiCommand* message = envelope.mutable_scpi();
    message->set_command(cmd.toStdString());
    message->set_parameter(par.toStdString());

    m_pResourceManagerClient->sendMessage(&envelope);
}


void cRMConnection::tcpErrorHandler(QAbstractSocket::SocketError errorCode)
{
    if (DEBUG1) syslog(LOG_ERR,"tcp socket error resource manager port: %d\n",errorCode);
    emit connectionRMError();
}


void cRMConnection::responseHandler(QByteArray message)
{
    ProtobufMessage::NetMessage answer;
    if (answer.ParseFromArray(message, message.count()))
    {
        if ( !(answer.has_reply() && answer.reply().rtype() == answer.reply().ACK))
        {
            if (DEBUG1)
            {
                QByteArray ba = m_sCommand.toLocal8Bit();
                syslog(LOG_ERR,"command: %s, was not acknowledged\n", ba.data() );
            }
            emit connectionRMError();
        }
    }
    else
    {
        if (DEBUG1) syslog(LOG_ERR,"answer from resource manager not protobuf \n");
        emit connectionRMError();
    }
}


void cRMConnection::SendIdent(QString ident)
{
    ProtobufMessage::NetMessage envelope;
    ProtobufMessage::NetMessage::NetReply* message = envelope.mutable_reply();
    message->set_rtype(ProtobufMessage::NetMessage::NetReply::IDENT);
    message->set_body(ident.toStdString());

    m_pResourceManagerClient->sendMessage(&envelope);
}

