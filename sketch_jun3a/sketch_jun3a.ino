 #include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set your LCD address (0x27 and 0x3F are the most common)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Definitions
const int BTN_A = 2; // Nav / Play-Pause
const int BTN_B = 8; // Next / Prev
const int BTN_C = 6; // Custom / Extra
const int BTN_D = 5; // Custom / Extra
const int FLAME_SENSOR_PIN = 7; // Digital Out from Flame Sensor

// Button State Tracking Structure
struct Button {
  int pin;
  bool lastState;
  unsigned long pressTime;
  bool isPressed;
};

// Initialize buttons (starting state HIGH due to INPUT_PULLUP)
Button btnA = {BTN_A, HIGH, 0, false};
Button btnB = {BTN_B, HIGH, 0, false};
Button btnC = {BTN_C, HIGH, 0, false};
Button btnD = {BTN_D, HIGH, 0, false};

bool flameDetected = false;

// Timing constants for button presses
const int LONG_PRESS_MS = 500;
const int DEBOUNCE_MS = 50;

// System States
int currentScreen = 0; // 0 = PC Stats, 1 = Spotify
String sysLine1 = "Waiting for PC..";
String sysLine2 = "";
String spotifyText = "Connecting...   ";

// Scrolling Variables for Spotify
unsigned long lastScroll = 0;
int scrollPos = 0;
int progressPct = 0;

// Variables for Lyrics
String currentLyric = "";
int typeIndex = 0;
unsigned long lastTypeTime = 0;

void setup() {
  // Start serial communication at 9600 baud rate
  Serial.begin(9600);
  
  // Enable internal pull-up resistors (buttons read LOW when pressed)
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_C, INPUT_PULLUP);
  pinMode(BTN_D, INPUT_PULLUP);
  
  pinMode(FLAME_SENSOR_PIN, INPUT);
  
  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  
  // Custom characters for progress bar
  byte p1[8] = {0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
  byte p2[8] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18};
  byte p3[8] = {0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C};
  byte p4[8] = {0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E};
  byte p5[8] = {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};
  lcd.createChar(1, p1);
  lcd.createChar(2, p2);
  lcd.createChar(3, p3);
  lcd.createChar(4, p4);
  lcd.createChar(5, p5);

  lcd.setCursor(0, 0);
  lcd.print("CYBERDECK INIT");
  delay(1500);
  lcd.clear();
}

void loop() {
  readSerialData();
  handleButtons();
  checkFlameSensor();
  updateDisplay();
}

void checkFlameSensor() {
  // Flame sensors often read LOW when a flame is detected
  int flameVal = digitalRead(FLAME_SENSOR_PIN);
  if (flameVal == LOW && !flameDetected) {
    flameDetected = true;
    Serial.println("FLAME_DETECTED");
  } else if (flameVal == HIGH && flameDetected) {
    flameDetected = false;
  }
}

