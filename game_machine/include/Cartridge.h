#ifndef CATRIDGE_H
#define CATRIDGE_H

#include <Arduino.h>
#include <SmartMatrix3.h>
#include <stdlib.h>
#include <EEPROM.h>
#include <inttypes.h>



const rgb24 defaultBackgroundColor{0,0,0};
const rgb24 defaultForegroundColor{255,255,255};


class Cartridge {
    public:

    void start(SMLayerBackground<rgb24,0U> *p_background){
        ended = false;
        background = p_background;
        setup();
        background->fillScreen(defaultBackgroundColor);
        background->swapBuffers();
    }

    void render(){
        bool up, right, down, left, white, blue, red;

        up = digitalRead(UP_PIN);
        down = digitalRead(DOWN_PIN);
        left = digitalRead(LEFT_PIN);
        right = digitalRead(RIGHT_PIN);
        blue = digitalRead(BLUE_PIN);
        red = digitalRead(RED_PIN);
        white = digitalRead(WHITE_PIN);
        
        renderCount++;
        frames++;

        if (sincePrint>=1000){
            fps = renderCount;
            renderCount = 0;
            sincePrint = 0;
            Serial.println(fps);
        }

        if (blue & red & white & up){
            ended = true;

            background->fillScreen({200,0,0});
            background->swapBuffers();

            while (digitalRead(UP_PIN)||digitalRead(RED_PIN)||digitalRead(BLUE_PIN)||digitalRead(WHITE_PIN))
                delay(10);

            writeTopScoreData();
        }
       
        if (newTopScore){
            setTopScoreName(up,right,down,left,white,blue,red);
            return;
        }

        if (ended){
            return;
        }
 
        draw (up,right,down,left,white,blue,red);

        background->swapBuffers();
    }

    virtual const char* getName() = 0;

    virtual uint16_t getRefreshRate() = 0;

    bool isEnded(){
        return ended;
    }

    char* getTopScoreName(){
        return topScoreName;
    }

    uint32_t getTopScoreVal(){
        return topScoreVal;
    }

    void loadTopScore(){
        if (topScoreLoaded)
            return;
        else
            readTopScoreData();
    }


    protected:

    virtual void setup(){

    }

    virtual uint8_t getGameId(){
        return 255;
    }


    virtual void draw(bool up,bool right,bool down, bool left,bool white,bool blue,bool red){
        background->drawString(42,0,defaultForegroundColor,defaultBackgroundColor,print(getFPS()));
        background->drawString(0,0,defaultForegroundColor,defaultBackgroundColor,getName());
        if (up) background->drawChar(0,10,defaultForegroundColor,'U');
        if (right) background->drawChar(8,10,defaultForegroundColor,'R');
        if (down) background->drawChar(16,10,defaultForegroundColor,'D');
        if (left) background->drawChar(24,10,defaultForegroundColor,'L');
        if (white)  background->drawChar(32,10,defaultForegroundColor,'w');
        if (red) background->drawChar(40,10,defaultForegroundColor,'r');
        if (blue) background->drawChar(48,10,defaultForegroundColor,'b');
    }

        char* print(uint32_t val){
        sprintf(printBuffer, "%"PRIu32"",val);
        return printBuffer;
    }

    char* print(int val){
        sprintf(printBuffer,"%i",val);
        return printBuffer;
    }

    char* print(float val){
        sprintf(printBuffer,"%f",val);
        return printBuffer;
    }

    char* print(double val){
        sprintf(printBuffer,"%f",val);
        return printBuffer;
    }

    uint16_t getFPS(){
        return fps;
    }

    uint32_t getFrames(){
        return frames;
    }

    void setTopScore(uint32_t score){
        topScoreVal = score;
        newTopScore = true;
        for (uint8_t i = 0 ; i < 10 ; i++){
            topScoreName[i] = ' ';
        }
        while (digitalRead(UP_PIN)||digitalRead(RED_PIN)||digitalRead(BLUE_PIN)||digitalRead(WHITE_PIN))
            delay(10);
    }
  
