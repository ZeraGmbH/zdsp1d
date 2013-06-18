// implementation des eigenlichen servers zdsp1(d)

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdatastream.h>
#include <qfile.h>                                                                                   
#include <qbuffer.h>
#include <qtextstream.h>
#include <unistd.h>

#include "zeraglobal.h"
#include "zdspglobal.h"
#include "zhserver.h"
#include "zdsp1d.h"
#include "dsp.h"
#include "parse.h"


#define ADSP_IOC_MAGIC 'a'
/* ioctl commands */
#define ADSP_RESET _IOR(ADSP_IOC_MAGIC,1,char*)
#define ADSP_BOOT _IOR(ADSP_IOC_MAGIC,2,char*)
#define ADSP_INT_REQ _IOR(ADSP_IOC_MAGIC,3,char*)
#define ADSP_INT_ENABLE _IOR(ADSP_IOC_MAGIC,4,char*)
#define ADSP_INT_DISABLE _IOR(ADSP_IOC_MAGIC,5,char*)
#define IO_READ _IOR(ADSP_IOC_MAGIC,6,char*)


extern sMemSection dm32DspWorkspace;
extern sMemSection dm32DialogWorkSpace;
extern sMemSection dm32UserWorkSpace;
extern sMemSection dm32CmdList;
extern sMemSection symbConsts1;

extern sDspVar CmdListVar;
extern sDspVar UserWorkSpaceVar;

extern cNode* InitCmdTree();


static char devavail[6] = "avail";
static char devnavail[10]= "not avail";

static char dsprunning[8] = "running";
static char dspnrunning[12]= "not running";


cZDSP1Client::cZDSP1Client(int socket,cZDSP1Server* server)
    :cZHClient(socket)
{
    init(server);
}


void cZDSP1Client::init(cZDSP1Server *server)
{
    m_sCmdListDef = m_sIntCmdListDef = "Empty"; // alle listen default leer
    cDspCmd DspCmd;
    m_DspCmdList.append(DspCmd);
    m_DspIntCmdList.append(DspCmd);
    myServer = server;
    DspVarResolver.addSection( &dm32DspWorkspace);
    DspVarResolver.addSection( &dm32DialogWorkSpace);
    DspVarResolver.addSection( &dm32UserWorkSpace);
    DspVarResolver.addSection( &dm32CmdList);
    DspVarResolver.addSection( &symbConsts1);
    DspVarResolver.addSection( &msec);
    msec.StartAdr = msec.n = 0;
    Encryption = 0; // es werden alle var. abfragen im klartext gesendet
    m_bActive = false;
}

QString& cZDSP1Client::SetRavList(QString& s)
{
    int i = 0;
    m_DspVarList.clear(); // liste löschen
    m_nlen = 0; // belegt keinen speicher

    sOutput = ACKString;
    if (!s.isEmpty())
    {
        cDspClientVar v;
        for (i=0;;i++)
        {
            QString t = s.section(';',i,i); // alle teil strings bearbeiten
            if (t.isEmpty()) break; // dann sind wir fertig
            if ( v.Init(t) )
            {
                v.SetOffs(m_nlen);
                m_DspVarList.append(v);
                m_nlen += v.size();
            }
            else
            { // fehlerfall
                m_DspVarList.clear(); // liste löschen
                m_nlen = 0; // belegt keinen speicher
                sOutput = NACKString;
                break;
            }
        }
    }
    
    msec.n = m_DspVarList.count();

    if (msec.n > 0)
    { // wir haben mindestens 1 variable
        varArray.resize(msec.n);
        for (i = 0;i < msec.n; i++)
        { // und machen diese dem resolver zugänglich
            varArray[i].Name = (char*) m_DspVarList[i].name().latin1() ;
            varArray[i].size = m_DspVarList[i].size();
            varArray[i].type = (dType)m_DspVarList[i].type();
        }
        msec.DspVar = varArray.data();
    }

    m_fDspMemData.resize(m_nlen); // speicher im array reservieren
    return (sOutput);
}

QString& cZDSP1Client::GetRavList()
{
    QTextStream ts( &sOutput, IO_WriteOnly );
    if ( !m_DspVarList.empty() )
    {
        tDspVarList::iterator it;
        for ( it = m_DspVarList.begin(); it != m_DspVarList.end(); ++it )
        {
            ts << (*it).name() << ',' << (*it).size() << ';';
        }
    }
    else ts << "Empty";
    return(sOutput);
}    
    
 
int cZDSP1Client::GetEncryption()
{
    return(Encryption);
}


void cZDSP1Client::SetEncryption(int i)
{
    Encryption=i;
}

    
QString& cZDSP1Client::SetCmdListDef(QString& s)
{
    m_sCmdListDef = s;
    sOutput = ACKString; // ist erstmal ok, wird später beim SET kommando geprüft
    return (sOutput);
}


QString& cZDSP1Client::GetCmdListDef()
{
    return (m_sCmdListDef);
}
 

QString& cZDSP1Client::SetCmdIntListDef(QString& s)
{
    m_sIntCmdListDef = s;
    sOutput = ACKString; // ist erstmal ok, wird später beim SET kommando geprüft
    return (sOutput);
}


QString& cZDSP1Client::GetCmdIntListDef()
{
    return (m_sIntCmdListDef);
}


bool cZDSP1Client::syntaxCheck(QString& s)
{
    int p1,p2=-1;
    bool ok = ( (((p1 = s.find('(')) > 0) && ((p2 = s.find(')')) > 0)) || (p2>p1) );
    return ok;
}


cDspCmd cZDSP1Client::GenDspCmd(QString& scmd,bool* ok)
{
    cParse CmdParser;
    CmdParser.SetDelimiter("(,)"); // setze die trennzeichen für den parser
    CmdParser.SetWhiteSpace(" (,)");
    
    char* cmds = (char*) scmd.latin1(); // zeiger auf den C-string von scmd
    sDspCmd *dspcmd;
    cDspCmd lcmd, cmd;
    QString sSearch = CmdParser.GetKeyword(&cmds); // das 1. keyword muss ein befehlscode sein
    if ( ((dspcmd = findDspCmd(sSearch)) != NULL ) && syntaxCheck(scmd) )
    { // bekannter befehlscode ?
        switch (dspcmd->CmdClass) {
        case CMD: // nur kommandowort, kein parameter
        {
            sSearch = CmdParser.GetKeyword(&cmds);
            *ok = sSearch.isEmpty(); // hier darf nichts stehen
            if (*ok) lcmd = cDspCmd(dspcmd->CmdCode);
            return lcmd;
        }
        case CMD1i16: // kommandowort, ein parameter
        {
            short par;
            bool t = true;
            sSearch = CmdParser.GetKeyword(&cmds);
            t &= ( (par = DspVarResolver.offs(sSearch)) > -1); // -1 ist fehlerbedingung
            sSearch = CmdParser.GetKeyword(&cmds);
            t &= sSearch.isEmpty();
            if (t) lcmd = cDspCmd(dspcmd->CmdCode,(ushort)par);
            *ok = t;
            return lcmd;
        }
        case CMD2i16:
        {
            short par[2];
            bool t = true;
            for (int i=0; i<2; i++)
            {
                sSearch = CmdParser.GetKeyword(&cmds);
                t &= ( (par[i] = DspVarResolver.offs(sSearch)) > -1);
            }
            sSearch = CmdParser.GetKeyword(&cmds);
            t &= sSearch.isEmpty();
            if (t) {
                lcmd = cDspCmd(dspcmd->CmdCode, (ushort)par[0], (ushort)par[1]);
                if (dspcmd->modify) lcmd.w[1] = (lcmd.w[1] & 0xFFFF) | (sock << 16);
            }
            *ok = t;
            return lcmd;
        }
        case CMD3i16:
        {
            short par[3];
            bool t = true;
            int i;
            for (i=0; i<3; i++)
            {
                sSearch = CmdParser.GetKeyword(&cmds);

                t &= ( (par[i] = DspVarResolver.offs(sSearch)) > -1);
            }
            sSearch = CmdParser.GetKeyword(&cmds);
            t &= sSearch.isEmpty();
            if (t)
            {
                lcmd = cDspCmd( dspcmd->CmdCode, (ushort)par[0], (ushort)par[1], (ushort)par[2]);
                if (dspcmd->modify) lcmd.w[1] = (lcmd.w[1] & 0xFFFF) | (sock << 16);
            }

            *ok = t;
            return lcmd;
        }
        case CMD1i32:
        {
            long par;
            bool t;
            sSearch = CmdParser.GetKeyword(&cmds);
            t = ( (par = DspVarResolver.offs(sSearch)) > -1);
            sSearch = CmdParser.GetKeyword(&cmds);
            t &= sSearch.isEmpty();
            if (t) lcmd = cDspCmd(dspcmd->CmdCode,(ulong)par);
            *ok = t;
            return lcmd;
        }
        case CMD1i161fi32:
        {
            short par1;
            long par2 = 0;
            bool t;
            sSearch = CmdParser.GetKeyword(&cmds);
            *ok = ( (par1 = DspVarResolver.offs(sSearch)) > -1); // -1 ist fehlerbedingung
            if (!(*ok))
                return lcmd; // wenn fehler -> fertig
            sSearch = CmdParser.GetKeyword(&cmds);
            par2 = sSearch.toLong(&t); // test auf integer
            if (!t) par2 = sSearch.toLong(&t,16); // test auf hex
            if (!t)  {
                float tf = sSearch.toFloat(&t);
                long* pl = (long*) &tf;
                par2= *pl;
            }
            sSearch = CmdParser.GetKeyword(&cmds);
            t &= sSearch.isEmpty();
            if (t) lcmd = cDspCmd(dspcmd->CmdCode,(ushort)par1,(ulong)par2);
            *ok = t;
            return lcmd;
        }
        }
    }
    *ok = false;
    return cmd;
}


