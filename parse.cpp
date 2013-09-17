// implementation für keyword parser

#include "parse.h"

const QString cParse::GetKeyword(QChar **s)
{
    QString ls = "";
    QChar tc=GetChar(s);
    if (!tc.isNull())
    { // whitespace weg und schon mal 1 zeichen
        for (;;) {
            ls+=tc;
            tc=**s; // hole nächstes zeichen
            if (tc.isNull()) break; // string zu ende -> fertig
            (*s)++; // sonst nächstes zeichen
            if ( delimiter.contains(tc,false) ) break; // wenn zeichen delimiter -> fertig
        }
    };

    return(ls); // schlüsselwort ohne delimiter
}

QChar cParse::GetChar(QChar** s)
{
    QChar tc=0;
    if (!(**s).isNull())
    { // abfrage für den fall, dass string schon zu ende
        do {
            tc = **s;
            (*s)++;
        }
        while ( (!tc.isNull()) && (whitespace.contains(tc,false)) ); // ignoriere whitespace character
    }

    return(tc); // return = 0 oder zeichen != whitespace
}

const QString cParse::SetDelimiter(const QString s)
{
    QString r = delimiter; 
    delimiter = s;

    return(r); // gib alten delimiter zurück (man weiss ja nie)
}

const QString cParse::SetWhiteSpace(const QString s)
{
    QString r = whitespace;
    whitespace = s;

    return(r); // whitespace zurück
}
