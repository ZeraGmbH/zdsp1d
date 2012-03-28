// header datei für kommando interpreter

#ifndef KINTERPRETER_H
#define KINTERPRETER_H

#include <stdlib.h>
//#include <qstring.h>
#include "parse.h"

class cNode;
class cbIFace;

class cCmdInterpreter { // interpretiert einen inputstring und führt kommando aus (scpi)
public:
    cCmdInterpreter(cbIFace* cb, cNode* r, cParse* p) {
	m_pcbIFace = cb; m_pRootCmd = r; m_pParser = p; Answer = 0; };
    ~cCmdInterpreter() {if (Answer) free(Answer);};  // gibt speicher ggf. frei
    cParse* m_pParser;
    char* CmdExecute(char*);
    void SetAnswer(const char*);
    cbIFace* m_pcbIFace; // pointer auf call back interface
private:
    char* Answer; // der vom kommando generierte output steht hier 
    cNode* m_pRootCmd; // der startknoten aller kommandos
};

#endif