void cZDSP1Client::SetActive(bool b)
{
    m_bActive = b;
}


void cZDSP1Client::SetStartAdr(ulong sa)
{
 //   m_nStartAdr = sa;
    msec.StartAdr = sa;
}


bool cZDSP1Client::GenCmdList(QString& s, tDspCmdList& cl, QString& errs)
{
    bool ok = true;
    cl.clear();
    for (int i = 0;;i++)
    {
        QString cs = s.section(';',i,i);
        if ( (cs.isEmpty()) || (cs==("Empty")) )break; // liste ist durch
        cl.append(GenDspCmd(cs, &ok));
        if (!ok)
        {
            errs = cs;
            break;
        }
    }
    return ok;
}


bool cZDSP1Client::GenCmdLists(QString& errs)
{
    bool ok;
    ok = GenCmdList(m_sCmdListDef,m_DspCmdList,errs);
    if (ok) ok =  GenCmdList(m_sIntCmdListDef, m_DspIntCmdList,errs);
    return ok;
}


bool cZDSP1Client::isActive()
{
    return m_bActive;
} 


tDspCmdList& cZDSP1Client::GetDspCmdList()
{
    return (m_DspCmdList);
}
 

tDspCmdList& cZDSP1Client::GetDspIntCmdList()
{
    return (m_DspIntCmdList);
}


tDspMemArray& cZDSP1Client::GetDspMemData()
{
    return(m_fDspMemData);
}


bool cZDSP1Client::InitiateActValues(QString& s)
{
    int fd = myServer->DevFileDescriptor;
    bool ok = false;

    if (s.isEmpty())
    { // sonderfall liste leer -> alle messwerte lesen
        QByteArray ba(m_nlen<<2);
        QDataStream bas( ba, IO_Raw | IO_ReadOnly);
        bas.setByteOrder(QDataStream::LittleEndian);
//        bas.setFloatingPointPrecision(QDataStream::SinglePrecision);
        if (myServer->DspDevSeek(fd, msec.StartAdr/*m_nStartAdr*/) >= 0)
        {
            if (myServer->DspDevRead(fd, ba.data(), m_nlen<<2) >= 0)
            {
                for (uint i = 0; i < m_fDspMemData.size(); i++)
                    bas >> m_fDspMemData[i];
                return true;
            }
        }
        return(false);
    }
    else
    {
        for (int i=0;;i++)
        {
            QString vs = s.section(";",i,i);
            vs=vs.stripWhiteSpace();
            if (vs.isEmpty())
            {
                ok = true;
                break; // dann sind wir fertig
            }

            tDspVarList:: iterator it;
            for (it = m_DspVarList.begin(); it != m_DspVarList.end(); it++)
                if ( (*it).name() == vs) break;
            if (it == m_DspVarList.end()) break; // fehler

            int len = (*it).size(); // in float
            int of = (*it).offs(); // dito
            QByteArray ba(len*4); // der benötigte speicher

            QDataStream bas( ba, IO_Raw | IO_ReadOnly);
            bas.setByteOrder(QDataStream::LittleEndian);
            // bas.setFloatingPointPrecision(QDataStream::SinglePrecision);

            if (myServer->DspDevSeek(fd,msec.StartAdr/* m_nStartAdr*/ + of) < 0) break; // file positionieren
            if (myServer->DspDevRead(fd, ba.data(), len*4 ) < 0) break; // fehler beim lesen

            for (int j = of; j < of+len; j++)
            {
                bas >> m_fDspMemData[j];
            }
        }
    }
    return ok;
}


QString& cZDSP1Client::FetchActValues(QString& s)
{
    sOutput="";
    QTextStream ts( &sOutput, IO_WriteOnly );
    QString tmps=s;
    tDspVarList:: iterator it;
    if (s.isEmpty())
        for (it = m_DspVarList.begin(); it != m_DspVarList.end(); it++) tmps = tmps + (*it).name() + ";";
    for (int i=0;;i++) {
        QString vs = tmps.section(";",i,i);
        vs=vs.stripWhiteSpace();
        if (vs.isEmpty()) break; // dann sind wir fertig

        tDspVarList:: iterator it;
        for (it = m_DspVarList.begin(); it != m_DspVarList.end(); it++)
            if ( (*it).name() == vs) break;
        if (it == m_DspVarList.end())
        {
            sOutput = ERREXECString; // fehler
            break;
        }

        int start = (*it).offs();
        int len = (*it).size();
        int end = start + len;
        int j;
        if (Encryption)
        {
            char* c;
            sOutput += QString("%1%2").arg((*it).name()).arg( ":");
            float *f = &m_fDspMemData[start];
            sOutput +=QString(c = qSEncryption((char*) f,len*4));
            delete c;
        }
        else
        {
            ts << (*it).name() << ":"; // messwerte sind immer float !!!!! das muss zu erkennen sein !!!!
            QString vs;
            for (j = start; j < end -1; j++) {
                vs = QString ("%1").arg( m_fDspMemData[j], 0, 'e', -1 ); // exp. ist immer float
                ts << vs << ",";
            }
            vs = QString ("%1").arg( m_fDspMemData[j], 0, 'e', -1 );
            ts << vs << ";";
        }
    }
    return sOutput;
}


bool cZDSP1Client::DspVar(QString& s,int& ir)
{ // einen int (32bit) wert lesen
    bool ret = false;
    QByteArray *ba = new QByteArray();
    QString ss = QString("%1,1").arg(s);
    if ( DspVarRead(ss,ba) != 0)
    { // 1 wort ab name (s) lesen
        ir = *((int*) (ba->data()));
        ret = true;
    }
    delete ba;
    return ret;
}


bool cZDSP1Client::DspVar(QString& s,float& fr)
{ // eine float wert lesen
    bool ret = false;
    QByteArray *ba = new QByteArray();
    QString ss = QString("%1,1").arg(s);
    if ( DspVarRead(ss,ba) != 0)
    {  // 1 wort ab name(s) lesen
        fr = *((float*) (ba->data()));
        ret = true;
    }
    delete ba;
    return ret;
}
 

sDspVar* cZDSP1Client::DspVarRead(QString& s,QByteArray* ba)
{
    bool ok;
    QString name = s.section(",",0,0);
    sDspVar *DspVar;
    if ( (DspVar = DspVarResolver.vadr(name)) == 0) return 0; // fehler, den namen gibt es nicht
    QString p = s.section(",",1,1);
    int n = p.toInt(&ok);
    if (!ok || (n<1) ) return 0; // fehler in der anzahl der elemente
    
    ba->resize(4*n);
    
    int fd = myServer->DevFileDescriptor;
    
    if ( (myServer->DspDevSeek(fd, DspVar->adr) >= 0) && (myServer->DspDevRead(fd, ba->data(), n*4 ) >= 0) )
        return DspVar; // dev.  seek und dev. read ok

    return 0; // sonst fehler		
}


char* cZDSP1Client::qSEncryption(char* ch,int n )
{
    char* tm1;
    short* tm2=new short[n+1]; // sind dopp elt soviele bytes wie in ba (+2)
    tm1=(char*) tm2; // zeiger um daten an string zu koppeln
    char c;
    for (int j=0;j<n;j++,ch++)
    {
        c=*ch;
        *tm2++ = (((c << 4) | c) & 0x0F0F) | 0x3030;
        //  *tm2++=((c>>4) & 0xF) | 0x30;
        //  *tm2++=(c & 0xF) | 0x30;
    }
    *tm2='!'; // delimiter ! weil ; in daten sein kann . die 0 als abschluss ist hier drin ....ich weiß
    return tm1;
}


