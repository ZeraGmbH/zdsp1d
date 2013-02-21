// implementation zhserver

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <qstring.h>
#include <qstringlist.h>
#include <q3valuelist.h>

#include "zdspglobal.h"
#include "zhserver.h"
#include "dsp1scpi.h"

class cNode;

// hier erst mal noch einen speziellen kommando knoten typ für den befehlsinterpreter 
// konstruktor, psNodeNames, psNode2Set,nNodeDef, pNextNode, pNewLevelNode, m_pCmd, m_pQuery 
cNodeZHServer::cNodeZHServer(QStringList* sl,QString* s,int ns,cNode* n1,cNode* n2,SCPICmdType ct,SCPICmdType qt)
    :cNode(ns,n1,n2,ct,qt)
{
    
    sNodeNames=sl;
    psNode2Set=s;
}


cNode* cNodeZHServer::TestNode(cCmdInterpreter* ci,char** inp)
{
    char* tinp=*inp;
    QString stmp;
    stmp=ci->m_pParser->GetKeyword(&tinp); // lässt das nächste schlüsselwort vom parser lesen
    QString STMP=stmp.upper();
    m_nNodeStat=isUnknown; // erst mal unbekannt
    for ( QStringList::iterator it = sNodeNames->begin(); it != sNodeNames->end(); ++it )
    {
        if ( ((*it) == stmp)  || ((*it) == STMP))
        {
            *psNode2Set=*it; // speichere knotenname an der vorgesehen stelle
            m_nNodeStat |= isKnown;
            break;
        }
    }

    if (m_nNodeStat == isUnknown) return (pNextNode); // zum prüfen des nächsten knoten ( solange != NULL gehts weiter )
    *inp=tinp; // hinter dem schlüsselwort gehts weiter
    
    char c=ci->m_pParser->GetChar(inp);
    switch (c)
    {
    case ':' 	: // es ist ein knoten
        if (nNodeDef & isNode) {
            m_nNodeStat |= isNode; // es darf einer sein
            if (m_nQuery) ci->m_pcbIFace->SCPIQuery(m_nQuery); // in abhängigkeit vom knoten muss noch was passieren (z.b. einen nachfolgenden knoten umbauen)
            return (pNewLevelNode); // dann return nächsten level ( d.h. es geht weiter )
        }
        else return (NULL); // es darf keiner sein -> fertig !!!
    case '?'	: // es ist eine query
        m_nNodeStat |= (nNodeDef & isQuery); // gesetzt wenn es eine sein darf
        return (NULL); // -> fertig !!!
    default		: // es ist ein command
        m_nNodeStat |= (nNodeDef & isCommand); // gesetzt wenn es eines sein darf
        *inp=tinp; // lass dem Kommando das zeichen (reparsen)
        return (NULL);
    }
}


void cNodeZHServer::SetNodeNameList(QStringList* sl)
{ // umschreiben der liste der knotennamen
    sNodeNames=sl;
}

// definition was ein client ist

cZHClient::cZHClient(int socket)
{
    sock=socket;
    m_sAsyncMessage = "";
    sInput = "";
    sOutput = "";
}


cZHClient::~cZHClient()
{
}


void cZHClient::SetOutput(const char* s)
{
    sOutput=s;
}


char* cZHClient::GetOutput()
{
    return((char*)sOutput.latin1());
}


bool cZHClient::OutpAvail()
{
    return (sOutput.length() > 0);
}


bool cZHClient::AsyncMessageAvail()
{
    return (m_sAsyncMessage.length() > 0);
}


void cZHClient::ClearInput()
{ // löscht den input buffer
    sInput="";
}


void cZHClient::AddInput(char* s)
{ // addiert einen teil string zum buffer
    sInput+=s;
}


char* cZHClient::GetInput()
{ // gibt zeiger auf input
    return((char*)sInput.latin1());
}


void cZHClient::AddAsyncMessage(const char * message)
{
    m_sAsyncMessage += ";";
    m_sAsyncMessage += message;
}


char *cZHClient::GetAsyncMessage()
{
    m_sRM = m_sAsyncMessage.section(';',1,1);
    m_sAsyncMessage.replace(0,m_sRM.length()+1,"");
    return (char*)m_sRM.latin1();
}


// hier jetzt die definitionen des eigenlichen server rumpfes

int cZHServer::ActSock; // der aktive socket im Execute 
QString cZHServer::sSoftwareVersion;


cZHServer::cZHServer()
{
    SetServerNr("0"); // default karte 0
    sSoftwareVersion = ServerBasisName;
    sSoftwareVersion += " ";
    sSoftwareVersion += ServerVersion;
    clientlist.setAutoDelete( TRUE ); // die liste hält die objekte
}


cZHServer::cZHServer(cCmdInterpreter* ci)
{
    cZHServer();
    pCmdInterpreter=ci;
}


