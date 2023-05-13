#include "ssd1306_i2c.hpp"//kopia headera dla C++
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "ekran.hpp"
//static int inicjalizowany=0;
void inicjuj(void)
{
    ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
}
void ekran_godzina(void)
{
    printf("Godzina!");
    char buf[2048] = {0};
    time_t czas;
    struct tm * timeinfo;
    static void * skok = &&pierw;
    goto *skok;
    pierw:
    skok = &&kolejny;
    inicjuj();
    kolejny:

    time(&czas);
    timeinfo = localtime(&czas);
    char * repr = asctime(timeinfo);
    repr[strlen(repr)-1]='\0';//nie chcemy \n
    sprintf(buf, "%s,", repr);
    ssd1306_clearDisplay();
    ssd1306_drawString(buf);
    ssd1306_display();
    return;
}
void ekran_nagrywanie(char * etykieta, char * napis)
{
    static int pierwszy = 1;
    char buf[2048] = {0};
    if(pierwszy){inicjuj(); pierwszy=0;}
    sprintf(buf, "%s\n%s",etykieta, napis);
    ssd1306_clearDisplay();
    ssd1306_drawString(buf);
    ssd1306_display();
}
void odwroc_ekran(void)
{
    static int i=0;
    ssd1306_invertDisplay(i=!i);
    return;
}
void przekrec_ekran(void){rotate_screen();}
void rozmiar_tekstu(int s){ssd1306_setTextSize(s);}