QString& cZDSP1Client::DspVarListRead(QString& s)
{
    bool ok=false;
    sOutput="";
    QTextStream ts( &sOutput, IO_WriteOnly );
    QByteArray *ba = new QByteArray();
    
    for (int i=0;;i++)
    {
        QString vs = s.section(";",i,i); // variablen string:  varname, anzahl werte
        if (vs.isEmpty()) {
            ok = true;
            break; // dann sind wir fertig
        }

        sDspVar *DspVar;
        if ( (DspVar = DspVarRead(vs,ba)) == 0) break; // fehler aufgetreten

        int n = ba->size()/4;
        if (Encryption)
        {
            n=ba->size();
            char* c;
            sOutput +=QString("%1%2").arg(DspVar->Name).arg(":");
            sOutput += QString(c = qSEncryption((char*)(ba->data()),n));
            delete c;
        }
        else
        {
            ts << DspVar->Name << ":";
            switch (DspVar->type)
            {
            case eInt :{
                ulong *ul = (ulong*) ba->data();
                for (int j = 0; j < n-1; j++,ul++) ts << (*ul) << "," ;
                ts << *ul << ";" ;
                break;}
            case eUnknown:
            case eFloat :{
                float *f = (float*) ba->data();
                for (int j = 0; j < n-1; j++,f++) ts << (*f) << "," ;
                ts << *f << ";" ;
                break;}
            }
        };
    }
    delete ba;
    if (!ok) sOutput=ERREXECString;
    return sOutput;
}


const char* cZDSP1Client::DspVarWriteRM(QString& s)
{
    if ( DspVarWrite(s) ) return ACKString;
    else return ERREXECString;
}


bool cZDSP1Client::DspVarWrite(QString& s)
{
    const int gran = 10; // immer 10 elemente allokieren
    bool ok=false;
    int fd = myServer->DevFileDescriptor;
    
    for (int i=0;;i++)
    {
        QString vs = s.section(";",i,i);
        if (vs.isEmpty())
        {
            ok = true;
            break; // dann sind wir fertig
        }
        QString name = vs.section(",",0,0);
        long adr;
        int type;
        if ( (adr = DspVarResolver.adr(name, &type) ) == -1) break; // fehler, den namen gibt es nicht

        int n,alloc;
        n = alloc = 0; // keine elemente

        QByteArray ba;
        QDataStream bas ( ba, IO_Raw | IO_ReadWrite );
	
        bas.setByteOrder(QDataStream::LittleEndian);
        //bas.setFloatingPointPrecision(QDataStream::SinglePrecision);
        bool ok2 = true;
        if (type == eUnknown)
        {
            for (int j=1;;j++)
            {
                QString p = vs.section(",",j,j);
                if (p.isEmpty()) break;
                if (++n > alloc)
                {
                    alloc += gran;
                    ba.resize(alloc*4);
                }
                long vl = p.toLong(&ok2); // test auf long
                if (ok2) bas << vl;
                else {
                    ulong vul = p.toULong(&ok2); // test auf ulong
                    if (ok2) bas << vul;
                    else {
                        float vf = p.toFloat(&ok2); // test auf float
                        if (ok2) bas << vf;
                    }
                }
                if (!ok2) break;
            }
        }
        else
        {
            for (int j=1;;j++)
            {
                QString p = vs.section(",",j,j);
                if (p.isEmpty()) break;
                if (++n > alloc)
                {
                    alloc += gran;
                    ba.resize(alloc*4);
                }

                if (type == eInt)
                {
                    long vl = p.toLong(&ok2); // test auf long
                    if (ok2)
                        bas << vl;
                    else
                    {
                        ulong vul = p.toULong(&ok2); // test auf ulong
                        if (ok2) bas << vul;
                    }
                }
                else
                {
                    float vf = p.toFloat(&ok2); // test auf float
                    if (ok2)
                        bas << vf;
                }
                if (!ok2) break;
            }
        }

        if (!ok2) break;
        if (n>0) {
            if (myServer->DspDevSeek(fd, adr) < 0) break; // file positionieren
            if (myServer->DspDevWrite(fd, ba.data(), n*4 ) < 0) break; // fehler beim schreiben
        }
    }
    return ok;
}


/* globaler zeiger auf  "den"  server und eine signal behandlungsroutine */
cZDSP1Server* DSPServer;
int cZDSP1Server::gotSIGIO;
int pipeFD[2]; 
char pipeBuf[2] = "I";

void SigHandler(int)
{
    DSPServer->gotSIGIO = 1;
    if ((DSPServer->DebugLevel) & 2) 
	syslog(LOG_INFO,"dsp interrupt received\n");  
    write(pipeFD[1], pipeBuf, 1);
}


struct sigaction mySigAction;
sigset_t mySigmask, origSigmask;


cZDSP1Server::cZDSP1Server()
    :cZHServer()
{
    cParse* parser=new(cParse); // das ist der parser
    pCmdInterpreter=new cCmdInterpreter(this,InitCmdTree(),parser); // das ist der kommando interpreter
    m_sDspDeviceVersion = m_sDspSerialNumber = "Unknown"; // kennen wir erst mal nicht

    DebugLevel = 0; // MaxDebugLevel; // default alle debug informationen
    m_sDspDeviceNode = DSPDeviceNode; // default device node
    m_sDspBootPath = "";
    DSPServer = this;

    sigemptyset(&mySigmask);
    sigaddset(&mySigmask, SIGIO); // wir wollen sigio blocken wenn wir (p)select aufrufen
    sigprocmask(SIG_BLOCK, &mySigmask, &origSigmask);

    mySigAction.sa_handler = &SigHandler; // signal handler einrichten
    sigemptyset(&mySigAction.sa_mask);
    mySigAction. sa_flags = SA_RESTART;
    mySigAction.sa_restorer = NULL;
    sigaction(SIGIO, &mySigAction, NULL); // handler für sigio definieren 
    gotSIGIO = 0;
    ActivatedCmdList = 0; // der derzeit aktuelle kommando listen satz (0,1)
 }


cZDSP1Server::~cZDSP1Server()
{
    close(DevFileDescriptor); // close dev.
}


int cZDSP1Server::DspDevOpen()
{
    if ( (DevFileDescriptor = open(m_sDspDeviceNode.latin1(),O_RDWR)) < 0 )
    {
        if (DEBUG1)  syslog(LOG_ERR,"error opening dsp device: %s\n",m_sDspDeviceNode.latin1());
    }
    return DevFileDescriptor;
}


int cZDSP1Server::DspDevSeek(int fd, ulong adr)
{
    int r;
    if ( (r = lseek(fd,adr,0)) < 0 )
    {
        if  (DEBUG1)  syslog(LOG_ERR,"error positioning dsp device: %s\n",m_sDspDeviceNode.latin1());
    }
    return r;
}


int cZDSP1Server::DspDevWrite(int fd,char* buf,int len)
{
    int r;
    if ( (r = write(fd,buf,len)) <0 )
    {
        if (DEBUG1)  syslog(LOG_ERR,"error writing dsp device: %s\n",m_sDspDeviceNode.latin1());
    }
    return r;
}


int cZDSP1Server::DspDevRead(int fd,char* buf,int len)
{
    int r;
    if ( (r = read(fd,buf,len)) <0 )
    {
        if (DEBUG1)  syslog(LOG_ERR,"error reading dsp device: %s\n",m_sDspDeviceNode.latin1());
    }
    return r;
}


