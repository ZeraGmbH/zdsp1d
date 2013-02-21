// headerdatei zera hardware server 
// basis klasse für einen solchen
// rein virtuell -> definiert was ein hw-server
// immer benötigt

// cNodeZHServer ist eine erweiterung zum navigieren in scpi (ähnlichen)
// befehlslisten 

#ifndef ZHSERVER_H
#define ZHSERVER_H

#include <netinet/in.h>
#include <q3ptrlist.h>
#include <qstringlist.h>
#include <qstring.h>

#include "scpi.h"
#include "dsp1scpi.h"
#include "cmdinterpret.h"


class cNodeZHServer: public cNode {
public:
    cNodeZHServer(QStringList*,QString*,int,cNode*,cNode*,SCPICmdType,SCPICmdType); 
    virtual ~cNodeZHServer(){};
    // konstruktor, psNodeNames,psNode2Set, nNodedef, pNextNode, pNewLevelNode, Cmd, Query
    virtual cNode* TestNode(cCmdInterpreter*,char**); // zeiger, zeiger auf zeiger auf inputzeile, testet den knoten
    void SetNodeNameList(QStringList*); // zum späteren umschreiben der liste der knotennamen
private:
    QStringList* sNodeNames; // liste der möglichen nodes (es handelt sich z.b. um kanal namen)
    QString* psNode2Set; // der gefundene knoten name wird nach psNode2Set kopiert für weitere bearbeitung
};


class cZHClient {
public:
    cZHClient(){};
    cZHClient(int socket);
    ~cZHClient(); //  allokierten speicher ggf. freigeben
    int sock; // socket für den die verbindung besteht
    void SetOutput(const char*); // setzt den output, d.h. legt auch daten dafür an
    char* GetOutput(); // gibt den output zurück
    bool OutpAvail(); // true wenn mind. 1 zeichen im ausgabepuffer
    bool AsyncMessageAvail(); // dito
    void ClearInput(); // löscht den input buffer
    void AddInput(char*); // addiert einen teil string zum buffer
    char* GetInput(); // gibt zeiger auf input
    void AddAsyncMessage(const char*);
    char* GetAsyncMessage();

protected:
    QString sInput;
    QString sOutput;

private:
    QString m_sAsyncMessage;
    QString m_sRM;
};


class cZHServer {
public:
    cZHServer();
    cZHServer(cCmdInterpreter*);
    virtual ~cZHServer(){};
    virtual int Execute(); // server ausführen
    QString& GetSoftwareVersion();
    virtual int SetServerNr(const char*); // setzen der device nr -> neuen server namen
    virtual void AddClient(int socket); // fügt einen client hinzu
    virtual void DelClient(int); // entfernt einen client
protected:
    QString sServerName;
    static int ActSock; // der aktive socket im Execute 
    static QString sSoftwareVersion; // version des hw-servers (programm name + version )
    cCmdInterpreter* pCmdInterpreter; // der benutzte kommando interpreter
private:
    Q3PtrList<cZHClient> clientlist; // liste der clients
};    
    
#endif // ifndef ZHSERVER_H
