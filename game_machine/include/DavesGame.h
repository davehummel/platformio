#ifndef DAVESGAME_H
#define DAVESGAME_H

#include <Arduino.h>
#include <SmartMatrix3.h>
#include <stdlib.h>
#include <Cartridge.h>



class DavesGame:public Cartridge {
    public:

    const char* getName(){
        return "Daves Game";
    }

    uint16_t getRefreshRate(){
        return 60;
    }


    protected:

    void setup(){
        Serial.println("Empty Setup() in cartridge");
    }

    uint8_t getGameId(){
        return 12;
    }

    void draw(bool up,bool right,bool down, bool left,bool white,bool blue,bool red){
        background->drawTriangle(x,y-1,x-1,y+1,x+1,y+1,{100,200,220});
        if (up)
            y=y-1;
        if (down)
            y=y+1;
        if (left)
            x = x-1;
        if (right)
            x = x+1;

        if (x < 0)
            x = 63;
        else if (x>63)
            x = 0;
        
        if (y < 0)
            y = 63;
        else if (y >63)
            y = 0;

        if (red){
            score++;
            setTopScore(score);
        }

        if (blue){
            background->fillScreen({0,0,0});
        }

    }

    private:

    int8_t x=32,y=32;
    uint32_t score = 0;

};
#endif