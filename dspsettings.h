#ifndef DSPSETTINGS_H
#define DSPSETTINGS_H

#include <QObject>

#include "xmlsettings.h"

namespace DSPSettings
{
    enum configstate
    {
        setDSPDevNode
    };
}


namespace Zera
{
namespace XMLConfig
{
    class cReader;
}
}


class cDSPSettings : public cXMLSettings
{
    Q_OBJECT

public:
    cDSPSettings(Zera::XMLConfig::cReader *xmlread);
    QString& getDeviceNode();

public slots:
    virtual void configXMLInfo(QString key);

private:
    QString m_sDeviceNode;
};



#endif // DSPSETTINGS_H
