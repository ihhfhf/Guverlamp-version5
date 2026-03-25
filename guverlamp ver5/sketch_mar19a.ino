/*
   Управление кнопкой/сенсором
  - Удержание - яркость
  - 1х тап - вкл/выкл
  - 2х тап - переключ режима
  - 3х тап - вкл/выкл белый свет
  - 4х тап - старт/стоп авто смены режимов
*/


// ************************** НАСТРОЙКИ ***********************
#define CURRENT_LIMIT 2000  // лимит по току в миллиамперах, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит
#define AUTOPLAY_TIME 30    // время между сменой режимов в секундах

#define NUM_LEDS 60         // количсетво светодиодов в одном отрезке ленты
#define NUM_STRIPS 10        // количество отрезков ленты (в параллели)
#define LED_PIN 6           // пин ленты
#define BTN_PIN 13           // пин кнопки/сенсора
#define MIN_BRIGHTNESS 5  // минимальная яркость при ручной настройке
#define BRIGHTNESS 250      // начальная яркость
#define FIRE_PALETTE 0      // разные типы огня (0 - 3).

// ************************** ДЛЯ РАЗРАБОТЧИКОВ ***********************
#define MODES_AMOUNT 10      // УВЕЛИЧИЛ ДО 10 для новых эффектов
#include "GyverButton.h"
GButton touch(BTN_PIN, LOW_PULL, NORM_OPEN);

#include <FastLED.h>
CRGB leds[NUM_LEDS];
CRGBPalette16 gPal;

#include "GyverTimer.h"
GTimer_ms effectTimer(60);
GTimer_ms autoplayTimer((long)AUTOPLAY_TIME * 1000);
GTimer_ms brightTimer(20);

int brightness = BRIGHTNESS;
int tempBrightness;
byte thisMode;

bool gReverseDirection = false;
boolean loadingFlag = true;
boolean autoplay = true;
boolean powerDirection = true;
boolean powerActive = false;
boolean powerState = true;
boolean whiteMode = false;
boolean brightDirection = true;
boolean wasStep = false;


// залить все
void fillAll(CRGB newcolor) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = newcolor;
  }
}

// функция получения цвета пикселя по его номеру
uint32_t getPixColor(int thisPixel) {
  return (((uint32_t)leds[thisPixel].r << 16) | ((long)leds[thisPixel].g << 8 ) | (long)leds[thisPixel].b);
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  if (CURRENT_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT / NUM_STRIPS);
  FastLED.setBrightness(brightness);
  FastLED.show();

  randomSeed(analogRead(0));
  touch.setTimeout(300);
  touch.setStepTimeout(50);

  if (FIRE_PALETTE == 0) gPal = HeatColors_p;
  else if (FIRE_PALETTE == 1) gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);
  else if (FIRE_PALETTE == 2) gPal = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);
  else if (FIRE_PALETTE == 3) gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::White);
}

void loop() {
  touch.tick();
  if (touch.hasClicks()) {
    byte clicks = touch.getClicks();
    switch (clicks) {
      case 1:
        powerDirection = !powerDirection;
        powerActive = true;
        tempBrightness = brightness * !powerDirection;
        break;
      case 2: if (!whiteMode && !powerActive) {
          nextMode();
        }
        break;
      case 3: if (!powerActive) {
          whiteMode = !whiteMode;
          if (whiteMode) {
            effectTimer.stop();
            fillAll(CRGB::White);
            FastLED.show();
          } else {
            effectTimer.start();
          }
        }
        break;
      case 4: if (!whiteMode && !powerActive) autoplay = !autoplay;
        break;
      default:
        break;
    }
  }

  if (touch.isStep()) {
    if (!powerActive) {
      wasStep = true;
      if (brightDirection) {
        brightness += 5;
      } else {
        brightness -= 5;
      }
      brightness = constrain(brightness, MIN_BRIGHTNESS, 255);
      FastLED.setBrightness(brightness);
      FastLED.show();
    }
  }

  if (touch.isRelease()) {
    if (wasStep) {
      wasStep = false;
      brightDirection = !brightDirection;
    }
  }

  if (effectTimer.isReady() && powerState) {
    switch (thisMode) {
      case 0: lighter();
        break;
      case 1: lightBugs();
        break;
      case 2: colors();
        break;
      case 3: rainbow();
        break;
      case 4: sparkles();
        break;
      case 5: fire();
        break;
      case 6: twinkles();           // ЗВЕЗДОПАД
        break;
      case 7: rainbowSnake();       // РАДУЖНАЯ ЗМЕЙКА
        break;
      case 8: firework();           // НОВЫЙ ЭФФЕКТ - САЛЮТ
        break;
      case 9: runningRainbow();     // НОВЫЙ ЭФФЕКТ - БЕГУЩАЯ РАДУГА
        break;
    }
    FastLED.show();
  }

  if (autoplayTimer.isReady() && autoplay) {    // таймер смены режима
    nextMode();
  }

  brightnessTick();
}

