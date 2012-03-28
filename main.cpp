#include <iostream>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <qstring.h>
#include "zdsp1d.h"
#include "zdspglobal.h"

int main( int argc, char *argv[] )
{
    pid_t pid;
    openlog(ServerBasisName, LOG_PID, LOG_DAEMON); // verbindung zum syslogd aufnehmen

    cZDSP1Server* zdsp1d=new cZDSP1Server(); // das ist der server
   
    // überprüfen ob default einstellungen überschrieben werden sollen
    int r=0;   
    
    if ( ( argc > 1 ) && (zdsp1d->SetDebugLevel(argv[1]) ) ) {
	syslog(LOG_EMERG,"illegal debug level\n") ; 
	r=1;
    } 
    
    if ( ( argc > 2 ) && ( zdsp1d->SetServerNr(argv[2]) ) ) { //  neue dsp karten nummer 
	syslog(LOG_EMERG,"illegal pcb nr\n") ; 
	r=1;
    } 
    
    if ( ( argc > 3) && ( zdsp1d->SetDeviceNode(argv[3]) ) ) { // neuer device knoten
	syslog(LOG_EMERG,"illegal dsp device node\n");
	r=1;
    }
    
     if ( ( argc > 4) && ( zdsp1d->SetBootPath(argv[4]) ) ) { // neuer boot path
	syslog(LOG_EMERG,"illegal dsp boot path\n");
	r=1;
    }
    
    if (r==0) { // nur wenn bis jetzt kein fehler aufgetreten ist
#ifndef DEBUG
	if ( (pid=fork() ) < 0 ) { // kindprozess generieren 
	    syslog(LOG_EMERG,"fork() failed\n") ; // fehlermeldung falls nicht
	    return (1);
	}
	
	if (pid==0) { // wenn es der kindprozess ist
	    zdsp1d->SetFASync(); // erst hier, weil sich die pid durch fork ändert 
	    syslog(LOG_INFO,"zdsp1d server child process created\n");
	    chdir ("/"); // bekommt er einen platz zum "leben"
	    setsid();
	    close (STDIN_FILENO); // löst sich von der konsole
	    close (STDOUT_FILENO);
	    close (STDERR_FILENO);
	    open ("/dev/null",O_RDWR); 
	    dup (STDIN_FILENO);
	    dup (STDOUT_FILENO);
                  r=zdsp1d->Execute(); // und läuft von nun an eigenständig
                  syslog(LOG_INFO,"zdsp1d server child process terminated\n");		  
	}
#else
	zdsp1d->SetFASync();
	r=zdsp1d->Execute(); // wenn DEBUG -> kein fork -> server läuft im vordergrund 
#endif // DEBUG	
    }
    closelog();
    return (r);
}
