#include <QVariant>
#include <xmlconfigreader.h>

#include "dspsettings.h"


cDSPSettings::cDSPSettings(Zera::XMLConfig::cReader *xmlread)
{
    m_pXMLReader = xmlread;
    m_ConfigXMLMap["zdsp1dconfig:connectivity:dsp:device:node"] = DSPSettings::setDSPDevNode;
}


QString& cDSPSettings::getDeviceNode()
{
    return m_sDeviceNode;
}


void cDSPSettings::configXMLInfo(QString key)
{
    if (m_ConfigXMLMap.contains(key))
    {
        m_sDeviceNode = m_pXMLReader->getValue(key);
    }
}
