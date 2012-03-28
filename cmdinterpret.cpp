// kommando interpreter für scpi kommandolisten

#include <stdlib.h>
#include <qstring.h>
#include "zeraglobal.h"
#include "dsp1scpi.h"
#include "scpi.h"
#include "cmdinterpret.h"



char* cCmdInterpreter::CmdExecute(char* s) {
    SetAnswer(ACKString);
    if (*s) { // leeres kommando ist nichts falsches -> also richtig
	char* CmdString=s; // der input string
	cNode* actNode=m_pRootCmd; // startknoten setzen
	cNode* prevNode;
	do {
	    prevNode=actNode;
	} while ( (actNode = actNode->TestNode(this,&CmdString)) );
	switch ( prevNode->m_nNodeStat ) {
	  case (isKnown | isCommand) : SetAnswer( m_pcbIFace->SCPICmd(prevNode->m_nCmd,CmdString));break;
	  case (isKnown | isQuery) : SetAnswer( m_pcbIFace->SCPIQuery(prevNode->m_nQuery));break;
	  default: SetAnswer(NACKString);
	  };
    }
    return (Answer);
}

void cCmdInterpreter::SetAnswer(const char* s)
{
    if (Answer) free(Answer);
    Answer=(char*) malloc(strlen(s)+1);
    strcpy(Answer,s);
}
