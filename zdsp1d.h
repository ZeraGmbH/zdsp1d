// header datei zdsp1d.h
// globale definitionen


#ifndef ZDSP1D_H
#define ZDSP1D_H

#include <QObject>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <q3valuelist.h>
//Added by qt3to4:
#include <Q3PtrList>
//#include <Q3MemArray>
#include <QVector>

#include "dsp1scpi.h"
#include "zhserver.h"
#include "dsp.h"


typedef Q3ValueList<cDspCmd> tDspCmdList;
typedef Q3ValueList<cDspClientVar> tDspVarList;
typedef QVector<float> tDspMemArray;

class QSocketNotifier;
class QByteArray;
class QStateMachine;
class QState;
class cZDSP1Server; // forward
class cDebugSettings;
class cETHSettings;
class cDSPSettings;
class cRMConnection;

class cZDSP1Client
{
public:
    cZDSP1Client(){};
    cZDSP1Client(int socket, Zera::Net::cClient* netclient, cZDSP1Server *server);
    ~cZDSP1Client(){}; //  allokierten speicher ggf. freigeben
    
    
    QString& SetRavList(QString&);
    QString& GetRavList();
    QString& SetCmdListDef(QString& );
    QString& GetCmdListDef();
    QString& SetCmdIntListDef(QString&);
    QString& GetCmdIntListDef();
    void SetEncryption(int);
    int GetEncryption();
    bool GenCmdLists(QString&); // baut die cmdlisten  für den dsp zusammen wenn fehler -> false 
    cDspCmd GenDspCmd(QString&,bool*); // generiert ein dsp kommando aus einem string
    bool InitiateActValues(QString&); // liess die messergebnisse (liste)
    bool isActive(); 
    QString& FetchActValues(QString&); // gibt die messergebnisse aus (liste)
    void SetActive(bool); // merkt sich in m_bActive ob diese liste aktiv ist
    void SetStartAdr(ulong); // zum relokalisieren der userdaten
    QString& DspVarListRead(QString&); // lesen dsp daten ganze Liste
    bool DspVar(QString&,int&); // einen int (32bit) wert lesen
    bool DspVar(QString&,float&); // eine float wert lesen
    sDspVar* DspVarRead(QString&,QByteArray*); // lesen dsp variable;  name , länge stehen im parameter string; werte im anschluss im qbytearray
    const char* DspVarWriteRM(QString&); // dito schreiben mit rückmeldung
    bool DspVarWrite(QString&);  // schreiben  true wenn ok
    tDspCmdList& GetDspCmdList(); // damit der server die komplette liste aller clients
    tDspCmdList& GetDspIntCmdList(); // an den dsp übertragen kann
    tDspMemArray& GetDspMemData(); 
    cDspVarResolver DspVarResolver; // zum auflösen der variablen aus kommandos
    int sock; // socket für den die verbindung besteht
    Zera::Net::cClient* m_pNetClient; // our network client

private:
    void init(int socket, Zera::Net::cClient *netclient, cZDSP1Server* server);
    cZDSP1Server* myServer; 
    bool m_bActive;
    bool GenCmdList(QString&, tDspCmdList&,QString&);
    bool syntaxCheck(QString&);
          
    int Encryption;
    char* qSEncryption(char*,int );
    QString sOutput;
    QString m_sCmdListDef; // kommando liste defintion
    QString m_sIntCmdListDef; // interrupt kommando  liste defintion
        
//    ulong m_nStartAdr; // die absolute adresse an der ein variablen "block" im dsp steht 
    int m_nlen; // länge des gesamten datenblocks (in float bzw. long)
    tDspMemArray m_fDspMemData; // der datenblock bzw. kopie desselben
    tDspVarList m_DspVarList; // liste mit variablen beschreibungen
    tDspCmdList m_DspCmdList; // liste mit dsp kommandos (periodisch)
    tDspCmdList  m_DspIntCmdList; // liste mit dsp kommandos (interrupt)
    QVector<sDspVar> varArray; // array von sDspVar
    sMemSection msec; // eine memory section für den DspVarResolver 

};

class cZDSP1Server: public QObject, public cZHServer, public cbIFace {
    Q_OBJECT
public:
    cZDSP1Server();
    virtual ~cZDSP1Server();
    virtual void AddClient(Zera::Net::cClient *m_pNetClient); // fügt einen client hinzu
    virtual void DelClient(Zera::Net::cClient *m_pNetClient); // entfernt einen client

    virtual const char* SCPICmd( SCPICmdType, QChar*);
    virtual const char* SCPIQuery( SCPICmdType);
    
    static int gotSIGIO; // für signal handling
    void SetFASync(); // async. benachrichtung einschalten
    