const char* cZDSP1Server::mTestDsp(char* s)
{
    int nr=0, tmode;
    bool ok, tstart;

    tstart=false;

    QString par = pCmdInterpreter->m_pParser->GetKeyword(&s); // holt den parameter aus dem kommando
    tmode=par.toInt(&ok);
    if ((ok) && ( (tmode>=0) && (tmode<2) ))
    {
        par = pCmdInterpreter->m_pParser->GetKeyword(&s);
        nr=par.toInt(&ok);
        if ((ok) && ( (nr>=0) && (nr<1000) ))
            tstart=true;
    }

    if (tstart == true)
    {
        int i,j;
        int errcount = 0;
        switch (tmode)
        {
            case 0:
                for (i=0; i<nr; i++)
                {
                    mResetDsp(s);
                    for (j=0; j< 100; j++)
                    {
                        usleep(1000);
                        if (Test4DspRunning() == false)
                            break;
                    }
                    if (j==100)
                        errcount++;
                    else
                    {
                        mBootDsp(s);
                        usleep(1000);
                        if (Test4DspRunning() == false)
                            errcount++;
                    }
                    Answer = QString("Test booting dsp %1 times, errors %2").arg(nr).arg(errcount);
                }
                break;

            case 1:
                const int n = 10000;
                int i,j;
                bool err;
                ulong faultadr=0;
                int bw, br, br2;
		
	  bw = br = br2 = 0;	
//                float tval;
                char byte;

                QByteArray ba; // wir werden 10000 floats in das array schreiben
                QByteArray ba2; // zurückgelesene daten
                ba.resize(n*4);
                ba2.resize(n*4);

//                QDataStream bas ( &ba, QIODevice::Unbuffered | QIODevice::ReadWrite );
//                bas.setByteOrder(QDataStream::LittleEndian);
//                bas.setFloatingPointPrecision(QDataStream::SinglePrecision);
//                tval = 1.0e32;
                byte = 0;
                for (i=0; i<n*4;i++)
                {
//                    tval *=-1.0;
//                    bas << (tval * (random() / RAND_MAX));
                    ba[i] = byte;
                    byte = (byte +1) % 256;
                }

                cZDSP1Client* cl = GetClient(ActSock);
                QString sadr  = "UWSPACE";
                ulong adr = cl->DspVarResolver.adr(sadr,NULL) ;
                for (i=0; i< nr; i++)
                {
                    if (DspDevSeek(DevFileDescriptor, adr) < 0)
                    {
                        Answer = QString("Test write/read dsp data, dev seek fault");
                        break; // file positionieren
                    }

                    if (DspDevWrite(DevFileDescriptor, ba.data(), n*4 ) < 0)
                    {
                        Answer = QString("Test write/read dsp data, dev write fault");
                        break; // fehler beim schreiben
                    }

                    if (DspDevSeek(DevFileDescriptor, adr) < 0)
                    {
                        Answer = QString("Test write/read dsp data, dev seek fault");
                        break; // file positionieren
                    }

                    if (DspDevRead(DevFileDescriptor, ba2.data(), n*4) < 0)
                    {
                        Answer = QString("Test write/read dsp data, dev read fault");
                        break; // fehler beim schreiben
                    }

                    err = false;
                    for (j=0; j<n*4; j++)
                    {
                        if (ba[j] != ba2[j])
                        {
                            bw = ba[j]; // das geschriebene byte
                            br = ba2[j]; // das gelesene byte
                            faultadr = adr + j;
                            DspDevRead(DevFileDescriptor, ba2.data(), n*4);
                            br2 = ba2[j];
                            err = true;
                        }
                        if (err)
                            break;
                    }

                    if (err)
                    {
                        Answer = QString("Test write/read dsp data, data fault adress %1, write %2, read1 %3, read2 %4").arg(faultadr,16).arg(bw,16).arg(br,16).arg(br2,16);
                        break; // file positionieren
                    }

                }

                if (i==nr)
                    Answer = QString("Test write/read dsp data, %1 times %2 bytes transferred, no errors").arg(nr).arg(n*4);

                break;

        }

    }

    else Answer = ERRVALString; // fehler wert
    return Answer.latin1();
}



const char* cZDSP1Server::mResetDsp(char*)
{

    int r = ioctl(DevFileDescriptor,ADSP_RESET); // und reset
    
    if ( r < 0 )
    {
        if (DEBUG1)  syslog(LOG_ERR,"error %d reset dsp device: %s\n",r,m_sDspDeviceNode.latin1());
        Answer = ERREXECString; // fehler bei der ausführung
        return Answer.latin1();
    }

    Answer = ACKString;
    return Answer.latin1();
}


const char* cZDSP1Server::mBootDsp(char*)
{
    QFile f (m_sDspBootPath);
    if (!f.open(IO_Raw | IO_ReadOnly))
    { // dsp bootfile öffnen
        if (DEBUG1)  syslog(LOG_ERR,"error opening dsp boot file: %s\n",m_sDspBootPath.latin1());
        Answer = ERRPATHString;
        return Answer.latin1();
    }
    
    long len = f.size();
    QByteArray BootMem(len);
    f.readBlock(BootMem.data(),len);
    f.close();
    
    int r = ioctl(DevFileDescriptor,ADSP_BOOT,BootMem.data()); // und booten
    
    if ( r < 0 )
    {
        if (DEBUG1)  syslog(LOG_ERR,"error %d booting dsp device: %s\n",r,m_sDspDeviceNode.latin1());
        Answer = ERREXECString; // fehler bei der ausführung
        return Answer.latin1();
    }

    Answer = ACKString;
    return Answer.latin1();
}


int cZDSP1Server::SetBootPath(const char * s)
{
    QString par = s;
    QFile bp(par);
    if ( bp.exists() )
    {
        m_sDspBootPath = par;
        return 0;
    }
    else return 1; // fehler path
}


const char* cZDSP1Server::mSetDspBootPath(char* s)
{
    QString par = pCmdInterpreter->m_pParser->GetKeyword(&s); // holt den parameter aus dem kommando
    if ( SetBootPath(par.latin1()) )
        Answer = ERRPATHString;
    else
        Answer = ACKString;
    return Answer.latin1();
}
	
    
const char* cZDSP1Server::mGetDspBootPath()
{
    return m_sDspBootPath.latin1();          
}    


const char* cZDSP1Server::mGetPCBSerialNumber()
{
    return m_sDspSerialNumber.latin1();          
}


const char* cZDSP1Server::mCommand2Dsp(QString& qs)
{
    do
    {
        Answer = ERREXECString;

        cZDSP1Client* cl = GetClient(ActSock);
        int ack;

        QString ss;
        if (! cl->DspVar( ss ="DSPACK",ack)) break;

        if ( ack ==  InProgress)
        {
            Answer = BUSYString;
            break;
        }

        if ( ack ==  CmdTimeout)
        {
            cl->DspVarWrite(ss = "DSPACK,0;");
            Answer = ERRTIMOString;
            break;
        }

        if (! cl->DspVarWrite(ss = "DSPACK,0;") ) break; // reset acknowledge
        if (! cl->DspVarWrite(qs)) break; // kommando und parameter -> dsp

        ioctl(DevFileDescriptor,ADSP_INT_REQ); // interrupt beim dsp auslösen
        Answer = ACKString; // sofort fertig melden ....sync. muss die applikation

    } while (0);
    return Answer.latin1();
}



const char* cZDSP1Server::mSetSamplingSystem(char* s)
{
    QString ss;
    return mCommand2Dsp(ss = QString("DSPCMDPAR,2,%1;").arg(s));
}	


const char* cZDSP1Server::mSetCommEncryption(char* s)
{
    bool ok;
    QString par = pCmdInterpreter->m_pParser->GetKeyword(&s); // holt den parameter aus dem kommando
    int enc=par.toInt(&ok);
    if ((ok) && ( (enc==0) || (enc==1) ))
    {
        cZDSP1Client* cl = GetClient(ActSock);
        cl->SetEncryption(enc);
        Answer = ACKString; // acknowledge
    }
    else Answer = ERRVALString; // fehler wert
    return Answer.latin1();
}


const char* cZDSP1Server::mGetSamplingSystem()
{
    do
    {
        Answer = ERREXECString;
        int n, ss, sm;

        cZDSP1Client* cl = GetClient(ActSock);

        QString s;
        if (! cl->DspVar(s = "NCHANNELS",n)) break;
        if (! cl->DspVar(s = "NSPERIOD",ss)) break;
        if (! cl->DspVar(s = "NSMEAS",sm)) break;

        Answer = QString("%1,%2,%3").arg(n).arg(ss).arg(sm);
    } while (0);
    return Answer.latin1();
}


const char* cZDSP1Server::mGetCommEncryption()
{
    cZDSP1Client* cl = GetClient(ActSock);
    Answer = QString::number(cl->GetEncryption());
    return Answer.latin1();
}


const char* cZDSP1Server::mSetEN61850SourceAdr(char* s)
{
    int i;
    QByteArray* ba = new(QByteArray);
    QString ss(s);
    ushort adr[6];
    bool ok;
    for (i = 0;i < 6;i++)
    {
        QString t = ss.section(',',i,i); // versuch 6 voneinander mit , getrennte parameter zu lesen
        adr[i] = t.toUShort(&ok);
        if (ok) ok &= (adr[i] < 256); // test ob adr bytes < 256
        if (!ok) break;
    }
    if (!ok) Answer = NACKString;
    else
        do
    {
        Answer = ERREXECString; // vorbesetzen
        QString as;
        cZDSP1Client* cl = GetClient(ActSock);
        if (!cl->DspVarRead(as = "ETHDESTSOURCEADRESS,3",ba)) break;
        else
        {
            ulong* pardsp = (ulong*) ba->data();
            pardsp[1] &= 0xFFFF0000; // die oberen bits behalten wir weil dest adr
            pardsp[1] = pardsp[1] | (adr[0] << 8) | adr[1];
            pardsp[2] = 0;
            for (i = 2;i<6;i++) pardsp[2] = (pardsp[2] << 8) + adr[i];
            mCommand2Dsp(as = QString("DSPCMDPAR,6,%1,%2,%3;").arg(pardsp[0]).arg(pardsp[1]).arg(pardsp[2])); // setzt answer schon
        }
    } while(0);
    delete ba;
    return Answer.latin1();
}


