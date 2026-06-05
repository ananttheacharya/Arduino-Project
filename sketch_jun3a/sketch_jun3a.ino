#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Definitions
const int PIN_D2 = 2; // Nav / Jump
const int PIN_D8 = 8; // Next / Simon
const int PIN_D6 = 6; // Custom / Simon
const int PIN_D5 = 5; // Custom / Simon
const int FLAME_SENSOR_PIN = 7;

struct Button {
  int pin;
  bool lastState;
  unsigned long pressTime;
  bool isPressed;
};

Button btnA = {PIN_D2, HIGH, 0, false};
Button btnB = {PIN_D8, HIGH, 0, false};
Button btnC = {PIN_D6, HIGH, 0, false};
Button btnD = {PIN_D5, HIGH, 0, false};

bool flameDetected = false;
const int LONG_PRESS_MS = 500;
const int DEBOUNCE_MS = 50;

// System States
bool isBatteryMode = true;
unsigned long lastDataTime = 0;
const unsigned long TIMEOUT_MS = 3000;

int pcScreen = 0;   // 0=Stats, 1=Spotify, 2=Lyrics, 3=Eyes
int battScreen = 0; // 0=Eyes, 1=Runner, 2=Simon, 3=Trivia

// PC Data
String sysLine1 = "Waiting for PC..";
String sysLine2 = "";
String spotifyText = "Connecting...   ";
int progressPct = 0;
String currentLyric = "";
int typeIndex = 0;
unsigned long lastTypeTime = 0;
unsigned long lastScroll = 0;
int scrollPos = 0;

int currentCharset = -1;

// ----- CUSTOM CHARACTERS (PROGMEM) -----
const byte prog_chars[5][8] PROGMEM = {
  {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10},
  {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18},
  {0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C},
  {0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E},
  {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F}
};

const byte ojo_abierto[8][8] PROGMEM = {
  {B01000,B00100,B00010,B10001,B01011,B00110,B01110,B11100},
  {B01000,B00111,B11111,B11100,B10001,B00111,B01111,B11111},
  {B00010,B11100,B11111,B00111,B10001,B11100,B11110,B11111},
  {B00010,B00100,B01000,B10001,B11010,B01100,B01110,B00111},
  {B11100,B01110,B00111,B00011,B00001,B00000,B00000,B00000},
  {B11111,B01111,B00111,B10001,B11100,B11111,B00111,B00000},
  {B11111,B11110,B11100,B10001,B00111,B11111,B11100,B00000},
  {B00111,B01110,B11100,B11000,B10000,B00000,B00000,B00000}
};

const byte ojo_cerrado[8][8] PROGMEM = {
  {B00000,B00000,B00000,B00001,B00011,B00111,B01111,B11111},
  {B00000,B00111,B11111,B11111,B11111,B11111,B11111,B11111},
  {B00000,B11100,B11111,B11111,B11111,B11111,B11111,B11111},
  {B00000,B00000,B00000,B10000,B11000,B11100,B11110,B11111},
  {B11111,B01111,B00111,B01011,B10001,B00010,B00100,B01000},
  {B11111,B11111,B11111,B11111,B11111,B11111,B00111,B01000},
  {B11111,B11111,B11111,B11111,B11111,B11111,B11100,B00010},
  {B11111,B11110,B11100,B11010,B10001,B01000,B00100,B00010}
};

const byte runner_chars[5][8] PROGMEM = {
  {B01110,B10111,B11111,B11100,B11111,B01010,B01010,B01000}, // Dino Walk 1
  {B01110,B10111,B11111,B11100,B11111,B01010,B01010,B00010}, // Dino Walk 2
  {B01110,B10111,B11111,B11100,B11111,B01010,B01010,B00000}, // Dino Mid-Air (no feet)
  {B00100,B00101,B10101,B10111,B11111,B01110,B00100,B00100}, // Cactus Small
  {B00100,B01101,B11101,B11111,B11111,B11111,B00100,B00100}  // Cactus Big
};

const byte lifeline_chars[3][8] PROGMEM = {
  {B01110,B10001,B10001,B11111,B11111,B01110,B00000,B00000}, // Phone
  {B01010,B01010,B11111,B10101,B10101,B11111,B00000,B00000}, // Audience
  {B11011,B10001,B11011,B01010,B00100,B01010,B11011,B11011}  // 50/50
};

