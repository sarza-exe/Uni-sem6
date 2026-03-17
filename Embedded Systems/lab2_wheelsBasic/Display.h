/* 
 * prosta implementacja klasy obsługującej 
 * silniki pojazdu za pośrednictwem modułu L298
 *
 * Sterowanie odbywa się przez:
 * 1)  powiązanie odpowiednich pinów I/O Arduino metodą attach() 
 * 2)  ustalenie prędkości setSpeed*()
 * 3)  wywołanie funkcji ruchu
 *
 * TODO:
 *  - zabezpieczenie przed ruchem bez attach()
 *  - ustawienie domyślnej prędkości != 0
 */


#include <Arduino.h>


#ifndef Display_h
#define Display_h



class Display {
    public: 
        Display();
        void init();
        void draw(int value);
        void left();
        void right();
        void stop();

    // private: 
    //     int pinsRight[3];
    //     int pinsLeft[3];
};



#endif