const char* cZDSP1Server::mSetEN61850DestAdr(char* s)
{
    int i;
    QByteArray* ba = new(QByteArray);
    QString ss(s);
    ushort adr[6]; // 2 * 4 werte reservieren
    bool ok;
    for (i = 0;i < 6;i++)
    {
        QString t = ss.section(',',i,i); // versuch 6 voneinander mit , getrennte parameter zu lesen
        adr[i] = t.toUShort(&ok);
        if (ok) ok &= (adr[i] < 256); // test ob adr bytes < 256
        if (!ok) break;
    }
    if (!ok) Answer = NACKString;
    else
        do
    {
        Answer = ERREXECString; // vorbesetzen
        QString as;
        cZDSP1Client* cl = GetClient(ActSock);
        if (!cl->DspVarRead(as = "ETHDESTSOURCEADRESS,3",ba)) break;
        else
        {
            ulong* pardsp = (ulong*) ba->data();
            pardsp[0] = 0;
            for (i = 0;i<4;i++) pardsp[0] = (pardsp[0] << 8) +adr[i];
            pardsp[1] &= 0xFFFF; // die unteren bits behalten wir weil source adr
            pardsp[1] = pardsp[1] | (adr[4] << 24) | (adr[5] << 16);
            mCommand2Dsp(as = QString("DSPCMDPAR,6,%1,%2,%3;").arg(pardsp[0]).arg(pardsp[1]).arg(pardsp[2]));
        }
    } while(0);
    delete ba;
    return Answer.latin1();
}


const char* cZDSP1Server::mSetEN61850EthTypeAppId(char* s)
{
    QString ss;
    cZDSP1Client* cl = GetClient(ActSock);
    if (! cl->DspVarWrite(ss = QString("ETHTYPEAPPID,%1;").arg(s)) ) 
	Answer = ERREXECString;
    else
	Answer = ACKString;
    
    return Answer.latin1();            
}


const char* cZDSP1Server::mGetEN61850EthTypeAppId()
{
    QByteArray* ba = new(QByteArray);
    QString as;
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->DspVarRead(as = "ETHTYPEAPPID,1",ba))
    {
        ulong *dataCount = (ulong*) ba->data(); // data zeigt auf 1*4 byte
        Answer = "";
        QTextStream ts( &Answer, IO_WriteOnly );
        ts << dataCount[0];
    }
    else
    {
        Answer = ERREXECString;
    }
    delete ba;
    return Answer.latin1();
}
	
 
const char* cZDSP1Server::mSetEN61850PriorityTagged(char* s)
{
    QString ss;
    cZDSP1Client* cl = GetClient(ActSock);
    if (! cl->DspVarWrite(ss = QString("ETHPRIORITYTAGGED,%1;").arg(s)) ) 
        Answer = ERREXECString;
    else
        Answer = ACKString;
    
    return Answer.latin1();     
}


const char* cZDSP1Server::mGetEN61850PriorityTagged()
{
    QByteArray* ba = new(QByteArray);
    QString as;
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->DspVarRead(as = "ETHPRIORITYTAGGED,1",ba))
    {
        ulong *dataCount = (ulong*) ba->data(); // data zeigt auf 1*4 byte
        Answer = "";
        QTextStream ts( &Answer, IO_WriteOnly );
        ts << dataCount[0];
    }
    else
    {
        Answer = ERREXECString;
    }
    delete ba;
    return Answer.latin1();
    
}


const char* cZDSP1Server::mSetEN61850EthSync(char* s)
{
    QString ss;
    cZDSP1Client* cl = GetClient(ActSock);
    if (! cl->DspVarWrite(ss = QString("SYNCASDU,%1;").arg(s)) ) 
	Answer = ERREXECString;
    else
	Answer = ACKString;
    
    return Answer.latin1();     
}
	
	
const char* cZDSP1Server::mGetEN61850EthSync()
{
    QByteArray* ba = new(QByteArray);
    QString as;
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->DspVarRead(as = "SYNCASDU,1",ba))
    {
        ulong *dataCount = (ulong*) ba->data(); // data zeigt auf 1*4 byte
        Answer = "";
        QTextStream ts( &Answer, IO_WriteOnly );
        ts << dataCount[0];
    }
    else
    {
        Answer = ERREXECString;
    }
    delete ba;
    return Answer.latin1();
    
}



const char* cZDSP1Server::mSetEN61850DataCount(char* s)
{
    QString ss;
    cZDSP1Client* cl = GetClient(ActSock);
    if (! cl->DspVarWrite(ss = QString("ETHDATACOUNT,%1;").arg(s)) ) 
        Answer = ERREXECString;
    else
        Answer = ACKString;
    
    return Answer.latin1();        
}
 

const char* cZDSP1Server::mGetEN61850DataCount()
{
    QByteArray* ba = new(QByteArray);
    QString as;
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->DspVarRead(as = "ETHDATACOUNT,2",ba))
    {
        ulong *dataCount = (ulong*) ba->data(); // data zeigt auf 2*4 byte
        Answer = "";
        QTextStream ts( &Answer, IO_WriteOnly );
        ts << dataCount[0] << "," << dataCount[1];
    }
    else
    {
        Answer = ERREXECString;
    }
    delete ba;
    return Answer.latin1();
}


const char* cZDSP1Server::mSetEN61850SyncLostCount(char* s)
{
    QString ss;
    cZDSP1Client* cl = GetClient(ActSock);
    if (! cl->DspVarWrite(ss = QString("ETHSYNCLOSTCOUNT,%1;").arg(s)) ) 
	Answer = ERREXECString;
    else
	Answer = ACKString;

    return Answer.latin1();        
}
 

const char* cZDSP1Server::mGetEN61850SyncLostCount()
{
    QByteArray* ba = new(QByteArray);
    QString as;
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->DspVarRead(as = "ETHSYNCLOSTCOUNT,1",ba))
    {
        ulong *dataCount = (ulong*) ba->data(); // data zeigt auf 1*4 byte
        Answer = "";
        QTextStream ts( &Answer, IO_WriteOnly );
        ts << dataCount[0];
    }
    else
    {
        Answer = ERREXECString;
    }
    delete ba;
    return Answer.latin1();
}


const char* cZDSP1Server::mGetEN61850SourceAdr()
{
    QByteArray* ba = new(QByteArray);
    QString as;
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->DspVarRead(as = "ETHDESTSOURCEADRESS,3",ba))
    {
        ulong* AdrByte = (ulong*) ba->data(); // data zeigt auf 3*4 byte
        ushort adr[6];  // dest, source address  sind je 6 byte
        int i;
        for (i = 0;i < 2;i++) adr[i] = ( AdrByte[1] >> ((1-i) * 8) ) & 0xFF;  // dsp byte order
        for (i = 0;i < 4;i++) adr[2+i] = ( AdrByte[2] >> ((3-i) * 8) ) & 0xFF; // -> network byte order
        Answer = "";
        QTextStream ts( &Answer, IO_WriteOnly );
        for (i = 0; i < 5; i++) ts << adr[i] << ",";
        ts << adr[i] << ";";
    }
    else
    {
        Answer = ERREXECString;
    }
    delete ba;
    return Answer.latin1();
}


const char* cZDSP1Server::mGetEN61850DestAdr()
{
    QByteArray* ba = new(QByteArray);
    QString as;
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->DspVarRead(as = "ETHDESTSOURCEADRESS,3",ba))
    {
        ulong* AdrByte = (ulong*) ba->data(); // data zeigt auf 3*4 byte
        ushort adr[6];  // dest, source address  sind je 6 byte
        int i;
        for (i = 0;i < 4;i++) adr[i] = ( AdrByte[0] >> ((3-i) * 8) ) & 0xFF;  // dsp byte order
        for (i = 0;i < 2;i++) adr[4+i] = ( AdrByte[1] >> ((3-i) * 8) ) & 0xFF; // -> network byte order
        Answer = "";
        QTextStream ts( &Answer, IO_WriteOnly );
        for (i = 0; i < 5; i++) ts << adr[i] << ",";
        ts << adr[i] << ";";
    }
    else
    {
        Answer = ERREXECString;
    }
    delete ba;
    return Answer.latin1();
}


const char* cZDSP1Server::mSetDspCommandStat(char* s)
{
    Answer = ERREXECString;
    QString ss;
	
    cZDSP1Client* cl = GetClient(ActSock);
    if (! cl->DspVarWrite(ss = QString("DSPACK,%1;").arg(s)) ) 
	Answer = ERREXECString;
    else
	Answer = ACKString;
    
    return Answer.latin1();    
}


const char* cZDSP1Server::mGetDspCommandStat()
{
    int stat;
    cZDSP1Client* cl = GetClient(ActSock);
	
    QString s;
    if (! cl->DspVar(s = "DSPACK",stat)) 
        Answer = ERREXECString;
    else
        Answer = QString("%1").arg(stat);
    
    return Answer.latin1();
}


const char* cZDSP1Server::mTriggerIntListHKSK(char* s)
{
    QString ss = s;
    ulong par = ss.toULong();
    par = (par & 0xFFFF )| (ActSock << 16);
    return mCommand2Dsp(ss = QString("DSPCMDPAR,4,%1;").arg(par)); // liste mit prozessNr u. HKSK 
}


const char* cZDSP1Server::mTriggerIntListALL(char*)
{
    QString ss;
    return mCommand2Dsp(ss = QString("DSPCMDPAR,1;"));
}


