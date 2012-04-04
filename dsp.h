// header datei dsp.h
// definitionen rund um den dsp

#ifndef DSP_H
#define DSP_H

#include <qstring.h>
#include <q3ptrlist.h>
#include <qdatastream.h>

#include "parse.h"

// enum zum lesen von dsp port adressen über ioctl
// Serial Port Interface,  Serial Interface, DSP Ctrl Register, what ?, the device name
enum IOCTL {SPI, Serial, DSPCtrl, DSPStat, DSPCfg, VersionNr, MagicId};

enum DspAcks {NBusy, InProgress, CmdError, ParError, CmdDone, CmdTimeout}; 

const int MAGIC_ID = 0xAA55BB44;
const int DSP_RUNNING = 0x80;

enum CmdType { CMD, CMD1i16, CMD2i16, CMD3i16 ,CMD1i32 , CMD1i161fi32 };
// 1fi32 kann 1 float oder 1 integer sein

struct sDspCmd { // wird zum ausdekodieren der dsp kommandos benötigt
    const char* Name; // name des befehls
    ushort	CmdCode; // der zugehörige befehlscode
    CmdType CmdClass; // der typ des befehls
    char modify; // !=0 -> verändern, diese befehle erhalten die prozessnr. (fd) als parameter 
};
    
sDspCmd* findDspCmd(QString&);
enum dType { eInt, eFloat};

struct sDspVar { // dient ebenfalls der dekodierung 
    const char* Name; // name der variablen
    ushort size;  // offset adresse
    dType type; // 
    ulong adr; // die abs. adresse auf welcher sich die variable befindet
};


struct sMemSection { // beschreibt eine dsp memory section
    long StartAdr;
    int n; // anzahl der sdspvar elemente
    sDspVar *DspVar;
};


class cDspVarResolver { // der löst die Variablen anhand der memsections und namen auf 
public:
    cDspVarResolver();
    void addSection(sMemSection*); // sections werden im konstruktor erstmal fest eingebunden
    long offs(QString&); // gibt die offs. adresse einer variablen zurück, -1 wenn es die variable nicht gibt
    long adr(QString&); // gibt die abs. adresse einer variablen zurück, -1 ...
    sDspVar* vadr(QString&); // wie adr(..) gibt aber zeiger auf sDspVar 
private:
    cParse VarParser;
    sDspVar *SearchedVar; // zeiger auf die gesuchte variable;
    long offs(QString&, sMemSection**);
    sMemSection *sec;
    Q3PtrList<sMemSection> MemSectionList;
};


class cDspClientVar { // zur verwaltung der dsp variablen auf client/server ebene
public:
    cDspClientVar();
    bool Init(QString&); // legt eine variable an 
    QString& name();
    void SetOffs(long);
    int size();
    ulong offs();
private:    
    QString m_sName; // eine dsp variable hat einen namen
    int m_nOffsAdr; // hat eine rel. start adresse
    int m_nSize; // und eine anzahl von elementen (float)
};


class cDspCmd { // hält einen 64bit dsp befehl incl. parameter 
public:
    cDspCmd(){w[0]=0;w[1]=0;}; 
    cDspCmd(const unsigned short); // nur befehl 16bit
    cDspCmd(const unsigned short,const unsigned short); // befehl 16bit und 16bit uint
    cDspCmd(const unsigned short, const unsigned long); // befehl und 32bit uint
    cDspCmd(const unsigned short, const unsigned short,const unsigned short,const unsigned short); // befehl und 3x  16bit uint
    cDspCmd(const unsigned short, const unsigned short,const unsigned short); // befehl und 2x  16bit uint
    cDspCmd(const unsigned short, const unsigned short,const unsigned long); // befehl und 1x  16bit uint und 1x 32bit uint ... kann auch ein float sein
//    void operator = (const cDspCmd &tc) { w[0] = tc.w[0]; w[1] = tc.w[1];};
    unsigned long w[2]; // ein DSP-Kommando besteht aus 64Bit
private:
};    

#endif    