void nextMode() {
  thisMode++;
  if (thisMode >= MODES_AMOUNT) thisMode = 0;
  loadingFlag = true;
  FastLED.clear();
}

void brightnessTick() {
  if (powerActive) {
    if (brightTimer.isReady()) {
      if (powerDirection) {
        powerState = true;
        tempBrightness += 10;
        if (tempBrightness > brightness) {
          tempBrightness = brightness;
          powerActive = false;
        }
        FastLED.setBrightness(tempBrightness);
        FastLED.show();
      } else {
        tempBrightness -= 10;
        if (tempBrightness < 0) {
          tempBrightness = 0;
          powerActive = false;
          powerState = false;
        }
        FastLED.setBrightness(tempBrightness);
        FastLED.show();
      }
    }
  }
}
#define TRACK_STEP 50

// ****************************** ОГОНЁК ******************************
int16_t position;
boolean direction;

void lighter() {
  FastLED.clear();
  if (direction) {
    position++;
    if (position > NUM_LEDS - 2) {
      direction = false;
    }
  } else {
    position--;
    if (position < 1) {
      direction = true;
    }
  }
  leds[position] = CRGB::White;
}

// ****************************** СВЕТЛЯЧКИ ******************************
#define MAX_SPEED 30
#define BUGS_AMOUNT 3
int16_t speed[BUGS_AMOUNT];
int16_t pos[BUGS_AMOUNT];
CRGB bugColors[BUGS_AMOUNT];

void lightBugs() {
  if (loadingFlag) {
    loadingFlag = false;
    for (int i = 0; i < BUGS_AMOUNT; i++) {
      bugColors[i] = CHSV(random(0, 9) * 28, 255, 255);
      pos[i] = random(0, NUM_LEDS);
      speed[i] += random(-5, 6);
    }
  }
  FastLED.clear();
  for (int i = 0; i < BUGS_AMOUNT; i++) {
    speed[i] += random(-5, 6);
    if (speed[i] == 0) speed[i] += (-5, 6);

    if (abs(speed[i]) > MAX_SPEED) speed[i] = 0;
    pos[i] += speed[i] / 10;
    if (pos[i] < 0) {
      pos[i] = 0;
      speed[i] = -speed[i];
    }
    if (pos[i] > NUM_LEDS - 1) {
      pos[i] = NUM_LEDS - 1;
      speed[i] = -speed[i];
    }
    leds[pos[i]] = bugColors[i];
  }
}

// ****************************** ЦВЕТА ******************************
byte hue;
void colors() {
  hue += 2;
  CRGB thisColor = CHSV(hue, 255, 255);
  fillAll(CHSV(hue, 255, 255));
}

// ****************************** РАДУГА ******************************
void rainbow() {
  hue += 2;
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CHSV((byte)(hue + i * float(255 / NUM_LEDS)), 255, 255);
}

// ****************************** КОНФЕТТИ ******************************
void sparkles() {
  byte thisNum = random(0, NUM_LEDS);
  if (getPixColor(thisNum) == 0)
    leds[thisNum] = CHSV(random(0, 255), 255, 255);
  fade();
}

// ****************************** ОГОНЬ ******************************
#define COOLING  55
// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

void fire() {
  random16_add_entropy( random());
  Fire2012WithPalette(); // run simulation frame, using palette colors
}

void Fire2012WithPalette()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    // Scale the heat value from 0-255 down to 0-240
    // for best results with color palettes.
    byte colorindex = scale8( heat[j], 240);
    CRGB color = ColorFromPalette( gPal, colorindex);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
}

// ****************************** НОВЫЙ КРАСИВЫЙ ЗВЕЗДОПАД ******************************
#define FALLING_STARS 3        // количество падающих звезд одновременно
#define TWINKLE_STARS 8        // количество мерцающих звезд
#define TWINKLE_SPEED 40       // скорость мерцания

// структура для падающей звезды
struct FallingStar {
  int position;        // текущая позиция
  int tailLength;      // длина хвоста
  byte brightness;     // яркость
  byte hue;           // цвет (золотистый/белый)
  bool active;        // активна ли
};