const char* cZDSP1Server::mResetMaxima(char*)
{
    QString ss;
    return mCommand2Dsp(ss =  QString("DSPCMDPAR,3;"));
}


int cZDSP1Server::SetDeviceNode(char* s)
{ // nur beim start zu setzen, nicht während des ablaufs
    QString devn = s;
    QFile dn(devn);
    if (dn.exists())
    {
        m_sDspDeviceNode = devn;
        return 0;
    }
    else return 1;
}


const char* cZDSP1Server::mGetDspDeviceNode()
{
    return m_sDspDeviceNode.latin1();
}


int cZDSP1Server::SetDebugLevel(const char* s)
{
    QString p = s;
    int dl = p.toInt();
    if ( (dl>=0) && (dl<=MaxDebugLevel) )
    {
        DebugLevel=dl;
        return 0;
    }
    else return 1;
}


const char* cZDSP1Server::mSetDspDebugLevel(char* s)
{
   QString par = pCmdInterpreter->m_pParser->GetKeyword(&s); // holt den parameter aus dem kommando
   if ( !SetDebugLevel(par.latin1())) Answer = ACKString;
   else Answer = ERRVALString;
   return Answer.latin1();
}


const char* cZDSP1Server::mGetDebugLevel()
{
    Answer = QString::number(DebugLevel);
    return Answer.latin1();          
}
 

const char* cZDSP1Server::mGetDeviceVersion()
{
    int r;
    r = ioctl(DevFileDescriptor,IO_READ,VersionNr);
    
    if ( r < 0 )
    {
        if (DEBUG1)  syslog(LOG_ERR,"error %d reading device version: %s\n",r,m_sDspDeviceNode.latin1());
        Answer = ERREXECString; // fehler bei der ausführung
        return Answer.latin1();
    }

    cZDSP1Client* cl = GetClient(ActSock);
    QString p = "VNR,1;";
    p = cl->DspVarListRead(p);  // ab "VNR"  1 wort lesen
    p = p.section(':',1,1);
    p = p.remove(';');
    double d = p.toDouble();
    m_sDspDeviceVersion = QString("DSPLCA: V%1.%2;DSP V%3").arg((r >>8) & 0xff).arg(r & 0xff).arg(d,0,'f',2);

    return m_sDspDeviceVersion.latin1();
}


const char* cZDSP1Server::mGetServerVersion()
{
    return sSoftwareVersion.latin1();
}


const char* cZDSP1Server::mGetDspStatus()
{
    if ( Test4DspRunning() )
        Answer = dsprunning;
    else
        Answer = dspnrunning;

    return Answer.latin1();
}


const char* cZDSP1Server::mGetDeviceStatus()
{
    if ( Test4HWPresent() )
        Answer = devavail;
    else
        Answer = devnavail;

    return Answer.latin1();
}


const char* cZDSP1Server::mGetDeviceLoadAct()
{
    cZDSP1Client* cl = GetClient(ActSock);
    QString p = "BUSY,1;";
    Answer = cl->DspVarListRead(p);  // ab "BUSY"  1 wort lesen

    return Answer.latin1();
}


const char* cZDSP1Server::mGetDeviceLoadMax()
{
    cZDSP1Client* cl = GetClient(ActSock);
    QString p = "BUSYMAX,1;";
    Answer = cl->DspVarListRead(p);  // ab "BUSYMAX"  1 wort lesen
    return Answer.latin1();
}


const char* cZDSP1Server::mResetDeviceLoadMax()
{
    cZDSP1Client* cl = GetClient(ActSock);
    QString p = "BUSYMAX,0.0";
    Answer = cl->DspVarWriteRM(p);
    return Answer.latin1();
}


const char* cZDSP1Server::mFetch(char* s)
{
    QString par = s;
    cZDSP1Client* cl = GetClient(ActSock);
    Answer = cl->FetchActValues(par);
    return Answer.latin1();
}


const char* cZDSP1Server::mInitiate(char* s)
{
    QString par = s;
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->InitiateActValues(par)) Answer = ACKString;
    else Answer = ERREXECString;
    return Answer.latin1();
}


QDataStream& operator<<(QDataStream& ds,cDspCmd c)
{
    ds << (Q_UINT32) c.w[0] << (Q_UINT32) c.w[1];
    return ds;
}

/*
void cZDSP1Server::DspIntHandler()
{ // behandelt den dsp interrupt
    int IRQCode;
    QString s;
    cZDSP1Client *client,*client2;
    bool clientAvail;
    int process = 0;

    clientAvail = ((client = clientlist.first()) !=0); // wir nutzen immer den 1. client zum lesen
    if (clientAvail) // wir haben noch einen über den wir lesen können
    {
        client->DspVar(s = "CTRLCMDPAR",IRQCode); // wir lesen die hksk
        process = IRQCode >> 16;
        clientAvail = ( (client2 = GetClient(process)) !=0); // ist der client noch da für den der interrupt bestimmt war?

    }

    if (!clientAvail)
    {
        if DEBUG1 syslog(LOG_ERR,"dsp interrupt mismatch, no such client (%d)\n",process);
        if (client != 0) // wir haben noch einen client zum rücksetzen der semaphore, wenn nicht -> ???? das darf aber nicht ->
            client->DspVarWrite(s = QString("CTRLACK,%1;").arg(CmdDone)); // acknowledge falls fehler
        else
        {
            cZDSP1Client *dummyClient = new cZDSP1Client(0,this); // dummyClient einrichten
            dummyClient->DspVarWrite(s = QString("CTRLACK,%1;").arg(CmdDone)); // und rücksetzen
        }
    }

    else
    {
        client->DspVarWrite(s = QString("CTRLACK,%1;").arg(CmdDone)); // sonst acknowledge
        IRQCode &= 0xFFFF;
        s = QString("DSPINT:%1").arg(IRQCode);
        client2->AddAsyncMessage((char*)s.latin1()); // die async. meldung
    }

    return ;
}
*/

void cZDSP1Server::DspIntHandler()
{ // behandelt den dsp interrupt

    QByteArray *ba;
    QString s;
    cZDSP1Client *client,*client2;
    int process = 0;

    if ((!clientlist.isEmpty()) && (client = clientlist.first()) !=0) // wenn vorhanden nutzen wir immer den 1. client zum lesen
    {
        //anzInt++;
        ba = new QByteArray();

        if (client->DspVarRead(s = "CTRLCMDPAR,20", ba)) // 20 worte lesen
        {
            ulong* pardsp = (ulong*) ba->data();
            int n = pardsp[0]; // anzahl der interrupts
            //qDebug() << QString("Interrupts n=%1").arg(n);
            for (int i = 1; i < (n+1); i++)
            {
                process = pardsp[i] >> 16;
                if ((client2 = GetClient(process)) !=0) // gibts den client noch, der den interrupt haben wollte
                {
                    s = QString("DSPINT:%1").arg(pardsp[i] & 0xFFFF);
                    client2->AddAsyncMessage((char*)s.latin1()); // die async. meldung

                }
            }

        }

        client->DspVarWrite(s = QString("CTRLACK,%1;").arg(CmdDone)); // jetzt in jedem fall acknowledge
        delete ba;
    }

    else

    {
        cZDSP1Client *dummyClient = new cZDSP1Client(0, this); // dummyClient einrichten
        dummyClient->DspVarWrite(s = QString("CTRLACK,%1;").arg(CmdDone)); // und rücksetzen
        delete dummyClient;
    }
}