void loadProgBarChars() {
  if (currentCharset == 0) return;
  for(int i=0; i<5; i++) {
    byte buffer[8]; memcpy_P(buffer, prog_chars[i], 8); lcd.createChar(i+1, buffer);
  }
  currentCharset = 0;
}
void loadEyesOpenChars() {
  if (currentCharset == 1) return;
  for(int i=0; i<8; i++) {
    byte buffer[8]; memcpy_P(buffer, ojo_abierto[i], 8); lcd.createChar(i, buffer);
  }
  currentCharset = 1;
}
void loadEyesClosedChars() {
  if (currentCharset == 2) return;
  for(int i=0; i<8; i++) {
    byte buffer[8]; memcpy_P(buffer, ojo_cerrado[i], 8); lcd.createChar(i, buffer);
  }
  currentCharset = 2;
}
void loadRunnerChars() {
  if (currentCharset == 3) return;
  for(int i=0; i<5; i++) {
    byte buffer[8]; memcpy_P(buffer, runner_chars[i], 8); lcd.createChar(i+1, buffer);
  }
  currentCharset = 3;
}
void loadLifelineChars() {
  if (currentCharset == 4) return;
  for(int i=0; i<3; i++) {
    byte buffer[8]; memcpy_P(buffer, lifeline_chars[i], 8); lcd.createChar(i+1, buffer);
  }
  currentCharset = 4;
}

// ----- GAMES VARIABLES -----
// Cyber Runner
int dinoY = 1; 
int oldDinoY = 1;
bool isJumping = false;
unsigned long jumpStartTime = 0;
int obsX = 15;
int oldObsX = 15;
int obsType = 4;
unsigned long lastRunnerUpdate = 0;
int runnerScore = 0;
bool runnerGameOver = false;

// Simon Says
int simonSeq[20];
int simonLen = 1;
int simonInputIdx = 0;
unsigned long simonLastTime = 0;
int simonState = 0; 

// Trivia 
struct TriviaQuestion {
  char q[45];
  char a[10];
  char b[10];
  char c[10];
  char d[10];
  uint8_t correct; 
}; // 86 bytes

int triviaIdx = 0;
int triviaState = 0; // 0=Answering, 3=Lifeline, 4=Result Next, 5=Result Back
unsigned long triviaScrollTime = 0;
int triviaScrollPos = 0;
TriviaQuestion curQ;
bool lifelineUsed[3] = {false, false, false};
bool optionHidden[4] = {false, false, false, false};
int lifelineCursor = 0;
String triviaMessage = "";
unsigned long triviaMessageTime = 0;
int totalTriviaStored = 0;

int triviaOptCursor = 0;
bool triviaNeedsUpdate = true;

void loadQuestionFromEEPROM(int index) {
  int addr = index * sizeof(TriviaQuestion);
  EEPROM.get(addr, curQ);
  
  // Clean potentially corrupted memory
  curQ.q[44] = '\0';
  curQ.a[9] = '\0';
  curQ.b[9] = '\0';
  curQ.c[9] = '\0';
  curQ.d[9] = '\0';
  if(curQ.correct > 3) curQ.correct = 0;
  
  for(int i=0; i<4; i++) optionHidden[i] = false;
  triviaState = 0;
  triviaScrollPos = 0;
  triviaOptCursor = 0;
  triviaNeedsUpdate = true;
}

// Ambient Eyes
unsigned long lastEyeBlink = 0;
unsigned long lastEyeMessage = 0;
bool isBlinking = false;
bool isShowingMessage = false;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_D2, INPUT_PULLUP);
  pinMode(PIN_D8, INPUT_PULLUP);
  pinMode(PIN_D6, INPUT_PULLUP);
  pinMode(PIN_D5, INPUT_PULLUP);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("CYBERDECK INIT"));
  
  // Check how many questions we have
  EEPROM.get(1000, totalTriviaStored); // Store count at end of EEPROM
  if(totalTriviaStored < 0 || totalTriviaStored > 10) totalTriviaStored = 0;
  if(totalTriviaStored > 0) loadQuestionFromEEPROM(0);

  randomSeed(analogRead(A1));
  delay(1500);
  lcd.clear();
}