// структура для мерцающей звезды
struct TwinklingStar {
  int position;        // позиция
  byte brightness;     // текущая яркость
  byte targetBright;   // целевая яркость
  byte phase;          // фаза мерцания
  byte hue;           // цвет (теплый/холодный)
};

FallingStar fallingStars[FALLING_STARS];
TwinklingStar twinklingStars[TWINKLE_STARS];

void starsInit() {
  // инициализация падающих звезд
  for (int i = 0; i < FALLING_STARS; i++) {
    fallingStars[i].active = false;
  }
  
  // инициализация мерцающих звезд
  for (int i = 0; i < TWINKLE_STARS; i++) {
    twinklingStars[i].position = random(0, NUM_LEDS);
    twinklingStars[i].brightness = random(30, 200);
    twinklingStars[i].targetBright = random(100, 255);
    twinklingStars[i].phase = random(0, 3);
    // случайный цвет: теплый (0-30) или холодный (160-200)
    twinklingStars[i].hue = random(0, 2) == 0 ? random(20, 40) : random(160, 200);
  }
}

void twinkles() {
  if (loadingFlag) {
    loadingFlag = false;
    starsInit();
  }
  
  FastLED.clear();
  
  // ********** МЕРЦАЮЩИЕ ЗВЕЗДЫ (фон) **********
  for (int i = 0; i < TWINKLE_STARS; i++) {
    switch(twinklingStars[i].phase) {
      case 0: // рост яркости
        twinklingStars[i].brightness += 2;
        if (twinklingStars[i].brightness >= twinklingStars[i].targetBright) {
          twinklingStars[i].phase = 1;
          twinklingStars[i].targetBright = random(30, 100);
        }
        break;
        
      case 1: // падение яркости
        twinklingStars[i].brightness -= 2;
        if (twinklingStars[i].brightness <= twinklingStars[i].targetBright) {
          twinklingStars[i].phase = 2;
          twinklingStars[i].targetBright = random(100, 200);
        }
        break;
        
      case 2: // мерцание (случайные вспышки)
        if (random(0, 20) == 0) {
          twinklingStars[i].brightness = random(150, 255);
        }
        if (random(0, 50) == 0) {
          twinklingStars[i].phase = random(0, 2);
        }
        break;
    }
    
    // ограничиваем яркость
    twinklingStars[i].brightness = constrain(twinklingStars[i].brightness, 20, 255);
    
    // рисуем звезду
    CRGB color = CHSV(twinklingStars[i].hue, 200, twinklingStars[i].brightness);
    leds[twinklingStars[i].position] = color;
    
    // изредка меняем позицию звезды
    if (random(0, 1000) == 0) {
      twinklingStars[i].position = random(0, NUM_LEDS);
    }
  }
  
  // ********** ПАДАЮЩИЕ ЗВЕЗДЫ **********
  // активируем новые падающие звезды
  for (int i = 0; i < FALLING_STARS; i++) {
    if (!fallingStars[i].active && random(0, 50) == 0) {
      fallingStars[i].active = true;
      fallingStars[i].position = 0;
      fallingStars[i].tailLength = random(3, 6); // длина хвоста 3-5 пикселей
      fallingStars[i].brightness = 255;
      fallingStars[i].hue = random(20, 40); // золотистые тона
    }
  }
  
  // обновляем и рисуем падающие звезды
  for (int i = 0; i < FALLING_STARS; i++) {
    if (fallingStars[i].active) {
      // рисуем хвост
      for (int j = 0; j < fallingStars[i].tailLength; j++) {
        int pos = fallingStars[i].position - j;
        if (pos >= 0 && pos < NUM_LEDS) {
          // яркость уменьшается к концу хвоста
          byte tailBright = fallingStars[i].brightness * (fallingStars[i].tailLength - j) / fallingStars[i].tailLength;
          // цвет немного меняется по хвосту
          byte tailHue = fallingStars[i].hue + j * 5;
          leds[pos] = CHSV(tailHue, 255, tailBright);
        }
      }
      
      // движение звезды
      fallingStars[i].position += 2;
      
      // деактивируем, если упала
      if (fallingStars[i].position >= NUM_LEDS + fallingStars[i].tailLength) {
        fallingStars[i].active = false;
      }
    }
  }
  
  // добавляем редкие яркие вспышки (падающие звезды с большим хвостом)
  if (random(0, 200) == 0) {
    int pos = 0;
    for (int j = 0; j < 8; j++) {
      int p = pos - j;
      if (p >= 0 && p < NUM_LEDS) {
        leds[p] = CHSV(30, 255, 255 - j * 30);
      }
    }
  }
  
  FastLED.show();
  delay(TWINKLE_SPEED);
}

