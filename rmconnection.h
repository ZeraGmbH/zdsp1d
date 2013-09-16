#ifndef RMCONNECTION_H
#define RMCONNECTION_H

#include <QObject>
#include <QAbstractSocket>

class QString;

namespace Zera {
namespace NetClient {
class  cClientNetBase;
}

}


class cRMConnection: public QObject
{
    Q_OBJECT

public:
    cRMConnection(QString ipadr, quint16 port, quint8 dlevel);
    void connect2RM();
    void SendIdent(QString ident);

public slots:
    void SendCommand(QString& cmd, QString &par);

private:
    QString m_sIPAdr;
    quint16 m_nPort;
    quint8 m_nDebugLevel;
    QString m_sCommand;
    Zera::NetClient::cClientNetBase* m_pResourceManagerClient;

private slots:
    void tcpErrorHandler(QAbstractSocket::SocketError errorCode);
    void responseHandler(QByteArray message);

signals:
    void connectionRMError();
    void connected();
};


#endif // RMCONNECTION_H
