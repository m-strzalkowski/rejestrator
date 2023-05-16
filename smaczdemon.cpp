/*
 *** SMACZDEMON.cpp ***
Program, który ma dzialac jako demon i sluchac przelacznika, wlaczajac nagrywanie. Poza tym wypisuje zegarek i wylacza system za drugim przelacznikiem
vide: man smaczdemon
*/

/* /etc/systemd/system/smaczdemon.service:
[Unit]
Description=Smacz Monitoring System Daemon
Documentation=man:smaczdemon(1)

[Service]
Type=simple
ExecStart=/home/ms/smacz/rejestrator/smaczdemon
KillSignal=SIGINT
RestartKillSignal=SIGINT
StandardOutput=null
Restart=on-failure
#TasksMax=8
# Increase the default a bit in order to allow many simultaneous
# files to be monitored, we might need a lot of fds.
#LimitNOFILE=16384

[Install]
WantedBy=multi-user.target
Alias=smaczdemon.service
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <stdarg.h>
#include <unistd.h>
#include "ekran.hpp"
#include "guziczki.hpp"
#include "piny_guzikow.hpp"
//brzydka magia, nie czytac.
int (*printwsk)(const char *,...) = printf;
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

//wylaczanie z gracja oraz obsluga sygnalow
int pid_nagrywaj=-1;
bool nie_rysuj=false;
bool nagrywaj_ma_byc_wlaczane=false;
bool wypisuj_wifi=false;
void zakoncz()
{
    syslog(LOG_NOTICE, "Smaczdaemon terminated.");
    system("pkill --signal SIGINT nagrywanie");
    closelog();
    exit(EXIT_SUCCESS);
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGUSR1:nagrywaj_ma_byc_wlaczane=false;syslog(LOG_NOTICE, "REC HAS TO BE OFF");return;
    case SIGINT:syslog(LOG_NOTICE, "SIGINT");zakoncz();
    case SIGTERM:syslog(LOG_NOTICE, "SIGTERM");zakoncz();
    default:
        fprintf(stderr, "Nie tego sie spodziewalem!\n");
        abort();
    }
}

//poprawna demonizacja, jak sie okazuje, niepotrzebna a wrecz zabroniona przy [Service]\n Type=simple (wtedy usluga umiera)
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
    openlog ("smaczdemon", LOG_PID , LOG_DAEMON);
}

//zwracane
//-1 - blad
//0 - nie znaleziono
//pid, pomija wołającego siebie na liście
int pid_innego_procesu_o_nazwie(const char * nazwa)
{
    static int i=0;
    int wczytano=0;
    int pid=0;
    char buf[2048] = {0};
    FILE * wy_pidof = NULL;
    sprintf(buf,"ps axo 'pid comm' |grep %s | grep -v grep| grep -v %d", nazwa,getpid());
    wy_pidof = popen((const char *)buf, "r");
    if(wy_pidof == NULL){syslog(LOG_WARNING, "Could not use 'ps axo 'pid comm' |grep %s'", nazwa); printf("open failed\n"); return -1;}
    buf[0]='\0';
    fgets(buf, sizeof(buf)-1, wy_pidof);
    buf[sizeof(buf)-1]='\0';
    pclose(wy_pidof);
    //syslog(LOG_NOTICE, "pid_innego_procesu_o_nazwie -> %s",buf);
    printf("%dBUF:'%s'\n",i,buf);
    wczytano = sscanf(buf, "%d", &pid);
    return wczytano ? pid:-1;
}

void wylacz_komputer(int kod)
{
    pid_nagrywaj = pid_innego_procesu_o_nazwie("nagrywaj");
    if(pid_nagrywaj>0 && (getpgid(pid_nagrywaj) > 0))//jaka grupe ma proces o tym pid -> czy zyje
    {
        syslog(LOG_NOTICE, "Not shutted down system bacause nagrywaj is running, with pid %d %d",pid_nagrywaj, getpgid(pid_nagrywaj));
    }
    else{
        nie_rysuj = true;
        char buf[256];
        sprintf(buf, "WYLACZANIE SYSTEMU:%d",kod);
        ekran_nagrywanie(buf,(char *)"");
        syslog(LOG_NOTICE, "Code of cause of shutting down::%d", kod);
        int status = system("shutdown now");
        syslog(LOG_NOTICE, "Issued a shutdown now:%d", status);
        exit(EXIT_SUCCESS);
    }
}
//callbaki do przelacznikow
void gdy_duzy_niski(void)
{
    syslog(LOG_NOTICE, "DUZY PRZELACZNIK W STANIE WYSOKIM");
    printf("\nDUZY PRZELACZNIK W STANIE NISKIM\n\n");
    if(!nagrywaj_ma_byc_wlaczane){wylacz_komputer(__LINE__);}
}
void gdy_duzy_wysoki(void)
{
    syslog(LOG_NOTICE, "DUZY PRZELACZNIK W STANIE NISKIM");
    printf("\nDUZY PRZELACZNIK W STANIE WYSOKIM\n\n");
}
void uruchom_nagrywaj()
{
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
void gdy_maly_niski(void)
{
    printf("\nMALY PRZELACZNIK W STANIE NISKIM\n\n");
    //if(!nagrywaj_ma_byc_wlaczane){nagrywaj_ma_byc_wlaczane=true;}
    //else{nagrywaj_ma_byc_wlaczane=false;}
    nagrywaj_ma_byc_wlaczane = ! nagrywaj_ma_byc_wlaczane;
    syslog(LOG_NOTICE, "REC HAS TO BE %s", (nagrywaj_ma_byc_wlaczane?"ON":"OFF"));
    if(nagrywaj_ma_byc_wlaczane)
    {
        fprintf(stderr,"uruchom nagrywaj");
        uruchom_nagrywaj();
    }
    else{
        fprintf(stderr,"pkill");
        int kod = system("pkill --signal SIGINT nagrywanie");
        syslog(LOG_NOTICE, "pkill --signal SIGINT nagrywanie:code %d",kod);
    }
}
void gdy_maly_wysoki(void)
{
    printf("\nMALY PRZELACZNIK W STANIE WYSOKIM\n\n");
}

void pierwsza_linia_komendy(char * buf,int bufsz, const char * komenda)
{
    FILE * wy_pidof = NULL;
    wy_pidof = popen((const char *)komenda, "r");
    if(wy_pidof == NULL){syslog(LOG_WARNING, "Could not use %s'", komenda); printf("open failed\n"); return;}
    fgets(buf, bufsz-1, wy_pidof);
    buf[bufsz-1]='\0';
    pclose(wy_pidof);
    return;
}
void pokaz_wifi_zmien(){wypisuj_wifi=!wypisuj_wifi;}
void pokaz_nazwe_wifi()
{
    //if(!wypisuj_wifi){return;}
    char buf[256] = {0};
    pierwsza_linia_komendy(buf,sizeof(buf),"iwgetid | grep '\".*\"' -o");
    printf("nazwa wifi:%s",buf);
    ekran_nagrywanie(buf,(char *)"");
}
void zmien_rozmiar_tekstu(void){static int i=1; i+=1; if(i>4){i=1;} rozmiar_tekstu(i);}
int main(int argc, char *argv[]) {
    //opcje
    int opt;
    bool demonizacja=false;

    int pidinnegodemona=-1;
    if((pidinnegodemona=pid_innego_procesu_o_nazwie("smaczdemon"))>0)
    {
        syslog(LOG_PERROR,"Wyglada na to, że chodzi już jakiś smaczdemon:%d",pidinnegodemona);
        sleep(1);
        syslog(LOG_INFO,"Wiec ja zdycham.");
        exit(EXIT_FAILURE);
    }

    if(chdir("/home/ms/smacz/rejestrator")!=0){syslog(LOG_PERROR,"chdir nieudane"); exit(1);};
    signal(SIGINT,sig_handler);
    signal(SIGTERM,sig_handler);
    signal(SIGUSR1,sig_handler);
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

    syslog(LOG_NOTICE, "Smaczdaemon daemonized.");
    //USTAWIANIE KONFIGURACJI SPRZETOWEJ
    /*
    !! - UWAGA - ZMIENIA CHARAKTERYSTYKE FIZYCZNYCH OBWODOW, WLACZAJAC REZYSTORY PULL-UP NA NIEKTORYCH PINACH - !!
    */
    ustawWiringPi();
    ustawPinDuzegoPrzelacznika(gdy_duzy_niski, gdy_duzy_wysoki);
    ustawPinMalegoPrzelacznika(gdy_maly_niski, gdy_maly_wysoki);
    ustawPinyDuzych();
    ustawPinOdDuzego(PIN_DUZEGO_C, &pokaz_wifi_zmien);
    //ustawPinOdDuzego(PIN_DUZEGO_A, &odwroc_ekran);
    //ustawPinOdDuzego(PIN_DUZEGO_B, &zmien_rozmiar_tekstu);
    //ustawPinOdDuzego(PIN_DUZEGO_D, &przekrec_ekran);
    syslog(LOG_NOTICE, "Smaczdaemon started.");

    int wczytano=0;
    int i=0;
    int poprzedni_pid_nagrywaj=-2;
    char buf[256];
    while(1)
    {
        i++;
        //MALY NISKI (0) - NAGRYWAJ MA BYC WLACZONE
        //MALY WYSOKI (1) - NAGRYWAJ MA BYC WYLACZONE
        bool poprz_nagr=nagrywaj_ma_byc_wlaczane;
        nagrywaj_ma_byc_wlaczane = ! czytaj_pin(PIN_MALEGO_PRZELACZNIKA);
        if(poprz_nagr != nagrywaj_ma_byc_wlaczane){syslog(LOG_NOTICE, "CORRECTED THAT REC HAS TO BE %s", (nagrywaj_ma_byc_wlaczane?"ON":"OFF"));}

        if(!nagrywaj_ma_byc_wlaczane)//sprawdzanie wylaczania co sekunde, bo przerwanie nie lapie na tym przelaczniku
        {
            if(czytaj_pin(PIN_DUZEGO_PRZELACZNIKA) == 1)//wysoki - rozlaczony
            {
                syslog(LOG_NOTICE, "Found that there should have been a shutdown.");
                wylacz_komputer(__LINE__);
            }
        }

        pid_nagrywaj = pid_innego_procesu_o_nazwie("nagrywaj");
        if(pid_nagrywaj>0){
            if(poprzedni_pid_nagrywaj != pid_nagrywaj)
            {
                syslog(LOG_NOTICE, "Found 'nagrywaj' %d with pid:%d", wczytano,pid_nagrywaj);
                if(!nagrywaj_ma_byc_wlaczane)//wyłączanie
                {
                    int kod = system("pkill --signal SIGINT nagrywanie");
                    syslog(LOG_NOTICE, "pkill --signal SIGINT nagrywanie:code %d",kod);
                }
            }
        }
        else{
            if(poprzedni_pid_nagrywaj != pid_nagrywaj)syslog(LOG_NOTICE, "Not found 'nagrywaj'.");
            if(!nie_rysuj)
            {
                printf("rysowanie\n");
                int kod1, kod2;
                kod1 = system("/home/ms/smacz/rejestrator/testIMU1");
                kod2 = system("/home/ms/smacz/rejestrator/testIMU2");
                if(kod1 != 0 || kod2 != 0)
                {
                    printf("rysowanie2\n");
                    //sprintf(buf, "IMU1/2:%d/%d ROZPIETE",kod1, kod2);
                    sprintf(buf, "IMU%s%s%s:BLAD (ROZPIETE)", ((kod1)?("1"):("")),((kod1 && kod2)?(","):("")),((kod2)?("2"):("")));
                    ekran_nagrywanie(buf,(char *)"");
                }
                else{
                    printf("rysowanie3\n");
                    if(nagrywaj_ma_byc_wlaczane){//a nie jest
                        printf("rysowanie4\n");
                        syslog(LOG_NOTICE, "Reviving nagrywanie...");
                        ekran_nagrywanie((char *)"wskrzeszanie...",(char *)"");
                        uruchom_nagrywaj();
                    }
                    else{
                        printf("rysowanie4\n");
                        if(wypisuj_wifi)
                        {
                            pokaz_nazwe_wifi();
                        }
                        else{
                            ekran_godzina();//godzina - gdy nagrywanie nie jest i nie ma być włączone oraz wifi nie ma byc pokazywane
                        }
                        }
                }
            }
        }
        poprzedni_pid_nagrywaj = pid_nagrywaj;
        sleep(1);
    }

    syslog(LOG_NOTICE, "Smaczdaemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
