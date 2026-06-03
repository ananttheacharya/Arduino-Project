#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set your LCD address (0x27 and 0x3F are the most common)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Definitions
const int BTN_A = 2; // Nav / Play-Pause
const int BTN_B = 8; // Next / Prev

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

void setup() {
  // Start serial communication at 9600 baud rate
  Serial.begin(9600);
  
  // Enable internal pull-up resistors (buttons read LOW when pressed)
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  
  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("CYBERDECK INIT");
  delay(1500);
  lcd.clear();
}

void loop() {
  readSerialData();
  handleButtons();
  updateDisplay();
}

// --- COMMUNICATION ---
void readSerialData() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    
    // Parse the incoming packet: <SysLine1|SysLine2|SpotifyData>
    if (data.startsWith("<") && data.endsWith(">")) {
      data = data.substring(1, data.length() - 1); // Strip the < and >
      
      int firstPipe = data.indexOf('|');
      int secondPipe = data.indexOf('|', firstPipe + 1);
      
      if (firstPipe > 0 && secondPipe > 0) {
        sysLine1 = data.substring(0, firstPipe);
        sysLine2 = data.substring(firstPipe + 1, secondPipe);
        spotifyText = data.substring(secondPipe + 1);
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
  currentScreen = !currentScreen; // Toggle between screen 0 and 1
  lcd.clear();
  scrollPos = 0; // Reset scroll position when switching screens
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

void handleButtons() {
  checkButton(btnA, btnA_Short, btnA_Long);
  checkButton(btnB, btnB_Short, btnB_Long);
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
    lcd.setCursor(0, 0);
    lcd.print("NOW PLAYING:    ");
    
    // Scrolling logic for song names longer than 16 characters
    if (millis() - lastScroll > 400) { // 400ms scroll speed
      lastScroll = millis();
      
      lcd.setCursor(0, 1);
      if (spotifyText.length() <= 16) {
        lcd.print(spotifyText);
      } else {
        // Create a circular string and grab a 16-character window
        String displayStr = spotifyText.substring(scrollPos) + spotifyText.substring(0, scrollPos);
        lcd.print(displayStr.substring(0, 16));
        
        scrollPos++;
        if (scrollPos >= spotifyText.length()) {
          scrollPos = 0;
        }
      }
    }
  }
}