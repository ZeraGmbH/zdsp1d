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


//#define CheckSumOffset 56
//#define LeiterkartenName "wm3000i"
#define DSPDeviceNode "/dev/zFPGA1dsp1"
#define ServerBasisName "zdsp1d"
#define ServerVersion "V1.07"
#define InpBufSize 4096

// wenn DEBUG -> kein fork() 
//#define DEBUG 1

#endif
