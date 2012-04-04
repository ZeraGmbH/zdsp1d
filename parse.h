// header datei für keyword parser

#ifndef KPARSER_H
#define KPARSER_H

#include <qstring.h>

class cParse { // sucht nach zusammenhängenden wörtern mit frei wählbaren delimitern und whitespaces
public:
    cParse() {delimiter = " :?;"; whitespace = " ";}; // setzt default delimiter, whitespace
    const QString GetKeyword(char**); // liesst nächstes schlüsselwort aus string
    char GetChar(char**); // liesst das nächste zeichen aus string
    const QString SetDelimiter(const QString s);
    const QString SetWhiteSpace(const QString s);
private:
    QString delimiter;
    QString whitespace;
};

#endif