bool cZDSP1Server::LoadDSProgram()
{ // die programmlisten aller aktiven clients laden
    
    // listen zusammen bauen

    bool ok;
    ulong umo = dm32UserWorkSpace.StartAdr; // usermememory offset
    QByteArray CmdMem;
    QByteArray CmdIntMem;
    QDataStream mds1 ( CmdMem, IO_Raw | IO_ReadWrite );
    mds1.setByteOrder(QDataStream::LittleEndian);
    QDataStream mds2 ( CmdIntMem, IO_Raw | IO_ReadWrite );
    mds2.setByteOrder(QDataStream::LittleEndian);
    cZDSP1Client* client;
    cDspCmd cmd;
    QString s,s2;
    
    QPtrListIterator<cZDSP1Client> it(clientlist);
    
    s =  QString( "DSPMEMOFFSET(%1)" ).arg(dm32DspWorkspace.StartAdr);
    client = it.toFirst();
    cmd = client->GenDspCmd(s, &ok);
    mds1 << cmd;

    while ( (client = it.current()) != 0)
    {
        ++it;
        if (client->isActive())
        {
            client->SetStartAdr(umo);
            s =  QString( "USERMEMOFFSET(%1)" ).arg(umo);
            cmd = client->GenDspCmd(s, &ok);
            mds1 << cmd;
            mds2 << cmd;
            umo += (client->GetDspMemData()).size(); // relokalisieren der daten im dsp
            tDspCmdList& cmdl = client->GetDspCmdList();
            for (uint i = 0; i < cmdl.size(); i++ ) mds1 << cmdl[i]; // cycl. liste
            tDspCmdList& cmdl2 = client->GetDspIntCmdList();
            for ( uint i = 0; i < cmdl2.size(); i++ ) mds2 << cmdl2[i]; // interrupt liste
        }
    }

    client = it.toFirst();
    s = QString( "DSPINTPOST()"); // wir triggern das senden der serialisierten interrupts
    cmd = client->GenDspCmd(s, &ok);
    mds1 << cmd;

    s =  QString( "INVALID()");
    cmd = client->GenDspCmd(s, &ok);
    mds1 << cmd; // kommando listen ende
    mds2 << cmd;
    
    ActivatedCmdList = (ActivatedCmdList + 1) & 1;
    if (ActivatedCmdList == 0)
    {
        s = QString("CMDLIST");
        s2=QString("INTCMDLIST");
    }
    else
    {
        s = QString("ALTCMDLIST");
        s2=QString("ALTINTCMDLIST");
    };

    ulong offset = client->DspVarResolver.adr(s, NULL) ;
    if (DspDevSeek(DevFileDescriptor, offset) < 0 )  // startadr im treiber setzen
        return false;
    
    if (DspDevWrite(DevFileDescriptor, CmdMem.data(), CmdMem.size()) < 0)
        return false;
    
    offset = client->DspVarResolver.adr(s2, NULL) ;
    if (DspDevSeek(DevFileDescriptor, offset) < 0 )  // startsadr im treiber setzen
        return false;
    
    if (DspDevWrite( DevFileDescriptor, CmdIntMem.data(), CmdIntMem.size()) < 0)
        return false;

    QString ss;
    mCommand2Dsp(ss = QString("DSPCMDPAR,7,%1;").arg(ActivatedCmdList));
    // dem dsp die neue liste mitteilen
    return true;
}



const char* cZDSP1Server::mUnloadCmdList(char *)
{
    cZDSP1Client* cl = GetClient(ActSock);
    cl->SetActive(false);
    if (!LoadDSProgram())
        Answer = ERREXECString;
    else
        Answer = ACKString;

    return Answer.latin1();
}


const char* cZDSP1Server::mLoadCmdList(char*)
{
    cZDSP1Client* cl = GetClient(ActSock);
    QString errs;
    if (cl->GenCmdLists(errs))
    { // die cmdlisten und die variablen waren schlüssig
        cl->SetActive(true);
        if (!LoadDSProgram())
            Answer = ERREXECString;
        else
            Answer = ACKString;
    }
    else
        Answer = QString("%1 %2").arg(ERRVALString).arg(errs); // das "fehlerhafte" kommando anhängen

    return Answer.latin1();
}


const char* cZDSP1Server::mSetRavList(char* s)
{
    QString qs = s;
    cZDSP1Client* cl = GetClient(ActSock);
    Answer  = cl->SetRavList(qs);

    return Answer.latin1(); 
}


const char* cZDSP1Server::mGetRavList() {
       cZDSP1Client* cl = GetClient(ActSock);
       Answer = cl->GetRavList();

       return Answer.latin1();
}


const char* cZDSP1Server::mSetCmdIntList(char* s)
{
    QString par = s;
    cZDSP1Client* cl = GetClient(ActSock);
    Answer = cl->SetCmdIntListDef(par);

    return Answer.latin1();     
}


const char* cZDSP1Server::mGetCmdIntList()
{
       cZDSP1Client* cl = GetClient(ActSock);
       Answer = cl->GetCmdIntListDef();

       return Answer.latin1();
}


const char* cZDSP1Server::mSetCmdList(char* s)
{
    QString par = s;
    cZDSP1Client* cl = GetClient(ActSock);
    Answer = cl->SetCmdListDef(par);

    return Answer.latin1();     
}
 

const char* cZDSP1Server::mGetCmdList()
{
      cZDSP1Client* cl = GetClient(ActSock);
      Answer = cl->GetCmdListDef();

      return Answer.latin1();    
}
 

const char* cZDSP1Server::mMeasure(char* s)
{
    QString par = s; // holt den parameter aus dem kommando
    cZDSP1Client* cl = GetClient(ActSock);
    if (cl->InitiateActValues(par))
        Answer = cl->FetchActValues(par);
    else
        Answer = ERREXECString;

    return Answer.latin1();
}


void cZDSP1Server::setDspType()
{
    int r;
    r = readMagicId();
    if ( r == MAGIC_ID21262 )
    {
        if (m_sDspBootPath == "") // wenn der benutzer denselben schon beim programmaufruf mitgesetzt hat, lassen wir das!
            // m_sDspBootPath = "/opt/zera/bin/zdsp21262.ldr"; // default dsp program name
	    m_sDspBootPath = "/home/zera/wm3000.ldr";
        // adressen im dsp stehen für adsp21262 default richtig
    }
    else
    {
        if (m_sDspBootPath == "") // dito
            //m_sDspBootPath = "/opt/zera/bin/zdsp21362.ldr"; // default dsp program name
	    m_sDspBootPath = "/home/zera/wm3000.ldr";
        // für adsp21362 schreiben wir die adressen um
        /*
        dm32DspWorkspace.StartAdr = dm32DspWorkSpaceBase21362;
        dm32DialogWorkSpace.StartAdr = dm32DialogWorkSpaceBase21362;
        dm32UserWorkSpace.StartAdr = dm32UserWorkSpaceBase21362;
        dm32CmdList.StartAdr = dm32CmdListBase21362;

        sDspVar* pDspVar = &CmdListVar;

        pDspVar->size = IntCmdListLen21362; pDspVar++;
        pDspVar->size = CmdListLen21362; pDspVar++;
        pDspVar->size = IntCmdListLen21362; pDspVar++;
        pDspVar->size = CmdListLen21362;

        pDspVar = &UserWorkSpaceVar;
        pDspVar->size = uwSpaceSize21362;
     */	
    }

}


int cZDSP1Server::readMagicId()
{
    int r;
    r = ioctl(DevFileDescriptor,IO_READ,MagicId);
    if (DEBUG1) syslog(LOG_INFO,"zdsp1d readMagicId : 0x%x\n", r);

    return r;
}


bool cZDSP1Server::Test4HWPresent()
{
    int r;
    r = readMagicId();

    return ( (r == MAGIC_ID21262) || (r == MAGIC_ID21362));
}


bool cZDSP1Server::Test4DspRunning()
{
    int r;
    r = ioctl(DevFileDescriptor,IO_READ,DSPStat);

    return ((r & DSP_RUNNING) > 0);
}	
	
	
const char* cZDSP1Server::mDspMemoryRead(char* s)
{
    QString par = s; 
    cZDSP1Client* cl = GetClient(ActSock);
    Answer = cl->DspVarListRead(par);

    return Answer.latin1();
}


const char* cZDSP1Server::mDspMemoryWrite(char* s) {
    QString par = s; 
    cZDSP1Client* cl = GetClient(ActSock);
    Answer = cl->DspVarWriteRM(par);

    return Answer.latin1();
}


cZDSP1Client* cZDSP1Server::GetClient(int s)
{
    cZDSP1Client* client;
    for (client = clientlist.first(); client; client = clientlist.next() )
        if ((client->sock) == s)
            return client;

    return NULL;
}