void checkConnection() {
  if (millis() - lastDataTime > TIMEOUT_MS) {
    if (!isBatteryMode) {
      isBatteryMode = true;
      lcd.clear();
      runnerGameOver = false;
      runnerScore = 0;
      obsX = 15;
      simonLen = 1;
      simonState = 0;
      generateSimonSeq();
      if(totalTriviaStored > 0) {
        triviaIdx = 0;
        loadQuestionFromEEPROM(triviaIdx);
      }
    }
  } else {
    if (isBatteryMode) {
      isBatteryMode = false;
      lcd.clear();
      scrollPos = 0;
    }
  }
}

void readSerialData() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    lastDataTime = millis();
    
    if (data.startsWith("<Q|")) {
      // Parse <Q|idx|Qtext|A|B|C|D|corr>
      int p[6];
      int curr = 2;
      for(int i=0; i<6; i++) {
        p[i] = data.indexOf('|', curr+1);
        curr = p[i];
      }
      if(p[5] > 0) {
        int idx = data.substring(3, p[0]).toInt();
        TriviaQuestion tempQ;
        data.substring(p[0]+1, p[1]).toCharArray(tempQ.q, 45);
        data.substring(p[1]+1, p[2]).toCharArray(tempQ.a, 10);
        data.substring(p[2]+1, p[3]).toCharArray(tempQ.b, 10);
        data.substring(p[3]+1, p[4]).toCharArray(tempQ.c, 10);
        data.substring(p[4]+1, p[5]).toCharArray(tempQ.d, 10);
        tempQ.correct = data.substring(p[5]+1, data.indexOf('>')).toInt();
        
        EEPROM.put(idx * sizeof(TriviaQuestion), tempQ);
        if(idx + 1 > totalTriviaStored) {
          totalTriviaStored = idx + 1;
          EEPROM.put(1000, totalTriviaStored);
        }
      }
    }
    else if (data.startsWith("<L|") && data.endsWith(">")) {
      currentLyric = data.substring(3, data.length() - 1);
      typeIndex = 0;
    }
    else if (data.startsWith("<") && data.endsWith(">")) {
      data = data.substring(1, data.length() - 1);
      int p1 = data.indexOf('|');
      int p2 = data.indexOf('|', p1 + 1);
      int p3 = data.indexOf('|', p2 + 1);
      if (p1 > 0 && p2 > 0 && p3 > 0) {
        sysLine1 = data.substring(0, p1);
        sysLine2 = data.substring(p1 + 1, p2);
        spotifyText = data.substring(p2 + 1, p3);
        progressPct = data.substring(p3 + 1).toInt();
      }
    }
  }
}

// ----- BUTTON HANDLERS -----
void onBtnA_Short() {
  if (isBatteryMode) {
    if (battScreen == 1) { // Runner Jump
      if (!runnerGameOver && !isJumping) {
        isJumping = true;
        jumpStartTime = millis();
        oldDinoY = dinoY;
        dinoY = 0;
        lcd.setCursor(2, 1); lcd.print(" "); // Clear bottom instantly
      } else if (runnerGameOver) {
        runnerGameOver = false;
        runnerScore = 0;
        obsX = 15;
        lcd.clear();
      }
    } else if (battScreen == 2) { // Simon
      simonInput(0);
    } else if (battScreen == 3) { // Trivia
      handleTriviaButton(0);
    } else {
      battScreen = (battScreen + 1) % 4;
      lcd.clear();
      if (battScreen == 2) { simonLen = 1; simonState = 0; generateSimonSeq(); }
    }
  } else {
    pcScreen = (pcScreen + 1) % 4;
    lcd.clear();
    scrollPos = 0;
    if (pcScreen == 2) typeIndex = 0;
  }
}

void onBtnA_Long() {
  if (isBatteryMode) {
     battScreen = (battScreen + 1) % 4;
     lcd.clear();
     if (battScreen == 2) { simonLen = 1; simonState = 0; generateSimonSeq(); }
  } else {
    Serial.println("PLAY_PAUSE");
  }
}