// ****************************** РАДУЖНАЯ ЗМЕЙКА ******************************
#define SNAKE_LENGTH 8          // длина змейки
#define SNAKE_SPEED 30          // скорость движения
int snakeHead = 0;
int snakeDir = 1;
byte snakeHue = 0;
byte snakeTrail[SNAKE_LENGTH];  // запоминаем позиции хвоста

void rainbowSnake() {
  if (loadingFlag) {
    loadingFlag = false;
    snakeHead = random(0, NUM_LEDS);
    snakeDir = random(0, 2) * 2 - 1; // 1 или -1
    for (int i = 0; i < SNAKE_LENGTH; i++) {
      snakeTrail[i] = -1;
    }
  }
  
  // сдвигаем историю хвоста
  for (int i = SNAKE_LENGTH - 1; i > 0; i--) {
    snakeTrail[i] = snakeTrail[i-1];
  }
  snakeTrail[0] = snakeHead;
  
  FastLED.clear();
  
  // рисуем змейку
  snakeHue += 3;
  
  for (int i = 0; i < SNAKE_LENGTH; i++) {
    int pos = snakeTrail[i];
    if (pos >= 0 && pos < NUM_LEDS) {
      // цвет меняется по длине змейки
      byte hue = snakeHue + i * 15;
      // яркость уменьшается к хвосту
      byte bright = 255 - (i * 30);
      leds[pos] = CHSV(hue, 255, bright);
    }
  }
  
  // движение змейки
  snakeHead += snakeDir;
  
  // отскок от границ
  if (snakeHead >= NUM_LEDS) {
    snakeHead = NUM_LEDS - 1;
    snakeDir = -snakeDir;
  }
  if (snakeHead < 0) {
    snakeHead = 0;
    snakeDir = -snakeDir;
  }
  
  FastLED.show();
  delay(SNAKE_SPEED);
}

// ****************************** НОВЫЙ ЭФФЕКТ - САЛЮТ ******************************
#define FUSE_SPEED 2           // скорость запала
#define EXPLOSION_DURATION 25  // длительность взрыва (увеличил)
int fusePos = 0;
bool explosionActive = false;
int explosionTimer = 0;
byte explosionColors[NUM_LEDS];
byte explosionBrightness[NUM_LEDS];

