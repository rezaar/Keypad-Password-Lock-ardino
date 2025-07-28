/*
  Project: Keypad Password Lock
  Author: Reza Ahmadi
  GitHub: https://github.com/rezaar/Keypad-Password-Lock-ardino
  License: Do not use commercially without permission.
*/
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// --- LCD I2C ---
LiquidCrystal_I2C lcd(0x27, 16, 2);

// --- Keypad ---
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5};
byte colPins[COLS] = {6, 7, 8, A0};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// --- رمز عبور ---
#define PASS_ADDR 0
#define PASS_LEN 6
String correctPass = "";
String inputBuffer = "";
String tempNewPass = "";
bool firstRun = false;

// --- وضعیت‌ها ---
enum State {
  NORMAL,
  ENTER_FIRST_PASS,
  ENTER_OLD_PASS,
  ENTER_NEW_PASS,
  CONFIRM_NEW_PASS
};
State state = NORMAL;

// --- رله ---
#define RELAY_PIN A1

// --- امنیت ---
int wrongAttempts = 0;
const int maxAttempts = 3;
unsigned long lockoutStart = 0;
const unsigned long lockoutDuration = 30000;
bool isLockedOut = false;

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  correctPass = readPass();
  if (!isPassValid(correctPass)) {
    lcd.print("Set New Pass:");
    state = ENTER_FIRST_PASS;
    firstRun = true;
  } else {
    lcd.print("Enter Password:");
  }
}

void loop() {
  if (isLockedOut) {
    if (millis() - lockoutStart >= lockoutDuration) {
      isLockedOut = false;
      wrongAttempts = 0;
      lcd.clear();
      lcd.print("Try Again:");
    } else {
      return; // قفل است، کاری نکن
    }
  }

  handleKeypad();
}

void handleKeypad() {
  char key = keypad.getKey();
  if (!key) return;

  if (key == '*') {
    inputBuffer = "";
    showMessage("Cleared", 500);
    return;
  }

  if (isdigit(key) && inputBuffer.length() < PASS_LEN) {
    inputBuffer += key;
    lcd.setCursor(inputBuffer.length() - 1, 1);
    lcd.print('*');
  }

  if (key == 'A') {
    if (!firstRun) {
      state = ENTER_OLD_PASS;
      inputBuffer = "";
      showMessage("Enter Old Pass:", 0);
    }
    return;
  }

  if (key == 'B') {
    if (inputBuffer.length() != PASS_LEN) {
      showMessage("6 digits only!", 1000);
      return;
    }

    if (state == ENTER_FIRST_PASS) {
      correctPass = inputBuffer;
      savePass(correctPass);
      showMessage("Pass Saved", 1500);
      state = NORMAL;
      firstRun = false;
      lcd.print("Enter Password:");
      inputBuffer = "";
      return;
    }

    if (state == ENTER_OLD_PASS) {
      if (inputBuffer == correctPass) {
        state = ENTER_NEW_PASS;
        inputBuffer = "";
        showMessage("New Password:", 0);
      } else {
        state = NORMAL;
        showMessage("Wrong Old Pass", 1500);
        inputBuffer = "";
      }
    } else if (state == ENTER_NEW_PASS) {
      tempNewPass = inputBuffer;
      inputBuffer = "";
      state = CONFIRM_NEW_PASS;
      showMessage("Confirm Again:", 0);
    }
    return;
  }

  if (key == 'D') {
    if (state == CONFIRM_NEW_PASS && inputBuffer.length() == PASS_LEN) {
      if (inputBuffer == tempNewPass) {
        correctPass = tempNewPass;
        savePass(correctPass);
        showMessage("Password Changed", 1500);
      } else {
        showMessage("Mismatch!", 1500);
      }
      state = NORMAL;
      inputBuffer = "";
    }
  }

  if (state == NORMAL && inputBuffer.length() == PASS_LEN) {
    if (inputBuffer == correctPass) {
      unlock();
      wrongAttempts = 0; // بازنشانی تلاش‌ها
    } else {
      wrongAttempts++;
      if (wrongAttempts >= maxAttempts) {
        showMessage("LOCKED OUT!", 1500);
        isLockedOut = true;
        lockoutStart = millis();
      } else {
        showMessage("Wrong Password!", 1500);
      }
    }
    inputBuffer = "";
  }
}

void unlock() {
  showMessage("Access Granted", 1000);
  digitalWrite(RELAY_PIN, HIGH);
  delay(5000);
  digitalWrite(RELAY_PIN, LOW);
  lcd.clear();
  lcd.print("Enter Password:");
}

void showMessage(String msg, int wait) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  if (wait > 0) delay(wait);
  if (state == NORMAL && wait > 0 && !isLockedOut) {
    lcd.clear();
    lcd.print("Enter Password:");
  }
}

String readPass() {
  String p = "";
  for (int i = 0; i < PASS_LEN; i++) {
    char c = EEPROM.read(PASS_ADDR + i);
    if (isDigit(c)) p += c;
  }
  return p;
}

void savePass(String pass) {
  for (int i = 0; i < PASS_LEN; i++) {
    EEPROM.write(PASS_ADDR + i, pass[i]);
  }
}

bool isPassValid(String p) {
  return p.length() == PASS_LEN && p != "000000";
}