void onBtnB_Short() {
  if (isBatteryMode && battScreen == 2) simonInput(1);
  else if (isBatteryMode && battScreen == 3) handleTriviaButton(1);
  else if (!isBatteryMode) Serial.println("NEXT");
}
void onBtnB_Long() {
  if (!isBatteryMode) Serial.println("PREV");
}

void onBtnC_Short() {
  if (isBatteryMode && battScreen == 2) simonInput(2);
  else if (isBatteryMode && battScreen == 3) handleTriviaButton(2);
  else if (!isBatteryMode) Serial.println("BTN_C_SHORT");
}
void onBtnC_Long() {
  if (!isBatteryMode) Serial.println("BTN_C_LONG");
}

void onBtnD_Short() {
  if (isBatteryMode && battScreen == 2) simonInput(3);
  else if (isBatteryMode && battScreen == 3) handleTriviaButton(3);
  else if (!isBatteryMode) Serial.println("BTN_D_SHORT");
}
void onBtnD_Long() {
  if (!isBatteryMode) Serial.println("BTN_D_LONG");
}

void checkButton(Button &btn, void (*onShortPress)(), void (*onLongPress)()) {
  bool currentState = digitalRead(btn.pin);
  if (currentState == LOW && btn.lastState == HIGH) { 
    btn.pressTime = millis();
    btn.isPressed = true;
  } 
  else if (currentState == HIGH && btn.lastState == LOW && btn.isPressed) { 
    unsigned long holdTime = millis() - btn.pressTime;
    if (holdTime > DEBOUNCE_MS && holdTime < LONG_PRESS_MS) onShortPress();
    btn.isPressed = false;
  } 
  else if (currentState == LOW && btn.isPressed) { 
    unsigned long holdTime = millis() - btn.pressTime;
    if (holdTime >= LONG_PRESS_MS) {
      onLongPress();
      btn.isPressed = false;
    }
  }
  btn.lastState = currentState;
}

void handleButtons() {
  checkButton(btnA, onBtnA_Short, onBtnA_Long);
  checkButton(btnB, onBtnB_Short, onBtnB_Long);
  checkButton(btnC, onBtnC_Short, onBtnC_Long);
  checkButton(btnD, onBtnD_Short, onBtnD_Long);
}

// ----- GAMES LOGIC -----
void generateSimonSeq() {
  for(int i=0; i<20; i++) simonSeq[i] = random(0, 4);
}

void simonInput(int val) {
  if (simonState == 1) {
    if (val == simonSeq[simonInputIdx]) {
      simonInputIdx++;
      if (simonInputIdx >= simonLen) {
        simonLen++;
        simonState = 0;
        simonLastTime = millis() + 500;
        lcd.clear();
        lcd.print(F("GOOD! Level ")); lcd.print(simonLen);
      }
    } else {
      simonState = 2;
      lcd.clear();
      lcd.print(F("WRONG! Game Over"));
      lcd.setCursor(0,1);
      lcd.print(F("Score: ")); lcd.print(simonLen-1);
    }
  } else if (simonState == 2) {
    simonLen = 1; simonState = 0; generateSimonSeq(); lcd.clear();
  }
}

void doTriviaPhone() {
  int r = random(0, 5);
  if (r==0) triviaMessage = F("The voices in Manan's head said its A");
  else if (r==1) { triviaMessage = F("Garima thinks its "); triviaMessage += (char)('A' + random(0,4)); }
  else if (r==2) triviaMessage = F("Minisha calls you gay");
  else if (r==3) triviaMessage = F("Shivam asks if this is the meaning of lifE");
  else triviaMessage = F("Abaan says your an idiot");
}

