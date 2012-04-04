// implementation für keyword parser

#include "parse.h"

const QString cParse::GetKeyword(char** s) { 
    QString ls = "";
    char tc=GetChar(s);
    if (tc) { // whitespace weg und schon mal 1 zeichen
	for (;;) {
	    ls+=tc;
	    tc=**s; // hole nächstes zeichen
	    if ( delimiter.contains(tc,false) ) break; // wenn zeichen delimiter -> fertig
	    if (!tc) break; // string zu ende -> fertig
	    (*s)++; // sonst nächstes zeichen 
	}
    };
    return(ls); // schlüsselwort ohne delimiter 
}

char cParse::GetChar(char** s)	{
    char tc=0;
    if (**s) { // abfrage für den fall, dass string schon zu ende
	do { 
	    tc = **s;
	    (*s)++;
	}
	while ( (tc) && (whitespace.contains(tc,false)) ); // ignoriere whitespace character
    }
    return(tc); // return = 0 oder zeichen != whitespace
}

const QString cParse::SetDelimiter(const QString s) {
    QString r = delimiter; 
    delimiter = s;
    return(r); // gib alten delimiter zurück (man weiss ja nie)
}

const QString cParse::SetWhiteSpace(const QString s) {
    QString r = whitespace;
    whitespace = s;
    return(r); // whitespace zurück
}
