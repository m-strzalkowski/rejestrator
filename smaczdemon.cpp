/*
 * daemonize.c
 * This example daemonizes a process, writes a few log messages,
 * sleeps 20 seconds and terminates afterwards.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdarg.h>
#include "ekran.hpp"
#include "guziczki.hpp"

int (*printwsk)(const char *,...) = printf;//nie dziala w C++
int nic(const char *,...){return 0;}
void (*syslogproxy)(int, const char *,...) = syslog;
void nic2(int,const char *,...){return;}
void syslog_dummy(int s, const char * format, ...) {
   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
   putchar('\n');
}
#define printf printwsk
#define syslog syslogproxy

int pid_nagrywaj=-1;

void zakoncz()
{
    syslog(LOG_NOTICE, "Smaczdaemon terminated.");
    closelog();
    exit(EXIT_SUCCESS);
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGINT:syslog(LOG_NOTICE, "SIGINT");return;
    case SIGTERM:syslog(LOG_NOTICE, "SIGTERM");return;
    default:
        fprintf(stderr, "Nie tego sie spodziewalem!\n");
        abort();
    }
}


static void skeleton_daemon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }

    /* Open the log file */
    openlog ("smaczdemon", LOG_PID, LOG_DAEMON);
}
//zwracane
//-1 - blad
//0 - nie znaleziono
//pid
int pid_procesu_o_nazwie(const char * nazwa)
{
    static int i=0;
    int wczytano=0;
    int pid=0;
    char buf[2048] = {0};
    FILE * wy_pidof = NULL;
    sprintf(buf,"ps axo 'pid comm' |grep %s", nazwa);
    wy_pidof = popen((const char *)buf, "r");
    if(wy_pidof == NULL){syslog(LOG_WARNING, "Could not use 'ps axo 'pid comm' |grep %s'", nazwa); printf("open failed\n"); return -1;}
    buf[0]='\0';
    fgets(buf, sizeof(buf)-1, wy_pidof);
    buf[sizeof(buf)-1]='\0';
    pclose(wy_pidof);
    printf("%dBUF:'%s'\n",i,buf);
    wczytano = sscanf(buf, "%d", &pid);
    return pid;
}
void wylacz_komputer(void)
{
    if(pid_nagrywaj>0 && (getpgid(pid_nagrywaj) > 0))//jaka grupe ma proces o tym pid -> czy zyje
    {
        syslog(LOG_NOTICE, "Not started 'nagrywaj' because it is appaently already running, with pid %d %d",pid_nagrywaj, getpgid(pid_nagrywaj));
    }
    else{
        int status = system("shutdown now");
        syslog(LOG_NOTICE, "Issued a shutdown now:%d", status);
    }
}
void gdy_duzy_niski(void)
{
    printf("\nDUZY PRZELACZNIK W STANIE NISKIM\n\n");
    wylacz_komputer();
}
void gdy_duzy_wysoki(void)
{
    printf("\nDUZY PRZELACZNIK W STANIE WYSOKIM\n\n");
    wylacz_komputer();
}

void gdy_maly_niski(void)
{
    printf("\nMALY PRZELACZNIK W STANIE NISKIM\n\n");
    if(pid_nagrywaj>0 && (getpgid(pid_nagrywaj) > 0))//jaka grupe ma proces o tym pid -> czy zyje
    {
        syslog(LOG_NOTICE, "Not started 'nagrywaj' because it is appaently already running, with pid %d %d",pid_nagrywaj, getpgid(pid_nagrywaj));
    }
    else{
        syslog(LOG_NOTICE, "Starting 'nagrywaj'");
        int status = system("/home/ms/smacz/rejestrator/nagrywaj");
        /*
            char *argv[1] = {NULL};
        	int pid = fork();

        if ( pid == 0 ) {
            execvp( "/home/ms/smacz/rejestrator/nagrywaj", argv );
        }
        */
        syslog(LOG_NOTICE, "Starting 'nagrywaj:%d'", status);
    }
}
void gdy_maly_wysoki(void)
{
    printf("\nMALY PRZELACZNIK W STANIE WYSOKIM\n\n");
}

int main(int argc, char *argv[]) {
    int opt;
    bool demonizacja=false;
    if(chdir("/home/ms/smacz/rejestrator")!=0){perror("chdir nieudane"); exit(1);};
    signal(SIGINT,sig_handler);
    syslog(LOG_NOTICE, "Smaczdaemon start.");
    //signal(SIGTERM,sig_handler);

    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
        case 'd': demonizacja = true; break;
        default:
            fprintf(stderr, "Uzycie: %s [-d ]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if(demonizacja)
    {
        skeleton_daemon();
        printf = nic;
    }//else{syslog = syslog_dummy;}
    syslog(LOG_NOTICE, "Smaczdaemon demonized.");
    ustawWiringPi();
    ustawPinDuzegoPrzelacznika(gdy_duzy_niski, gdy_duzy_wysoki);
    ustawPinMalegoPrzelacznika(gdy_maly_niski, gdy_maly_wysoki);
    ustawPinyDuzych();
    syslog(LOG_NOTICE, "Smaczdaemon started.");

    char buf[2048] = {0};
    int wczytano=0;
    int i=0;
    int poprzedni_pid_nagrywaj=-2;
    while (1)
    {
        i++;
        //TODO: Insert daemon code here.
        sleep(1);
        /*FILE * wy_pidof = NULL;
        //wy_pidof = popen("pidof nagrywaj", "r");
        wy_pidof = popen("ps axo 'pid comm' |grep nagrywaj", "r");
        if(wy_pidof == NULL){syslog(LOG_WARNING, "Could not use popen to find 'nagrywaj'."); printf("open failed\n"); continue;}
        //wczytano = fscanf(wy_pidof, "%d", &pid_nagrywaj);
        fgets(buf, sizeof(buf)-1, wy_pidof);
        buf[sizeof(buf)-1]='\0';
        pclose(wy_pidof);
        printf("%dBUF:'%s'\n",i,buf);
        wczytano = sscanf(buf, "%d", &pid_nagrywaj);
        */
        pid_nagrywaj = pid_procesu_o_nazwie("nagrywaj");
        if(pid_nagrywaj>0){
            if(poprzedni_pid_nagrywaj != pid_nagrywaj)syslog(LOG_NOTICE, "Found 'nagrywaj' %d with pid:%d", wczytano,pid_nagrywaj);
        }
        else{
            if(poprzedni_pid_nagrywaj != pid_nagrywaj)syslog(LOG_NOTICE, "Not found 'nagrywaj'.");
            ekran_godzina();
        }
        poprzedni_pid_nagrywaj = pid_nagrywaj;
    }

    syslog(LOG_NOTICE, "Smaczdaemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}