void handleTriviaButton(int btnIdx) {
  if(totalTriviaStored == 0) return;
  
  if (triviaState == 0) {
    if (btnIdx == 2) { // D6 pressed -> Lifelines
      triviaState = 3;
      lifelineCursor = 0;
      triviaNeedsUpdate = true;
      lcd.clear();
      return;
    }
    
    // Answering
    if (btnIdx == 3) { // D5 (Left)
      for(int i=0; i<4; i++) {
        triviaOptCursor = (triviaOptCursor + 3) % 4; // -1 % 4
        if (!optionHidden[triviaOptCursor]) break;
      }
      triviaNeedsUpdate = true;
    }
    else if (btnIdx == 1) { // D8 (Right)
      for(int i=0; i<4; i++) {
        triviaOptCursor = (triviaOptCursor + 1) % 4;
        if (!optionHidden[triviaOptCursor]) break;
      }
      triviaNeedsUpdate = true;
    }
    else if (btnIdx == 0) { // D2 (Confirm)
      lcd.clear();
      if (triviaOptCursor == curQ.correct) {
        lcd.print(F("CORRECT!"));
        runnerScore++; // Use runnerScore for trivia score
      } else {
        lcd.print(F("WRONG! It was "));
        lcd.print((char)('A' + curQ.correct));
      }
      triviaState = 4;
      triviaMessageTime = millis();
    }
  } 
  else if (triviaState == 3) {
    // Lifeline Menu
    if (btnIdx == 3) { // D5 (Left)
       lifelineCursor = (lifelineCursor + 2) % 3;
       triviaNeedsUpdate = true;
    }
    else if (btnIdx == 1) { // D8 (Right)
       lifelineCursor = (lifelineCursor + 1) % 3;
       triviaNeedsUpdate = true;
    }
    else if (btnIdx == 0) { // D2 (Confirm)
      if (!lifelineUsed[lifelineCursor]) {
        lifelineUsed[lifelineCursor] = true;
        triviaState = 5; // Show message
        triviaMessageTime = millis();
        if (lifelineCursor == 0) doTriviaPhone();
        else if (lifelineCursor == 1) { // Audience
          if (random(0,100) < 1) {
            int wrong = (curQ.correct + 1) % 4;
            triviaMessage = String((char)('A' + wrong)) + F(" has 82% votes");
          } else {
            triviaMessage = String((char)('A' + curQ.correct)) + F(" has 92% votes");
          }
        } else if (lifelineCursor == 2) { // 50/50
          int hidden = 0;
          for(int i=0; i<4 && hidden<2; i++) {
            if (i != curQ.correct && random(0,2)==0) {
              optionHidden[i] = true;
              hidden++;
            }
          }
          for(int i=0; i<4 && hidden<2; i++) {
             if (i != curQ.correct && !optionHidden[i]) { optionHidden[i] = true; hidden++; }
          }
          triviaMessage = F("Removed 2 options");
        }
      }
    }
    else if (btnIdx == 2) { // D6 (Back)
      triviaState = 0;
      triviaNeedsUpdate = true;
      lcd.clear();
    }
  }
}

// ----- RENDER LOGIC -----
void drawEyes() {
  if (millis() - lastEyeMessage > 60000) {
    isShowingMessage = true;
    lastEyeMessage = millis();
    lcd.clear();
    int r = random(0, 4);
    if(r==0) { lcd.print(F("Drink water!")); }
    if(r==1) { lcd.print(F("Check posture!")); }
    if(r==2) { lcd.print(F("Stretch legs!")); }
    if(r==3) { lcd.print(F("Take a breath.")); }
  }
  
  if (isShowingMessage) {
    if (millis() - lastEyeMessage > 3000) {
      isShowingMessage = false;
      lcd.clear();
      lastEyeMessage = millis();
    } else {
      return;
    }
  }

  if (!isBlinking && millis() - lastEyeBlink > 3000) {
    if (random(0,10) > 4) {
      isBlinking = true;
      lastEyeBlink = millis();
      loadEyesClosedChars();
    } else lastEyeBlink = millis();
  }
  if (isBlinking && millis() - lastEyeBlink > 200) {
    isBlinking = false;
    lastEyeBlink = millis();
    loadEyesOpenChars();
  }
  if (!isBlinking) loadEyesOpenChars();
  
  lcd.setCursor(2,0); lcd.write(0); lcd.write(1); lcd.write(2); lcd.write(3);
  lcd.setCursor(2,1); lcd.write(4); lcd.write(5); lcd.write(6); lcd.write(7);
  lcd.setCursor(10,0); lcd.write(0); lcd.write(1); lcd.write(2); lcd.write(3);
  lcd.setCursor(10,1); lcd.write(4); lcd.write(5); lcd.write(6); lcd.write(7);
}