void firework() {
  if (loadingFlag) {
    loadingFlag = false;
    fusePos = 0;
    explosionActive = false;
    explosionTimer = 0;
    FastLED.clear();
  }
  
  FastLED.clear();
  
  if (!explosionActive) {
    // ОРАНЖЕВЫЙ ЗАПАЛ (искра с хвостиком)
    for (int i = 0; i < 6; i++) {
      int pos = fusePos - i;
      if (pos >= 0 && pos < NUM_LEDS) {
        // Оранжевый градиент
        byte bright = 255 - i * 40;
        if (i == 0) {
          leds[pos] = CRGB(255, 200, 100);  // яркий оранжевый
        } else if (i == 1) {
          leds[pos] = CRGB(255, 150, 50);   // оранжевый
        } else if (i == 2) {
          leds[pos] = CRGB(200, 100, 0);    // темно-оранжевый
        } else {
          leds[pos] = CRGB(100, 50, 0);      // красновато-коричневый
        }
      }
    }
    
    // Добавляем искры вокруг запала
    for (int i = 0; i < 3; i++) {
      int sparkPos = fusePos + random(-2, 3);
      if (sparkPos >= 0 && sparkPos < NUM_LEDS) {
        leds[sparkPos] = CRGB(255, 255, 200); // бело-желтые искры
      }
    }
    
    // Движение запала
    fusePos += FUSE_SPEED;
    
    // Если достиг конца - взрыв
    if (fusePos >= NUM_LEDS) {
      explosionActive = true;
      explosionTimer = 0;
      
      // Генерируем случайные цвета и яркости для взрыва
      for (int i = 0; i < NUM_LEDS; i++) {
        explosionColors[i] = random(0, 255); // случайный оттенок
        explosionBrightness[i] = random(200, 255); // случайная яркость
      }
    }
  } else {
    // ОБЪЕМНЫЙ ВЗРЫВ
    explosionTimer++;
    
    // Разные фазы взрыва
    if (explosionTimer < 5) {
      // Фаза 1: Яркая вспышка в центре
      int center = NUM_LEDS / 2;
      for (int i = max(0, center - 5); i < min(NUM_LEDS, center + 5); i++) {
        leds[i] = CRGB(255, 255, 255); // белая вспышка
      }
    } 
    else if (explosionTimer < 10) {
      // Фаза 2: Разлет цветных искр
      for (int i = 0; i < NUM_LEDS; i++) {
        if (random(0, 3) == 0) {
          byte hue = (explosionColors[i] + explosionTimer * 5) % 255;
          leds[i] = CHSV(hue, 255, 255);
        }
      }
    }
    else if (explosionTimer < 20) {
      // Фаза 3: Основной взрыв - все цвета радуги с пульсацией
      for (int i = 0; i < NUM_LEDS; i++) {
        byte hue = (explosionColors[i] + explosionTimer * 3) % 255;
        
        // Эффект пульсации
        float pulse = sin((float)explosionTimer * 0.5) * 0.3 + 0.7;
        byte bright = 255 * pulse;
        
        // Добавляем случайные яркие точки
        if (random(0, 10) == 0) {
          bright = 255;
        }
        
        leds[i] = CHSV(hue, 255, bright);
      }
      
      // Добавляем "звездочки" - более яркие точки
      for (int j = 0; j < 10; j++) {
        int pos = random(0, NUM_LEDS);
        leds[pos] = CRGB(255, 255, 255);
      }
    }
    else if (explosionTimer < 25) {
      // Фаза 4: Затухание с мерцанием
      for (int i = 0; i < NUM_LEDS; i++) {
        if (random(0, 5) == 0) {
          byte hue = (explosionColors[i] + explosionTimer * 2) % 255;
          byte bright = 150 - (explosionTimer - 20) * 30;
          leds[i] = CHSV(hue, 255, max(0, bright));
        }
      }
    }
    
    // Добавляем разлетающиеся искры во все стороны
    for (int i = 0; i < 15; i++) {
      int pos = random(0, NUM_LEDS);
      int dir = random(-2, 3);
      int sparkPos = pos + dir * (explosionTimer / 2);
      if (sparkPos >= 0 && sparkPos < NUM_LEDS) {
        byte hue = random(0, 255);
        byte bright = 255 - explosionTimer * 5;
        leds[sparkPos] = CHSV(hue, 255, max(0, bright));
      }
    }
    
    // Конец взрыва
    if (explosionTimer >= EXPLOSION_DURATION) {
      explosionActive = false;
      fusePos = 0; // готовим новый салют
    }
  }
  
  FastLED.show();
  delay(30);
}

// ****************************** НОВЫЙ ЭФФЕКТ - БЕГУЩАЯ РАДУГА ******************************
#define RUNNING_SPEED 20       // скорость бегущей радуги
#define RAINBOW_WIDTH 10       // ширина радужной полосы
int runningPos = 0;
byte runningHue = 0;

void runningRainbow() {
  if (loadingFlag) {
    loadingFlag = false;
    runningPos = 0;
    runningHue = 0;
  }
  
  FastLED.clear();
  
  // Бегущая радужная полоса
  for (int i = 0; i < NUM_LEDS; i++) {
    // Расчет расстояния до центра полосы
    int dist = abs(i - runningPos);
    
    if (dist < RAINBOW_WIDTH) {
      // Градиент радуги
      byte hue = runningHue + (i * 10); // меняем цвет по длине
      byte bright = 255 - (dist * (255 / RAINBOW_WIDTH));
      leds[i] = CHSV(hue, 255, bright);
    } else if (dist < RAINBOW_WIDTH + 5) {
      // Затухающий хвост
      byte bright = 50 - (dist - RAINBOW_WIDTH) * 10;
      if (bright > 0) {
        leds[i] = CHSV(runningHue + (i * 10), 255, bright);
      }
    }
  }
  
  // Движение полосы
  runningPos++;
  runningHue += 2;
  
  // Возврат в начало
  if (runningPos >= NUM_LEDS + RAINBOW_WIDTH) {
    runningPos = 0;
  }
  
  FastLED.show();
  delay(RUNNING_SPEED);
}

// ****************** СЛУЖЕБНЫЕ ФУНКЦИИ *******************
void fade() {
  for (int i = 0; i < NUM_LEDS; i++) {
    if ((uint32_t)getPixColor(i) == 0) continue;
    leds[i].fadeToBlackBy(TRACK_STEP);
  }
}