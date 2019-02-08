#include <Arduino.h>
/*
    SmartMatrix Features Demo - Louis Beaudoin (Pixelmatix)
    This example code is released into the public domain
*/
#define UP_PIN 36
#define DOWN_PIN 38
#define LEFT_PIN 37
#define RIGHT_PIN 39
#define BLUE_PIN 33
#define RED_PIN 35
#define WHITE_PIN 34

#include <SmartLEDShieldV4.h>  // comment out this line for if you're not using SmartLED Shield V4 hardware (this line needs to be before #include <SmartMatrix3.h>)
#include <SmartMatrix3.h>
#include <Cartridge.h>
#include <DavesGame.h>

#define GAME_SCROLL_SIZE 6
#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24
const uint8_t kMatrixWidth = 64;        // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 64;       // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
const uint8_t kPanelType = SMARTMATRIX_HUB75_64ROW_MOD32SCAN; // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels, or use SMARTMATRIX_HUB75_64ROW_MOD32SCAN for common 64x64 panels
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);


const int defaultBrightness = (100*255)/100;    // full (100%) brightness

const uint8_t gameCount = 1;
Cartridge* games[gameCount] = {new DavesGame()};

bool gameActive = false;

uint8_t selectionIndex = 0;
uint8_t selectionScroll = 0;


// the setup() method runs once, when the sketch starts
void setup() {

  Serial.begin(115200);
        pinMode(UP_PIN,INPUT_PULLDOWN);
        pinMode(DOWN_PIN,INPUT_PULLDOWN);
        pinMode(LEFT_PIN,INPUT_PULLDOWN);
        pinMode(RIGHT_PIN,INPUT_PULLDOWN);
        pinMode(RED_PIN,INPUT_PULLDOWN);
        pinMode(BLUE_PIN,INPUT_PULLDOWN);
        pinMode(WHITE_PIN,INPUT_PULLDOWN);
  Serial.println(matrix.getRefreshRate());
  matrix.setRefreshRate(60);
  matrix.addLayer(&backgroundLayer); 
  matrix.begin();

  matrix.setBrightness(defaultBrightness);

  backgroundLayer.enableColorCorrection(true);
  backgroundLayer.setFont(fontChoices::font6x10);

}

void startGame(){
     matrix.setRefreshRate(games[selectionIndex]->getRefreshRate());
     games[selectionIndex]->start(&backgroundLayer);
     gameActive = true;
}


void drawMenu(){
  
  backgroundLayer.fillScreen({0,0,0});

  if (digitalRead(WHITE_PIN)){
    while (digitalRead(WHITE_PIN))
      delay(10);
    startGame();
  }

  if (digitalRead(BLUE_PIN)){
    backgroundLayer.drawString(1,1,{200,200,200},games[selectionIndex]->getName());
    backgroundLayer.drawString(1,10,{200,200,0},(char*)F("Top Score"));
    games[selectionIndex]->loadTopScore();
    backgroundLayer.drawString(1,20,{255,10,10},games[selectionIndex]->getTopScoreName());
    char printBuffer[12];
    sprintf(printBuffer,"%Lu",games[selectionIndex]->getTopScoreVal());
    backgroundLayer.drawString(1,30,{20,240,210},printBuffer);
    backgroundLayer.swapBuffers();

    Serial.print(games[selectionIndex]->getTopScoreName());
    Serial.print(" ");
    Serial.println(games[selectionIndex]->getTopScoreVal());

    while (digitalRead(BLUE_PIN))
      delay(10);
  }

  if (digitalRead(UP_PIN)){
    if (selectionIndex == 0)
      selectionIndex = gameCount - 1;
    else
      selectionIndex--;
    while (digitalRead(UP_PIN))
      delay(10);
  }

  if (digitalRead(DOWN_PIN)){
    if (selectionIndex == gameCount - 1)
      selectionIndex = 0;
    else
      selectionIndex++;

    while (digitalRead(DOWN_PIN))
      delay(10);
  }

  if (selectionIndex - selectionScroll >= GAME_SCROLL_SIZE){
    selectionScroll = selectionIndex - GAME_SCROLL_SIZE + 1;
  } else if (selectionIndex - selectionScroll < 0){
    selectionScroll = selectionIndex;
  }

  for (uint8_t i = selectionScroll ; (i < selectionScroll + GAME_SCROLL_SIZE) && (i<gameCount) ; i++ ){
    if (i == selectionIndex)
      backgroundLayer.drawString(1,2+10*(i-selectionScroll),{250,30,255},{100,200,100},games[i]->getName()); 
    else
      backgroundLayer.drawString(1,2+10*(i-selectionScroll),{250,30,255},{0,0,0},games[i]->getName()); 
  }
  
  backgroundLayer.swapBuffers();
}

// the loop() method runs over and over again,
// as long as the board has power

void loop() {
  if (gameActive == true){
    if (games[selectionIndex]->isEnded()){
      gameActive = false;
    } else {
      games[selectionIndex]->render();
    }
  } else {
    drawMenu();
  }
}