   SMLayerBackground<rgb24,0U> *background;

    private:

    void readTopScoreData(){
        uint8_t gameId = getGameId();
        for (uint8_t i = 0 ; i < 10 ; i++){
            topScoreName[i]=EEPROM.read(gameId*14+i);
            if (topScoreName[i] == 0)
                topScoreName[i] = ' ';
        }
        topScoreName[10] = '\0';

        topScoreVal = EEPROM.read(gameId*14+10);
        topScoreVal |= EEPROM.read(gameId*14+11) << 8;
        topScoreVal |= EEPROM.read(gameId*14+12) << 16;
        topScoreVal |= EEPROM.read(gameId*14+13) << 24;
        topScoreLoaded = true;
    }

    uint8_t topScoreCursor = 0;
    void setTopScoreName(bool up,bool right,bool down, bool left,bool white,bool blue,bool red){
        uint8_t r = frames;
        uint8_t b = frames+128;
        uint8_t g = frames+256;

        background->fillScreen(defaultBackgroundColor);
        background->drawString(0,1,{b,r,g},"Top Score!");
        background->drawString(1,15,{r,b,g},print(topScoreVal));

        background->fillRectangle(topScoreCursor*6,30,topScoreCursor*6+5,38,{frames/10%2*255,0,0});
        background->drawString(0,30,{0,0,255},topScoreName);

        if (left){
            if (topScoreCursor == 0)
                topScoreCursor = 9;
            else 
                topScoreCursor--;
            delay(200);
        } else if (right){
            if (topScoreCursor == 9)
                topScoreCursor = 0;
            else 
                topScoreCursor++;
            delay(200);
        } else if (down){
            if (topScoreName[topScoreCursor] == 'A'){
                topScoreName[topScoreCursor] = ' ';
            } else if (topScoreName[topScoreCursor] == ' ') {
                topScoreName[topScoreCursor] = '9';
            } else if (topScoreName[topScoreCursor] == '0') {
                topScoreName[topScoreCursor] = 'Z';
            } else {
                topScoreName[topScoreCursor]--;
            }
            delay(150);
        } else if (up){
            if (topScoreName[topScoreCursor] == ' '){
                topScoreName[topScoreCursor] = 'A';
            } else if (topScoreName[topScoreCursor] == '9') {
                topScoreName[topScoreCursor] = ' ';
            } else if (topScoreName[topScoreCursor] == 'Z') {
                topScoreName[topScoreCursor] = '0';
            } else {
                topScoreName[topScoreCursor]++;
            }
            delay(150);
        } else if (white || blue || red){
            writeTopScoreData();
            newTopScore = false;
        }
            

        background->swapBuffers();
    }

    void writeTopScoreData(){
        uint8_t gameId = getGameId();
        bool pastEndOfString = false;
        for (uint8_t i = 0 ; i < 10 ; i++){

            if (topScoreName[i] == '\0')
                pastEndOfString = true;

            if (pastEndOfString){
                EEPROM.write(gameId*14+i,' ');
            }else{
                EEPROM.write(gameId*14+i,topScoreName[i]);
            }
        }

        EEPROM.write(gameId*14+10, topScoreVal & 0xFF);
        EEPROM.write(gameId*14+11, (topScoreVal) >>8  & 0xFF);
        EEPROM.write(gameId*14+12, (topScoreVal) >>16  & 0xFF);
        EEPROM.write(gameId*14+13, (topScoreVal) >>24  & 0xFF);

    }

    bool ended = true;
    bool newTopScore = false;
    bool topScoreLoaded = false;
    elapsedMillis sincePrint;
    uint16_t renderCount = 0;
    uint16_t fps = 0;
    uint32_t frames = 0;
    char printBuffer[17];

    char topScoreName[11] = {'\0'};
    uint32_t topScoreVal;

};

#endif