    int SetBootPath(const char*);
    int SetDeviceNode(char*);
    int SetDebugLevel(const char*);
    
    int DspDevRead(int, char*, int); 
    int DspDevWrite(int, char*, int);
    int DspDevSeek(int,ulong);
    int DspDevOpen();
    
    int DevFileDescriptor; // kerneltreiber wird nur 1x geöffnet und dann gehalten
    int m_nDebugLevel;
    
    cDebugSettings* m_pDebugSettings;
    cETHSettings* m_pETHSettings;
    cDSPSettings* m_pDspSettings;

signals:
    void serverSetup();
    void sendAnswer(QByteArray);
    void abortInit();

private:
    Zera::Net::cServer* myServer; // the real server that does the communication job
    quint8 m_nerror;
    uchar ActivatedCmdList;
    Q3PtrList<cZDSP1Client> clientlist; // liste der clients
    
    // die routinen für das system modell
    const char* mCommand2Dsp(QString&); // indirekt für system modell    
    
    const char* mTestDsp(QChar *);
    const char* mResetDsp(QChar*);
    const char* mBootDsp(QChar*);
    const char* mSetDspBootPath(QChar*);
    const char* mGetDspBootPath();
    const char* mGetPCBSerialNumber();
    const char* mSetDspDebugLevel(QChar*);
    const char* mGetDebugLevel();
    const char* mGetDeviceVersion();
    const char* mGetServerVersion();  
    const char* mSetSamplingSystem(QChar*);
    const char* mGetSamplingSystem();
    const char* mSetCommEncryption(QChar*);
    const char* mGetCommEncryption();
    const char* mSetEN61850DestAdr(QChar*);
    const char* mGetEN61850DestAdr();
    const char* mSetEN61850SourceAdr(QChar*);
    const char* mGetEN61850SourceAdr();
    const char* mSetEN61850EthTypeAppId(QChar*);
    const char* mGetEN61850EthTypeAppId();
    const char* mSetEN61850PriorityTagged(QChar*);
    const char* mGetEN61850PriorityTagged();
    const char* mSetEN61850EthSync(QChar*);
    const char* mGetEN61850EthSync();
    const char* mSetDspCommandStat(QChar*);
    const char* mGetDspCommandStat();
    const char* mSetEN61850DataCount(QChar*);
    const char* mGetEN61850DataCount();
    const char* mSetEN61850SyncLostCount(QChar*);
    const char* mGetEN61850SyncLostCount();
    const char* mTriggerIntListHKSK(QChar*);
    const char* mTriggerIntListALL(QChar*);
    const char* mResetMaxima(QChar*);
        
    // die routinen für das status modell
    
    const char* mResetDeviceLoadMax();
    const char* mGetDeviceLoadMax();
    const char* mGetDeviceLoadAct();   
    const char* mGetDspStatus();
    const char* mGetDeviceStatus();
  
    // die routinen für das measure modell
    
    const char* mFetch(QChar *);
    const char* mInitiate(QChar*);
    const char* mUnloadCmdList(QChar*);
    const char* mLoadCmdList(QChar*);
    const char* mSetRavList(QChar*);
    const char* mGetRavList();
    const char* mSetCmdIntList(QChar*);
    const char* mGetCmdIntList();
    const char* mSetCmdList(QChar*);
    const char* mGetCmdList();
    const char* mMeasure(QChar*);
    
    // die routinen für das memory modell
    
    const char* mDspMemoryRead(QChar *);
    const char* mDspMemoryWrite(QChar *);
    
    bool LoadDSProgram();
    void setDspType();
    int readMagicId();
    bool Test4HWPresent(); 
    bool Test4DspRunning();
    cZDSP1Client* GetClient(int);
    
    QString m_sDspDeviceVersion; // version der hardware
    QString m_sDspSerialNumber; // seriennummer der hardware
    QString m_sDspDeviceNode; // für den zugriff zur hardware
    QString m_sDspBootPath;
    Zera::XMLConfig::cReader* myXMLConfigReader;

    QString Answer;

    QStateMachine* m_pInitializationMachine;
    QState* stateconnect2RM;
    QState* stateSendRMIdentandRegister;
    cRMConnection* m_pRMConnection;
    QSocketNotifier *m_pPipeInterrupt;

private slots:
    virtual void establishNewConnection(Zera::Net::cClient* newClient);
    virtual void deleteConnection();
    virtual void executeCommand(const QByteArray cmd);
    void DspIntHandler(int);
    void doConfiguration();
    void doSetupServer();
    void doCloseServer();
    void doConnect2RM();
    void doIdentAndRegister();
};

#endif
