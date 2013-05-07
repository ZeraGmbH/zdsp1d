// header datei zdsp1d.h
// globale definitionen


#ifndef ZDSP1D_H
#define ZDSP1D_H

#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include <qmemarray.h>

#include "dsp1scpi.h"
#include "zhserver.h"
#include "dsp.h"

// diese werte sind fix
#define MaxClients 10 

#define MaxDebugLevel 7
#define DEBUG1 (DebugLevel & 1) // alle fehlermeldungen loggen
#define DEBUG2 (DebugLevel & 2) // alle devnode aktivitäten loggen
#define DEBUG3 (DebugLevel & 4) // alle client an-,abmeldungen

typedef QValueList<cDspCmd> tDspCmdList;
typedef QValueList<cDspClientVar> tDspVarList;
typedef QMemArray<float> tDspMemArray;

class cZDSP1Server; // forward

class cZDSP1Client: public cZHClient {
public:
    cZDSP1Client(){};
    cZDSP1Client(int socket,cZDSP1Server *server);
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
    
private:
    void init(cZDSP1Server* server);
    cZDSP1Server* myServer; 
    bool m_bActive;
    cZDSP1Client* GetClient(int);
    bool GenCmdList(QString&, tDspCmdList&,QString&);
    bool syntaxCheck(QString&);
          
    int Encryption;
    char* qSEncryption(char*,int );
    QString m_sCmdListDef; // kommando liste defintion
    QString m_sIntCmdListDef; // interrupt kommando  liste defintion
        
//    ulong m_nStartAdr; // die absolute adresse an der ein variablen "block" im dsp steht 
    int m_nlen; // länge des gesamten datenblocks (in float bzw. long)
    tDspMemArray m_fDspMemData; // der datenblock bzw. kopie desselben
    tDspVarList m_DspVarList; // liste mit variablen beschreibungen
    tDspCmdList m_DspCmdList; // liste mit dsp kommandos (periodisch)
    tDspCmdList  m_DspIntCmdList; // liste mit dsp kommandos (interrupt)
    QMemArray<sDspVar> varArray; // array von sDspVar
    sMemSection msec; // eine memory section für den DspVarResolver 
};

class cZDSP1Server: public cZHServer, public cbIFace {
public:
    cZDSP1Server();
    virtual ~cZDSP1Server();
    virtual int Execute(); // server ausführen überladen v. cZHServer
    virtual void AddClient(int socket); // fügt einen client hinzu
    virtual void DelClient(int socket); // entfernt einen client
        
    virtual const char* SCPICmd( SCPICmdType, char*);
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
    int DebugLevel;
    
private:
    uchar ActivatedCmdList;
    QPtrList<cZDSP1Client> clientlist; // liste der clients
    
    // die routinen für das system modell
    const char* mCommand2Dsp(QString&); // indirekt für system modell    
    
    const char* mTestDsp(char*);
    const char* mResetDsp(char*);
    const char* mBootDsp(char*);
    const char* mSetDspBootPath(char*);
    const char* mGetDspBootPath();
    const char* mGetPCBSerialNumber();
    const char* mGetDspDeviceNode();   
    const char* mSetDspDebugLevel(char*);
    const char* mGetDebugLevel();
    const char* mGetDeviceVersion();
    const char* mGetServerVersion();  
    const char* mSetSamplingSystem(char*);
    const char* mGetSamplingSystem();
    const char* mSetCommEncryption(char*);
    const char* mGetCommEncryption();
    const char* mSetEN61850DestAdr(char*);
    const char* mGetEN61850DestAdr();
    const char* mSetEN61850SourceAdr(char*);
    const char* mGetEN61850SourceAdr();
    const char* mSetEN61850EthTypeAppId(char*);
    const char* mGetEN61850EthTypeAppId();
    const char* mSetEN61850PriorityTagged(char*);
    const char* mGetEN61850PriorityTagged();
    const char* mSetEN61850EthSync(char*);
    const char* mGetEN61850EthSync();
    const char* mSetDspCommandStat(char*);
    const char* mGetDspCommandStat();
    const char* mSetEN61850DataCount(char*);
    const char* mGetEN61850DataCount();
    const char* mSetEN61850SyncLostCount(char*);
    const char* mGetEN61850SyncLostCount();
    const char* mTriggerIntListHKSK(char*);
    const char* mTriggerIntListALL(char*);
    const char* mResetMaxima(char*);
        
    // die routinen für das status modell
    
    const char* mResetDeviceLoadMax();
    const char* mGetDeviceLoadMax();
    const char* mGetDeviceLoadAct();   
    const char* mGetDspStatus();
    const char* mGetDeviceStatus();
  
    // die routinen für das measure modell
    
    const char* mFetch(char*);
    const char* mInitiate(char*);
    const char* mUnloadCmdList(char*);
    const char* mLoadCmdList(char*);
    const char* mSetRavList(char*);
    const char* mGetRavList();
    const char* mSetCmdIntList(char*);
    const char* mGetCmdIntList();
    const char* mSetCmdList(char*);
    const char* mGetCmdList();
    const char* mMeasure(char*);	     
    
    // die routinen für das memory modell
    
    const char* mDspMemoryRead(char*);
    const char* mDspMemoryWrite(char*);
    
    void DspIntHandler();
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

    QString Answer;
};

#endif