void loop() {
  readSerialData();
  checkConnection();
  handleButtons();
  
  int flameVal = digitalRead(FLAME_SENSOR_PIN);
  if (flameVal == LOW && !flameDetected) {
    flameDetected = true;
    Serial.println("FLAME_DETECTED");
  } else if (flameVal == HIGH && flameDetected) flameDetected = false;

  if (!isBatteryMode) {
    // --- PC MODE ---
    if (pcScreen == 0) {
      lcd.setCursor(0, 0); lcd.print(sysLine1.substring(0, 16));
      lcd.setCursor(0, 1); lcd.print(sysLine2.substring(0, 16));
    } 
    else if (pcScreen == 1) {
      loadProgBarChars();
      if (millis() - lastScroll > 400) {
        lastScroll = millis();
        lcd.setCursor(0, 0);
        if (spotifyText.length() <= 16) {
          lcd.print(spotifyText);
          for(int i=spotifyText.length(); i<16; i++) lcd.print(" ");
        } else {
          String displayStr = spotifyText.substring(scrollPos) + "   " + spotifyText.substring(0, scrollPos);
          lcd.print(displayStr.substring(0, 16));
          scrollPos++;
          if (scrollPos >= spotifyText.length() + 3) scrollPos = 0;
        }
      }
      int totalBlocks = (progressPct * 80) / 100;
      int fullBlocks = totalBlocks / 5;
      int partialBlock = totalBlocks % 5;
      lcd.setCursor(0, 1);
      for(int i=0; i<16; i++) {
        if (i < fullBlocks) lcd.write(5);
        else if (i == fullBlocks) {
          if (partialBlock == 0) lcd.print(" ");
          else lcd.write(partialBlock);
        } else lcd.print(" ");
      }
    }
    else if (pcScreen == 2) {
      if (millis() - lastTypeTime > 25) { 
        lastTypeTime = millis();
        if (typeIndex < currentLyric.length()) typeIndex++;
      }
      int page = (typeIndex > 0) ? (typeIndex - 1) / 32 : 0;
      int startIdx = page * 32;
      int charsToShow = typeIndex - startIdx;
      
      String topRow = currentLyric.substring(startIdx, startIdx + min(charsToShow, 16));
      lcd.setCursor(0, 0); lcd.print(topRow);
      for (int i=topRow.length(); i<16; i++) lcd.print(" ");
      
      lcd.setCursor(0, 1);
      if (charsToShow > 16) {
        String bottomRow = currentLyric.substring(startIdx + 16, startIdx + charsToShow);
        lcd.print(bottomRow);
        for (int i=bottomRow.length(); i<16; i++) lcd.print(" ");
      } else lcd.print("                ");
    }
    else if (pcScreen == 3) drawEyes();
  } else {
    // --- BATTERY MODE ---
    if (battScreen == 0) {
      drawEyes();
    }
    else if (battScreen == 1) { // Cyber Runner (Ghosting fixed)
      loadRunnerChars();
      if (runnerGameOver) {
        lcd.setCursor(0,0); lcd.print(F("GAME OVER!      "));
        lcd.setCursor(0,1); lcd.print(F("Score: ")); lcd.print(runnerScore);
      } else {
        if (millis() - lastRunnerUpdate > max(60, 180 - runnerScore*2)) { // Faster subgrid timing
          lastRunnerUpdate = millis();
          oldObsX = obsX;
          obsX--;
          if (obsX < 0) {
            obsX = 15;
            obsType = random(4, 6); // 4 or 5
            runnerScore++;
          }
          
          oldDinoY = dinoY;
          if (isJumping) {
            unsigned long jTime = millis() - jumpStartTime;
            if (jTime < 200) dinoY = 0; // Ascend
            else if (jTime < 450) dinoY = 0; // Peak
            else if (jTime < 600) dinoY = 1; // Descend
            else { isJumping = false; dinoY = 1; }
          }
          
          if (obsX == 2 && dinoY == 1) runnerGameOver = true;
          else {
            // Only overwrite old spaces
            if(oldObsX != obsX) {
               lcd.setCursor(oldObsX, 1); lcd.print(" "); 
            }
            if(oldDinoY != dinoY && !isJumping) {
               lcd.setCursor(2, oldDinoY); lcd.print(" ");
            }
            
            lcd.setCursor(2, dinoY);
            if(isJumping) lcd.write(3); // Mid-air sprite
            else lcd.write((millis() % 400 < 200) ? 1 : 2); // Walk
            
            lcd.setCursor(obsX, 1); lcd.write(obsType);
            
            lcd.setCursor(12, 0); lcd.print(runnerScore);
          }
        }
      }
    }
    else if (battScreen == 2) {
      if (simonState == 0) {
        if (millis() - simonLastTime > 600) {
          int i = (millis() - simonLastTime - 600) / 400; // Faster flashing
          if (i < simonLen) {
            lcd.setCursor(0, 0); lcd.print(F("                "));
            lcd.setCursor(6, 0);
            int btn = simonSeq[i];
            if (btn==0) lcd.print(F("> D2 <"));
            if (btn==1) lcd.print(F("> D8 <"));
            if (btn==2) lcd.print(F("> D6 <"));
            if (btn==3) lcd.print(F("> D5 <"));
          } else {
            simonState = 1; simonInputIdx = 0;
            lcd.setCursor(0, 0); lcd.print(F("   YOUR TURN    "));
          }
        }
      }
    }
    else if (battScreen == 3) {
      // TRIVIA
      if(totalTriviaStored == 0) {
        lcd.setCursor(0,0); lcd.print(F("No Trivia Data  "));
        lcd.setCursor(0,1); lcd.print(F("Connect to PC   "));
        return;
      }
      
      if (triviaState == 0) {
        // Scroll Question on Top Row
        if (millis() - triviaScrollTime > 300) {
          triviaScrollTime = millis();
          String qstr = String(curQ.q) + "   ";
          lcd.setCursor(0,0);
          if(qstr.length() <= 16) {
             lcd.print(qstr); for(int i=qstr.length(); i<16; i++) lcd.print(" ");
          } else {
             String d = qstr.substring(triviaScrollPos) + qstr.substring(0, triviaScrollPos);
             lcd.print(d.substring(0, 16));
             triviaScrollPos = (triviaScrollPos + 1) % qstr.length();
          }
        }
        
        if (triviaNeedsUpdate) {
          lcd.setCursor(0,1);
          String s = "> ";
          s += (char)('A' + triviaOptCursor);
          s += ": ";
          if (triviaOptCursor == 0) s += curQ.a;
          else if (triviaOptCursor == 1) s += curQ.b;
          else if (triviaOptCursor == 2) s += curQ.c;
          else if (triviaOptCursor == 3) s += curQ.d;
          
          lcd.print(s);
          for(int i=s.length(); i<16; i++) lcd.print(" ");
          triviaNeedsUpdate = false;
        }
      }
      else if (triviaState == 3) { // Lifeline Menu
        if (triviaNeedsUpdate) {
          loadLifelineChars();
          lcd.setCursor(0,0); lcd.print(F("LIFELINE (D6)   "));
          lcd.setCursor(0,1);
          lcd.print("> ");
          if (lifelineCursor==0) { lcd.write(1); lcd.print(lifelineUsed[0]?F(" Used      "):F(" Phone     ")); }
          if (lifelineCursor==1) { lcd.write(2); lcd.print(lifelineUsed[1]?F(" Used      "):F(" Audience  ")); }
          if (lifelineCursor==2) { lcd.write(3); lcd.print(lifelineUsed[2]?F(" Used      "):F(" 50/50     ")); }
          triviaNeedsUpdate = false;
        }
      }
      else if (triviaState == 4 || triviaState == 5) {
        if (triviaState == 5) { // Show message from lifeline
          if (millis() - triviaScrollTime > 200) {
             triviaScrollTime = millis();
             lcd.setCursor(0,1);
             String m = triviaMessage + "   ";
             if(m.length()<=16) lcd.print(m);
             else {
                String d = m.substring(triviaScrollPos) + m.substring(0, triviaScrollPos);
                lcd.print(d.substring(0,16));
                triviaScrollPos = (triviaScrollPos + 1) % m.length();
             }
          }
        }
        
        if (millis() - triviaMessageTime > 4000) {
          if (triviaState == 4) { // Next question
             triviaIdx = (triviaIdx + 1) % totalTriviaStored;
             loadQuestionFromEEPROM(triviaIdx);
          } else {
             triviaState = 0; // Back to answering
             lcd.clear();
          }
        }
      }
    }
  }
}
