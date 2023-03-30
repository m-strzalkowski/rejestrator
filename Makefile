CC = gcc
CFLAGS = -O2 -g -Wall
.PHONY: all
all: ssd1306_i2c.o rejestrator.o guziczki.o ekran.o smaczdemon nagrywaj eksportuj test_guziczkow test_ekranu

ssd1306_i2c.o: ssd1306_i2c.c
	$(CC) -Wall -O2 -c ssd1306_i2c.c -o ssd1306_i2c.o -llsm9ds1
#	ar cr ssd1306_i2c.a ssd1306_i2c.o
#Funkcje do odczytu z IMU
rejestrator.o: rejestrator.cpp
	$(CC) -Wall -O2 -c rejestrator.cpp -o rejestrator.o -llsm9ds1

#Funkcje do ustawiania obwodow i czytania odpowiednich GPIO
guziczki.o: guziczki.cpp
	$(CC) $(CFLAGS) -O2 -c guziczki.cpp -lwiringPi

#Funcje do obslugi ekranu
ekran.o: ekran.cpp
#	$(CC) -Wall -O2 -c ekran.cpp ssd1306_i2c.c -lwiringPi
	$(CC) $(CFLAGS) -O2 -c ekran.cpp -lwiringPi


#Demon chodzacyw tle i zapewniajacy wlaczanie nagrywaj
smaczdemon: smaczdemon.cpp ekran.o guziczki.o ssd1306_i2c.o
	$(CC) $(CFLAGS) -c smaczdemon.cpp
	$(CC) $(CFLAGS) smaczdemon.o guziczki.o ekran.o ssd1306_i2c.o -o smaczdemon -lwiringPi -llsm9ds1

#Wlasciwy program nagrywajacy pomiary
nagrywaj: nagrywaj.cpp rejestrator.o guziczki.o ekran.o ssd1306_i2c.o
#	$(CC) -Wall -O2 -g nagrywaj.cpp guziczki.o rejestrator.o ekran.cpp ssd1306_i2c.c -o nagrywaj -llsm9ds1 -lwiringPi
	$(CC) $(CFLAGS) -c  nagrywaj.cpp -llsm9ds1 -lwiringPi
	$(CC) $(CFLAGS) nagrywaj.o rejestrator.o guziczki.o ekran.o ssd1306_i2c.o -o nagrywaj -llsm9ds1 -lwiringPi

#Eksporter pomiarow
eksportuj: eksportuj.cpp rejestrator.o
	$(CC) $(CFLAGS) -c eksportuj.cpp -o eksportuj.o -llsm9ds1
	$(CC) $(CFLAGS) eksportuj.o rejestrator.o -o eksportuj -llsm9ds1

#Test dzialania guziczkow
test_guziczkow: test_guziczkow.cpp guziczki.o ekran.o ssd1306_i2c.o
	$(CC) $(CFLAGS) -c test_guziczkow.cpp 
	$(CC) $(CFLAGS) test_guziczkow.o guziczki.o ekran.o ssd1306_i2c.o -o test_guziczkow -lwiringPi

test_ekranu: test_ekranu.cpp ekran.o ssd1306_i2c.o
	$(CC) $(CFLAGS) -c test_ekranu.cpp
	$(CC) $(CFLAGS) test_ekranu.o ekran.o ssd1306_i2c.o -o test_ekranu -lwiringPi
clean:
	rm -f rejestrator ekran test_ekranu smaczdemon nagrywaj eksportuj guziczki test_guziczkow