// --- COMMUNICATION ---
void readSerialData() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    
    // Parse the incoming packet
    if (data.startsWith("<L|") && data.endsWith(">")) {
      currentLyric = data.substring(3, data.length() - 1);
      typeIndex = 0;
    }
    else if (data.startsWith("<") && data.endsWith(">")) {
      data = data.substring(1, data.length() - 1); // Strip the < and >
      
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

// --- BUTTON LOGIC ---
void checkButton(Button &btn, void (*onShortPress)(), void (*onLongPress)()) {
  bool currentState = digitalRead(btn.pin);
  
  if (currentState == LOW && btn.lastState == HIGH) { 
    // Button just went down
    btn.pressTime = millis();
    btn.isPressed = true;
  } 
  else if (currentState == HIGH && btn.lastState == LOW && btn.isPressed) { 
    // Button just went up (Release)
    unsigned long holdTime = millis() - btn.pressTime;
    
    if (holdTime > DEBOUNCE_MS && holdTime < LONG_PRESS_MS) {
      onShortPress(); // Trigger short press action
    }
    btn.isPressed = false;
  } 
  else if (currentState == LOW && btn.isPressed) { 
    // Button is currently being held down
    unsigned long holdTime = millis() - btn.pressTime;
    if (holdTime >= LONG_PRESS_MS) {
      onLongPress(); // Trigger long press action
      btn.isPressed = false; // Set false to prevent multiple fires while holding
    }
  }
  btn.lastState = currentState;
}

// Callbacks for Button A (Pin D2)
void btnA_Short() {
  currentScreen = (currentScreen + 1) % 3; // Toggle between screen 0, 1, 2
  lcd.clear();
  scrollPos = 0; // Reset scroll position when switching screens
  if (currentScreen == 2) {
    typeIndex = 0; // Restart typewriter on menu switch
  }
}
void btnA_Long() { 
  Serial.println("PLAY_PAUSE"); 
}

// Callbacks for Button B (Pin D8)
void btnB_Short() { 
  Serial.println("NEXT"); 
}
void btnB_Long()  { 
  Serial.println("PREV"); 
}

// Callbacks for Button C (Pin D6)
void btnC_Short() { Serial.println("BTN_C_SHORT"); }
void btnC_Long()  { Serial.println("BTN_C_LONG"); }

// Callbacks for Button D (Pin D5)
void btnD_Short() { Serial.println("BTN_D_SHORT"); }
void btnD_Long()  { Serial.println("BTN_D_LONG"); }

void handleButtons() {
  checkButton(btnA, btnA_Short, btnA_Long);
  checkButton(btnB, btnB_Short, btnB_Long);
  checkButton(btnC, btnC_Short, btnC_Long);
  checkButton(btnD, btnD_Short, btnD_Long);
}

// --- DISPLAY LOGIC ---
void updateDisplay() {
  if (currentScreen == 0) {
    // --- MODE 0: PC STATS ---
    lcd.setCursor(0, 0);
    lcd.print(sysLine1.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(sysLine2.substring(0, 16));
  } 
  else if (currentScreen == 1) {
    // --- MODE 1: SPOTIFY ---
    // Spinning disc animation removed - scrolling on full 16 columns
    if (millis() - lastScroll > 400) { // 400ms scroll speed
      lastScroll = millis();
      
      lcd.setCursor(0, 0); // Name on top row now
      if (spotifyText.length() <= 16) {
        lcd.print(spotifyText);
        for(int i=spotifyText.length(); i<16; i++) lcd.print(" ");
      } else {
        // Create a circular string with spacing
        String displayStr = spotifyText.substring(scrollPos) + "   " + spotifyText.substring(0, scrollPos);
        lcd.print(displayStr.substring(0, 16));
        
        scrollPos++;
        if (scrollPos >= spotifyText.length() + 3) {
          scrollPos = 0;
        }
      }
    }
    
    // Progress bar on bottom row (16 chars wide, 5 sub-blocks per char = 80 total blocks)
    int totalBlocks = (progressPct * 80) / 100;
    int fullBlocks = totalBlocks / 5;
    int partialBlock = totalBlocks % 5;
    
    lcd.setCursor(0, 1);
    for(int i=0; i<16; i++) {
      if (i < fullBlocks) lcd.write((uint8_t)5);
      else if (i == fullBlocks) {
        if (partialBlock == 0) lcd.print(" ");
        else lcd.write((uint8_t)partialBlock);
      } else {
        lcd.print(" ");
      }
    }
  }
  else if (currentScreen == 2) {
    // --- MODE 2: LYRICS ---
    if (millis() - lastTypeTime > 25) { 
      lastTypeTime = millis();
      if (typeIndex < currentLyric.length()) {
        typeIndex++;
      }
    }
    
    int page = (typeIndex > 0) ? (typeIndex - 1) / 32 : 0;
    int startIdx = page * 32;
    int charsToShow = typeIndex - startIdx;
    
    String topRow = currentLyric.substring(startIdx, startIdx + min(charsToShow, 16));
    lcd.setCursor(0, 0);
    lcd.print(topRow);
    for (int i=topRow.length(); i<16; i++) lcd.print(" ");
    
    lcd.setCursor(0, 1);
    if (charsToShow > 16) {
      String bottomRow = currentLyric.substring(startIdx + 16, startIdx + charsToShow);
      lcd.print(bottomRow);
      for (int i=bottomRow.length(); i<16; i++) lcd.print(" ");
    } else {
      lcd.print("                ");
    }
  }
}