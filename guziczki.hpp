void ustawWiringPi(void);

void ustawPinyDuzych(void);
const char * symbolOstatniegoDuzego();
int ostatni_duzy(void);
//https://forums.raspberrypi.com/viewtopic.php?t=104814
/*class Przelacznik
{
    private:
    int pin;
    struct timeval ostatnie;
    void (*gdy_niski)(void) = NULL;
    void (*gdy_wysoki)(void) = NULL;
    void rozdzielacz(void);
    public:
    void ustawPin(int pin,void(*gdy_niski)(void), void(*gdy_wysoki)(void));
};*/

void ustawPinDuzegoPrzelacznika(void(*gdy_niski)(void), void(*gdy_wysoki)(void));
void ustawPinMalegoPrzelacznika(void(*gdy_niski)(void), void(*gdy_wysoki)(void));