int cZHServer::Execute()
{ // server ausführen
    int sock;
    if ( (sock = socket( PF_INET, SOCK_STREAM, 0)) == -1)
    { //   socket holen
        std::cout << "socket() failed\n";
        return(1);
    }
    struct servent* se;
    if ( (se=getservbyname( sServerName.latin1(),"tcp")) == NULL )
    {  // holt port nr aus /etc/services
        std::cout << "internet network services not found\n";
        return(1);
    }
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY; // alle adressen des host
    addr.sin_port = se->s_port; // ! s_port ist network byte order !
    addr.sin_family=AF_INET;
    if ( bind( sock, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    { // ip-adresse und port an socket binden
        std::cout << "bind() failed\n";
        return(1);
    }
    if ( listen(sock,3) == -1)
    { // einrichten einer warteschlange für einlaufende verbindungsaufbauwünsche
        std::cout << "listen() faild\n";
        return(1);
    }
    char InputBuffer[InpBufSize];
    int nBytes;
    fd_set rfds,wfds; // menge von deskriptoren für read bzw. write
    int fd,fdmax,s;
    for (;;) {
        FD_ZERO (&rfds);  // deskriptor menge löschen
        FD_ZERO (&wfds);

        fdmax=sock; // start socket
        FD_SET(sock,&rfds);
        if ( ! clientlist.isEmpty()) for ( cZHClient* client=clientlist.first(); client; client=clientlist.next() )
        {
            fd=client->sock;
            FD_SET(fd,&rfds);
            if ( client->OutpAvail() )
                FD_SET(fd,&wfds);
            if (fd>fdmax) fdmax=fd;
        }
        select(fdmax+1,&rfds,&wfds,NULL,NULL); // warte auf daten, oder das daten gesendet werden können

        if ( FD_ISSET(sock,&rfds) )
        { // hier ggf. client hinzunehmen
            int addrlen=sizeof(addr);
            if ( (s=accept(sock,(struct sockaddr*) &addr, (socklen_t*) &addrlen) ) == -1 )
            {
                std::cout << "accept() failed\n";
            }
            else
            {
                AddClient(s);
            }
        }

        if ( ! clientlist.isEmpty()) for ( cZHClient* client=clientlist.first(); client; client=clientlist.next() )
        {
            fd=client->sock;
            if (FD_ISSET(fd,&rfds) )
            { // sind daten für den client da, oder hat er sich abgemeldet ?
                if ( (nBytes=recv(fd,InputBuffer,InpBufSize,0)) > 0  )
                { // daten sind da
                    bool InpRdy=false;
                    switch (InputBuffer[nBytes-1])
                    { // letztes zeichen
                    case 0x0d: // cr
                        InputBuffer[--nBytes]=0; // c string ende daraus machen
                        InpRdy=true;
                        break;
                    case 0x0a: // linefeed
                        InputBuffer[--nBytes]=0;
                        if (nBytes)
                            if (InputBuffer[nBytes-1] == 0x0d) InputBuffer[--nBytes]=0;
                        InpRdy=true;
                        break;
                    case 0x04: // eof
                        InputBuffer[nBytes-1]=0; // c string ende daraus machen
                        InpRdy=true;
                        break;
                    case 0:
                        InpRdy=true; // daten komplett und 0 terminiert
                        break;
                    default:
                        InputBuffer[nBytes]=0; // teil string komplettieren
                    }

                    client->AddInput(&InputBuffer[0]);
                    if (InpRdy)
                    {
                        ActSock=fd;
                        client->SetOutput(pCmdInterpreter->CmdExecute(client->GetInput())); // führt kommando aus und setzt
                        client->ClearInput();
                    }
                }
                else
                {
                    DelClient(fd); // client hat sich abgemeldet ( hab den iterator zwar, aber DelClient ist virtuell !!! )
                    close(fd);
                }
            }
        }

        if ( ! clientlist.isEmpty()) for ( cZHClient* client=clientlist.first(); client; client=clientlist.next() )
        {
            fd=client->sock;
            if (FD_ISSET(fd,&wfds) )
            { // soll und kann was an den client gesendet werden ?
                char* out=client->GetOutput();
                send(fd,out,strlen(out)+1,0);
                client->SetOutput(""); // kein output mehr da .
            }
        }
    }
    close(sock);
}


QString& cZHServer::GetSoftwareVersion()
{ // abfrage software version
    return (sSoftwareVersion);
}


int cZHServer::SetServerNr(const char* s)
{
    bool ok=true;
    int r=1; // erstmal falsch
    QString snr=s;
    short nr= snr.toShort(&ok);
    if ( (ok) && (nr >= 0) && (nr < 10) )
    { // karte 0..9 erlaubt
        sServerName=ServerBasisName+snr; // z.b. wm3000id0
        r=0;
    }
    return (r);
}


void cZHServer::AddClient(int socket)
{ // fügt einen client hinzu
    clientlist.append(new cZHClient(socket));
    std::cout << "Client " << socket << " added\n";
}

void cZHServer::DelClient(int s)
{ // entfernt einen client
    for ( cZHClient* client = clientlist.first(); client; client = clientlist.next() )
    {
        if ((client->sock) == s)
        {
            clientlist.remove(client);
            std::cout << "Client " << s << " deleted\n";
            break;
        }
    }
}

