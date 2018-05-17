// globale definition

#ifndef ZDSPGLOBAL_H
#define ZDSPGLOBAL_H


// v1.01 erste lauffähige version
// v1.02 ergänzt : 
// 	{"INC", 54, CMD1i16, 0 },
//	{"COPYINDDATA", 55, CMD3i16, 0 },
//	{"TestSyncPPSSkipEQ",56,CMD,0},
//	{"ResetSyncPPS",57,CMD,0}};
//v1.03 ergänzt :
//	es existieren im dsp ab version 3.05 2 sätze von kommando listen welche immer im wechsel geladen
// 	werden sollten. dies wird vom dsp server ab dieser version unterstüzt. die beiden versionen müssen
//	unbedingt zusammenpassen. wenn also ein dsp mit v3.05 und grösser vorhanden ist, muss unbedingt
// 	ein server mit v1.03 und grösser vorhanden sein und umgekehrt.
//v1.04 ergänzt :
//	es gab probleme im zusammenhang mit interrupts. ein einlaufender interrupt vom dsp konnte nicht bearbeitet werden wenn der select aufruf blockierte . input oder verbindungsaufbau liess das system weiter laufen. es wurde deshalb ein timeout (50 usec) eingeführt damit der select aufruf in jedem fall zurückkehrt. behandlung der flagge gotSIGIO wurde gändert, weil u.U. interrupts nicht bearbeitet wurden und sich der dsp dann aufhing.
//v1.05 ergänzt :
//	neuer befehl für dsp (SQRT) eingeführt .... läuft aber nur mit dsp ab V3.06
//v1.06 fehler bereinigt @ gencmd für cmdi16if32 fehler bei 1. parameter wurde nicht gemeldet
//v1.07 ergänzt :
//  neue befehle für dsp
//  {"COPYVAL",61,CMD2i16,0},
//  {"CMPCLK",62,CMD3i16,0},
//  {"CLKMODE",63,CMD1i16,0},
//  {"GETSTIME",64,CMD1i16,0},
//  {"TESTTIMESKIPLT",65,CMD2i16,0}
//  eingeführt. ..... läuft aber erst mit dsp ab version 3.07
//  ADSP21362 unterstützung eingebaut, automatische umschaltung über magicId
//v1.08 ergänzt :
// neue befehle für dsp
// {"SUBVCC",66,CMD3i16,0}}
//  eingeführt. ..... läuft aber erst mit dsp ab version 3.08
//v1.09:
//  diese version wurde aus oe(Qt4.8) zurück nach Qt3 geholt. sie beinhaltet
//  unter anderem neue befehle und daten typen für den dsp, wenn daten in
//  den dsp geschrieben werden. sie unterstützt das neue dsp programm ab
//  version 3.09, welches einen neuen, performanteren ethernet dekoder
//  beinhaltet. ganz wesentlich ist aber dass der neue dsp server deutlich
//  weniger system performance benötigt, weil das interrupt handling stark
//  geändert wurde.
//  im en61850 decoder war eine änderung notwendig...diese wurde im dsp ab v3.16 realisiert
//  mittlerweile hatte sich das memory mapping in dialogworkspace geändert und musste daher
//  angepasst werden. es wurde ein dsp cmd DSPINTPOST eingeführt welches ans ende aller
//  kommandoliste angehängt werden muss, weil wir aus gründen von interrupt latenzen im
//  dsp auf queued interrupts umgestellt hatten. darüber hinaus musste der interrupt handler
//  geändert werden.
//  v1.10
//  es wurden mehrere Kommandos für den dsp ergänzt. die liste war für den com5003 bereits gewachsen
//  und für die wm3000 musste ein kommando ergänzt werden damit der dc aus den messperioden herausgerechnet
//  werden kann. dies ist erforderlich für die amplitudenjustage weil zera referenzgeräte immer noch ohne
//  dc arbeiten.

#define DSPDeviceNode "/dev/adsp21262-1/0"
#define ServerBasisName "zdsp1d"
#define ServerVersion "V1.10"

#define InpBufSize 4096

// wenn DEBUG -> kein fork() 
//#define DEBUG 1

#endif
