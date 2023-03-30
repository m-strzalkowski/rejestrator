#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "LSM9DS1_Types.h"
#include "LSM9DS1.h"
#include "rejestrator.h"

FILE * plik_nagrania = NULL;
FILE * plik_stanu = NULL;
int przeczytano = 0;
int zakoncz()
{
    if(plik_nagrania){fclose(plik_nagrania);}
    if(plik_stanu)
    {
        plik_stanu = freopen(NULL, "w", plik_stanu);
        if(!plik_stanu){return EXIT_FAILURE;}
        fprintf(plik_stanu,"%d", przeczytano);
        fclose(plik_stanu);
    }
    return EXIT_SUCCESS;
}
void sig_handler(int sig) {
    switch (sig) {
    case SIGINT:
        fprintf(stderr, "Received SIGINT...\n");
        exit(zakoncz());
    case SIGTERM:
        fprintf(stderr, "Received SIGTERM...\n");
        exit(zakoncz());
    default:
        fprintf(stderr, "wasn't expecting that!\n");
        abort();
    }
}
#define PRZYROSTEK_STANU "_pozycja"
int czestotliwosc = 20;//Hz
int main(int argc, char *argv[]) {
    bool dalej=false;
    bool csv=true;
    bool zacznij_na_koncu = false;
    bool zacznij_na_poczatku = false;
    bool tylko_wyzeruj = false;
    fprintf(stdout,"t_sec,label, pax,pay,paz,pzx,pzy,pzz,pmx,pmy,pmz, bax,bay,baz,bzx,bzy,bzz,bmx,bmy,bmz\n");
    char * sciezka_zapisu = (char *)"dane/zapis";
    char * sciezka_stanu_eksportu_zapisu = (char *)"dane/zapis_pozycja";
    Odczyt odczyt;

    int opt;
    while ((opt = getopt(argc, argv, "chez0")) != -1) {
        switch (opt) {
        case 'c': dalej = true; break;
        case 'h': csv = false; break;
        case 'e': zacznij_na_koncu = true; break;
        case 'z': zacznij_na_poczatku = true; break;
        case '0': tylko_wyzeruj = true; break;
        default:
            fprintf(stderr, "Uzycie: %s [-c -h -e/-z] [plik wejsciowy]\n -c - Gdy dojdzie do konca pliku, czeka az pojawia sie kolejne dane\n -h format czytelniejszy dla ludzi, inaczej csv\n e - nie zapisuje pozycji do ktorej doszedl w pliku _pozycja i od razu zaczyna od konca pliku (uzyc z -c).\n -z zaczyna na poczatku pliku i wyklucza sie z -e", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if(optind < argc)
    {
        sciezka_zapisu = argv[optind];
    }
    if(zacznij_na_koncu && zacznij_na_poczatku)
    {
        fprintf(stderr, "Opcje -e i -z wyklucaja sie wzajemnie (zaczynanie na koncu i na poczatku pliku)\n");
        fprintf(stderr, "Uzycie: %s [-c -h -e/-z -0] [plik wejsciowy]\n -c - Gdy dojdzie do konca pliku, czeka az pojawia sie kolejne dane\n -h format czytelniejszy dla ludzi, inaczej csv\n e - nie zapisuje pozycji do ktorej doszedl w pliku _pozycja i od razu zaczyna od konca pliku (uzyc z -c).\n -z zaczyna na poczatku pliku i wyklucza sie z -e", argv[0]);
        exit(EXIT_FAILURE);
    }

    if(memchr(sciezka_zapisu, 0, 2048)==NULL){fprintf(stderr, "Strasznie dlugi ten argument: %s",sciezka_zapisu); exit(EXIT_FAILURE);}
    char * nowa_sc_stanu = (char *)alloca((strlen(sciezka_zapisu)+strlen(PRZYROSTEK_STANU)+1));
    strcpy(nowa_sc_stanu, sciezka_zapisu);
    sciezka_stanu_eksportu_zapisu = nowa_sc_stanu;
    strcat(sciezka_stanu_eksportu_zapisu, PRZYROSTEK_STANU);
    fprintf(stderr, "zapis: %s stan:%s\n", sciezka_zapisu, sciezka_stanu_eksportu_zapisu);
    plik_nagrania = fopen(sciezka_zapisu, "rb");
    if(!plik_nagrania) {plik_nagrania = fopen(sciezka_zapisu, "a+");}
    if(!plik_nagrania)
    {
        fprintf(stderr, "Blad otwierania pliku z zapisem: %s", sciezka_zapisu);
        exit(EXIT_FAILURE);
    }

    //if(tylko_wyzeruj)
    //{
                
    //}

    if(!zacznij_na_koncu)
    {
        plik_stanu = fopen(sciezka_stanu_eksportu_zapisu, "a+");
        fseek(plik_stanu, 0,SEEK_SET);
        if(plik_stanu && !zacznij_na_poczatku)
        {
            if(fscanf(plik_stanu,"%d",&przeczytano))
            {
                if(fseek(plik_nagrania, przeczytano, SEEK_SET) == 0)
                {
                    fprintf(stderr, "Przesunieto na pozycje %d, ktora byla zapisana w pliku %s", przeczytano,sciezka_stanu_eksportu_zapisu);
                }
                else{
                    fprintf(stderr, "Blad: Przesuniecie na pozycje %d, ktora byla zapisana w pliku %s sie nie udalo.", przeczytano,sciezka_stanu_eksportu_zapisu);
                    przeczytano=0; fseek(plik_nagrania, 0,SEEK_SET); fprintf(stderr,"zaczynanie od poczatku\n");
                    }
            }else{
                przeczytano=0; fseek(plik_nagrania, 0,SEEK_SET); fprintf(stderr,"zaczynanie od poczatku\n");
            }
            
        }else{
            if(!zacznij_na_poczatku)fprintf(stderr, "Blad otwierania pliku z pozycja do ktorej dotarl eksport ostatnim razem. %s\n", sciezka_zapisu);
            przeczytano=0; fseek(plik_nagrania, 0,SEEK_SET); fprintf(stderr,"zaczynanie od poczatku\n");
        }

    }
    else{//zacyznanie od konca (opcja - e/-z)
        fseek(plik_nagrania, 0,SEEK_END); fprintf(stderr,"zaczynanie na koncu (-e)\n");
        przeczytano = ftell(plik_nagrania);
    }

    signal(SIGTERM,sig_handler);
    signal(SIGINT,sig_handler);//zalozenie pulapki na ctrl+c 

    int status, status_wiersza;//ile przeczytano pojedynczym read i ile z danego wiersza(o dugoci struktury Odczyt).
    for (;;) {
        
	//chcemy przeczytac dokladnie sizeof(odczyt) bajtow, nie wiecej, nie mniej
        status_wiersza=0;
	    do{
            //fprintf(stderr,"*FR*");
            status = fread(&odczyt, 1,sizeof(odczyt)-status_wiersza,plik_nagrania);
	        //fprintf(stderr, "[%d]\n",status);
            if(status < (int)(sizeof(odczyt)))
            {
                if(feof(plik_nagrania) && status ==0)
                {
                    if(status == 0){fprintf(stderr, "eof\n"); break;}
                //    else{fprintf(stderr, "file corrupted: it has to be divisible into %d byte blocks.", sizeof(odczyt)); exit(EXIT_FAILURE);}
                }
                if(status<=0)
                {
                    fprintf(stderr, "0 bytes read\n");
                    usleep(1000000/20);
                }
                else{
                    status_wiersza +=status;
                    fprintf(stderr, " %d/%d bytes read\n", status_wiersza, sizeof(odczyt));
                }
            }
            else{status_wiersza +=status;}
        }while(status_wiersza<(int)(sizeof(odczyt)));
        //printf("\nST:%d\n",status_wiersza);
	    przeczytano += status_wiersza;//sizeof(odczyt);
        
        if(feof(plik_nagrania))//plik sie skonczyl
        {
            if(!dalej)
            {
                fprintf(stderr, "koniec");
                exit(zakoncz());
            }
            else{
                fprintf(stderr,"\n!waiting (%ld)...!\n",ftell(plik_nagrania));
                usleep(1000000/20);//powinno byc poll(...)
		        //fclose(plik_nagrania);
                plik_nagrania = freopen(NULL, "r", plik_nagrania);//probojemy otworzyc ponownie plik.
                if(!plik_nagrania){fprintf(stderr,"nie udalo sie ponownie otworzyc pliku z zapisem."); zakoncz(); exit(EXIT_FAILURE);}
                if(fseek(plik_nagrania, przeczytano, SEEK_SET) == 0)
                {
                    fprintf(stderr, "Przesunieto na pozycje %d (%s)", przeczytano,sciezka_stanu_eksportu_zapisu);
                 }
                else{
                    fprintf(stderr, "Blad: Przesuniecie na pozycje %d w pliku %s po jego ponownym otwarciu sie nie udalo.Konczenie.\n", przeczytano,sciezka_stanu_eksportu_zapisu);
                    zakoncz(); exit(EXIT_FAILURE);
                }
		        continue;//nie chcemy wypisywac tego samego wiersza danych na wyjscie
            }
        }
/*	if(feof(zapis) && !dalej)
	{
                fprintf(stderr, "koniec");
                exit(zakoncz());
	}*/
        wypisz_odczyt(stdout,&odczyt,csv);

    }

    exit(EXIT_SUCCESS);
}