int cZDSP1Server::Execute() // server ausführen
{
    int sock;
    setDspType();
    if ( (sock = socket( PF_INET, SOCK_STREAM, 0)) == -1)
    { //   socket holen
        if DEBUG1 syslog(LOG_ERR,"socket() failed\n");
        return(1);
    }
    struct servent* se;
    if ( (se=getservbyname( sServerName.latin1(),"tcp")) == NULL )  // holt port nr aus /etc/services
    {
        if DEBUG1 syslog(LOG_ERR,"internet network services not found\n");
        return(1);
    }
    if ( pipe(pipeFD) == -1 )
    {
        if DEBUG1 syslog(LOG_ERR,"no pipe could be openend\n");
        return(1);
    }
    else
    {
	fcntl( pipeFD[1], F_SETFL, O_NONBLOCK); 
	fcntl( pipeFD[0], F_SETFL, O_NONBLOCK);
    }
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY; // alle adressen des host
    addr.sin_port = se->s_port; // ! s_port ist network byte order !
    addr.sin_family=AF_INET;
    if ( bind( sock, (struct sockaddr*) &addr, sizeof(addr)) == -1)
    { // ip-adresse und port an socket binden
        if DEBUG1 syslog(LOG_ERR,"bind() failed\n");
        return(1);
    }
    if ( listen(sock,3) == -1)
    { // einrichten einer warteschlange für einlaufende verbindungsaufbauwünsche
        if DEBUG1 syslog(LOG_ERR,"listen() faild\n");
        return(1);
    }
    char InputBuffer[InpBufSize];
    int nBytes;
    fd_set rfds,wfds; // menge von deskriptoren für read bzw. write
    int fd,fdmax,s,rm;
    for (;;) {
        FD_ZERO (&rfds);  // deskriptor menge löschen
        FD_ZERO (&wfds);

        if (gotSIGIO)
        {
            DspIntHandler(); // interrupt behandeln
            gotSIGIO = 0; // wenn behandelt -> flagge rücksetzen
        }

        fdmax=sock; // start socket
        FD_SET(sock,&rfds);
        FD_SET(pipeFD[0],&rfds);
        if (pipeFD[0] > fdmax)
	    fdmax = pipeFD[0];
        if ( ! clientlist.isEmpty())
            for ( cZDSP1Client* client=clientlist.first(); client; client=clientlist.next() )
            {
                fd=client->sock;
                FD_SET(fd,&rfds);
                if ( client->OutpAvail() || client->AsyncMessageAvail() ) // wir haben eine antwort oder eine asynch. meldung
                    FD_SET(fd,&wfds);
                if (fd>fdmax) fdmax=fd;
            }

        rm = pselect(fdmax+1,&rfds,&wfds,NULL,NULL,&origSigmask); // blockierender aufruf

        if (rm >= 0)  // wir wollten und können was senden bzw. wir können was lesen oder es war ein interrupt
        {

            if ( ! clientlist.isEmpty())
            { // erstmal input, bzw. client abmeldungen bearbeiten
                for ( cZDSP1Client* client=clientlist.first(); client; client=clientlist.next() )
                {
                    fd=client->sock;
                    if (FD_ISSET(fd,&rfds) )
                    { // sind daten für den client da, oder hat er sich abgemeldet ?
                        if ( (nBytes=recv(fd,InputBuffer,InpBufSize,0)) > 0  )
                        { // daten sind da
                            bool InpRdy=false;
                            switch (InputBuffer[nBytes-1]) { // letztes zeichen
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
                            if (InpRdy) {
                                ActSock=fd;
                                client->SetOutput(pCmdInterpreter->CmdExecute(client->GetInput())); // führt kommando aus und setzt output
                                client->ClearInput();
                            }
                        }
                        else
                        {
                            DelClient(fd); // client hat sich abgemeldet
                            close(fd);
                        }
                    }
                }
            }


            if ( ! clientlist.isEmpty())
            { // jetzt den output bearbeiten
                for ( cZDSP1Client* client=clientlist.first(); client; client=clientlist.next() )
                {
                    fd=client->sock;
                    if (FD_ISSET(fd,&wfds) )
                    { // soll und kann was an den client gesendet werden ?
                        if (client->OutpAvail())
                        {
                            QString out = client->GetOutput();
                            out+="\n";
                            send(fd,out.latin1(),out.length(),0);
                            client->SetOutput(""); // kein output mehr da
                        }
                        else
                            if (client->AsyncMessageAvail())
                            {
                                QString out = client->GetAsyncMessage();
                                out+="\n";
                                send(fd,out.latin1(),out.length(),0);
                            }
                    }
                }
            }

            if ( FD_ISSET(sock,&rfds) )
            { // hier ggf.  neuen client hinzunehmen
                int addrlen=sizeof(addr);
                if ( (s=accept(sock,(struct sockaddr*) &addr, (socklen_t*) &addrlen) ) == -1 )
                {
                    if DEBUG1 syslog(LOG_ERR,"accept() failed\n");
                }
                else
                {
                    AddClient(s);
                }
            }

            if ( FD_ISSET(pipeFD[0],&rfds) )
            { // we read the byte from pipe to avoid overflow	
		char buf[2];
		read(pipeFD[0], buf, 1);
            }
        }
    }
    close(sock);
    close(pipeFD[1]);
    close(pipeFD[0]);
}


void cZDSP1Server::SetFASync()
{
    fcntl(DevFileDescriptor, F_SETOWN, getpid()); // wir sind "besitzer" des device
    int oflags = fcntl(DevFileDescriptor, F_GETFL); 
    fcntl(DevFileDescriptor, F_SETFL, oflags | FASYNC); // async. benachrichtung (sigio) einschalten
}


void cZDSP1Server::AddClient(int socket)
{ // fügt einen client hinzu
    clientlist.append(new cZDSP1Client(socket,this));
    if DEBUG3 syslog(LOG_INFO,"client %d added\n",socket);
}


void cZDSP1Server::DelClient(int socket)
{ // entfernt einen client
    for ( cZDSP1Client* client = clientlist.first(); client; client = clientlist.next() )
    {
        if ((client->sock) == socket)
        {
            clientlist.remove(client);
            break;
        }
    }
    if DEBUG3 syslog(LOG_INFO,"client %d deleted\n",socket);
}


const char* cZDSP1Server::SCPICmd( SCPICmdType cmd, char* s)
{
    switch ((int)cmd)
    {
    case    TestDsp:        return mTestDsp(s);
    case 	ResetDsp:		return mResetDsp(s);
    case	BootDsp: 		return mBootDsp(s);
    case 	SetDspBootPath: 		return mSetDspBootPath(s);
    case 	eSetDebugLevel: 		return mSetDspDebugLevel(s);
    case  	Fetch:			return mFetch(s);
    case 	Initiate:			return mInitiate(s);
    case 	SetRavList: 		return mSetRavList(s);
    case 	SetCmdList: 		return mSetCmdList(s);
    case   SetCmdIntList: 		return mSetCmdIntList(s);
    case 	Measure: 		return mMeasure(s);
    case 	UnloadCmdList: 		return mUnloadCmdList(s);
    case 	LoadCmdList: 		return mLoadCmdList(s);
    case 	DspMemoryRead: 		return mDspMemoryRead(s);
    case   DspMemoryWrite:		return mDspMemoryWrite(s);
    case   SetSamplingSystem:	return mSetSamplingSystem(s);
    case	SetCommEncryption:	return mSetCommEncryption(s);
    case   SetEN61850DestAdr:    	return mSetEN61850DestAdr(s);
    case   SetEN61850SourceAdr:    	return mSetEN61850SourceAdr(s);
    case 	SetEN61850EthTypeAppId:  return mSetEN61850EthTypeAppId(s);
    case 	SetEN61850PriorityTagged:  return mSetEN61850PriorityTagged(s);
    case   SetEN61850EthSync: 	return mSetEN61850EthSync(s);
    case   SetDspCommandStat:	return mSetDspCommandStat(s);
    case   SetEN61850DataCount:	return mSetEN61850DataCount(s);
    case   SetEN61850SyncLostCount:	return mSetEN61850SyncLostCount(s);
    case   TriggerIntListHKSK:	return mTriggerIntListHKSK(s);
    case   TriggerIntListALL:		return mTriggerIntListALL(s);
    case   ResetMaxima:		return mResetMaxima(s);
    case   ResetDeviceLoadMax:	return mResetDeviceLoadMax();
    }
    Answer = "ProgrammierFehler"; // hier sollten wir nie hinkommen
    return Answer.latin1();
}


const char* cZDSP1Server::SCPIQuery( SCPICmdType cmd)
{
    switch ((int)cmd)
    {
    case 		GetDspBootPath: 		return mGetDspBootPath();
    case 		GetPCBSerialNumber: 	return mGetPCBSerialNumber();
    case		GetDspDeviceNode: 	return mGetDspDeviceNode();
    case		GetDebugLevel: 		return mGetDebugLevel();
    case 		GetDeviceVersion:		return mGetDeviceVersion();
    case 		GetServerVersion: 		return mGetServerVersion();
    case		GetDeviceLoadMax: 	return mGetDeviceLoadMax();
    case 		GetDeviceLoadAct: 	return mGetDeviceLoadAct();
    case		GetDspStatus:		return mGetDspStatus();	
    case 		GetDeviceStatus: 		return mGetDeviceStatus();
    case 		GetRavList: 		return mGetRavList();
    case 		GetCmdIntList: 		return mGetCmdIntList();
    case 		GetCmdList: 		return mGetCmdList();
    case		GetSamplingSystem:	return mGetSamplingSystem();	
    case        GetCommEncryption:	return mGetCommEncryption();
    case		GetEN61850DestAdr:	return mGetEN61850DestAdr(); 
    case		GetEN61850SourceAdr:	return mGetEN61850SourceAdr(); 
    case		GetEN61850EthTypeAppId:  return mGetEN61850EthTypeAppId();
    case		GetEN61850PriorityTagged:  return mGetEN61850PriorityTagged();
    case                 GetEN61850EthSync: 	return mGetEN61850EthSync();
    case		GetEN61850DataCount:	return mGetEN61850DataCount();
    case		GetEN61850SyncLostCount:	return mGetEN61850SyncLostCount();
    case 		GetDspCommandStat:	return mGetDspCommandStat();
    }
    Answer = "ProgrammierFehler"; // hier sollten wir nie hinkommen
    return Answer.latin1();
}


