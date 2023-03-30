#include "ssd1306_i2c.hpp"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "ekran.hpp"
#define SEC 1000000
//gcc proba.c ssd1306_i2c.c -lwiringPi -o proba

//int (*printwsk)() = printf;//nie dziala w C++
//int nic(){}
//#define printf printwsk

int debug=1;
int main(void)
{
    ekran_godzina();
    printf("TEST_EKRANU\n");
    //if(!debug){printwsk = nic;}
    char buf[4096]={0};
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);
    while(read(0,buf,1)<1)
    {
        usleep(SEC/2);
        ekran_godzina();
    }
    printf("EKRAN NAGRYWANIA\n");
    for(int i=0;i<1000000;i++)
    {
        sprintf(buf, "%d", i);
        ekran_nagrywanie("NIC", buf);
        printf("%d\n",i);
    }
}
/*
int main() {

	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
    ssd1306_display(); //Adafruit logo is visible
	ssd1306_clearDisplay();
	char buf[2048] = {0};
    for(int i=0;i<1000000;i++)
    {
        sprintf(buf, "%d", i);
        ssd1306_clearDisplay();
        ssd1306_drawString(buf);
        ssd1306_display();

    }

    while(1)
    {
        printf(">");
        scanf("%s",buf);
        printf("\n%s\n",buf);
        ssd1306_clearDisplay();
        ssd1306_drawString(buf);
        ssd1306_display();
    }
    return 0;
}
*/