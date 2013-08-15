#ifndef DEBUGSETTINGS_H
#define DEBUGSETTINGS_H

#include <QObject>
#include "xmlsettings.h"

namespace DebugSettings
{
enum debugconfigstate
{
    setdebuglevel
};
}

namespace Zera
{
namespace XMLConfig
{
    class cReader;
}
}


class cDebugSettings: public cXMLSettings
{
    Q_OBJECT
public:
    cDebugSettings(Zera::XMLConfig::cReader *xmlread);
    quint8 getDebugLevel();

public slots:
    virtual void configXMLInfo(QString key);

private:
    quint8 m_nDebugLevel;
};


#endif // DEBUGSETTINGS_H
