#include <Wire.h>
#include "HX711.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

#define sensorPin1 3
#define sensorPin2 2

#define DOUT 8
#define CLK 9

HX711 scale;

float calibration_factor = 312;  // Set your calibration factor
float units;
float ounces;

int sensorState1 = 0;
int sensorState2 = 0;
int count = 0;

int Buzzer = 10;
int Mode_Button = 6;
int Add_Button = 7;
int Sub_Button = 5;

const unsigned long interval = 500;
unsigned long previousMillis = 0;

const unsigned long interval_2 = 200;
unsigned long previousMillis_2 = 0;

int normal_max_people;
int normalpeopleAdddress = 5;

int pandemic_max_people;
int pandemicpeopleAddress = 8;

int Mode;
int ModeAddress = 10;

volatile boolean buttonState1 = HIGH;
volatile boolean lastButtonState1 = HIGH;
volatile unsigned long lastDebounceTime1 = 0;
volatile unsigned long debounceDelay1 = 50;

volatile boolean buttonState2 = HIGH;
volatile boolean lastButtonState2 = HIGH;
volatile unsigned long lastDebounceTime2 = 0;
volatile unsigned long debounceDelay2 = 50;

volatile boolean buttonState3 = HIGH;
volatile boolean lastButtonState3 = HIGH;
volatile unsigned long lastDebounceTime3 = 0;
volatile unsigned long debounceDelay3 = 50;

enum State {
  IDLE,
  MODE,
  NORMAL_MODE,
  NORMAL_MODE_COUNT,
  PANDEMIC_MODE,
  PANDEMIC_MODE_COUNT,
  CHECK_WEIGHT,
  READ_SMS
};

State currentState = IDLE;

SoftwareSerial sim800l(13, 12);
LiquidCrystal_I2C lcd(0x3F, 16, 2);

void setup() {
  Serial.begin(9600);

  pinMode(sensorPin1, INPUT);
  pinMode(sensorPin2, INPUT);

  pinMode(Mode_Button, INPUT_PULLUP);
  pinMode(Add_Button, INPUT_PULLUP);
  pinMode(Sub_Button, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);

  digitalWrite(Buzzer, LOW);

  normal_max_people = EEPROM.read(normalpeopleAdddress);
  pandemic_max_people = EEPROM.read(pandemicpeopleAddress);
  Mode = EEPROM.read(ModeAddress);

  scale.begin(DOUT, CLK);  // Initialize with data and clock pins
  scale.set_scale(calibration_factor);
  scale.tare();  // Reset the scale to 0

  sim800l.begin(9600);  // initialize the SIM800L module
  // configure SIM800L to receive text messages
  sim800l.println("AT+CMGF=1");  // set SMS text mode
  delay(100);
  sim800l.println("AT+CNMI=1,2,0,0,0");  // set SIM800L to notify when new SMS is received
  delay(100);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("SYSTEM");
  lcd.setCursor(1, 1);
  lcd.print("INITIALIZATION");
  delay(2000);
}

void loop() {
  unsigned long currentMillis = millis();

  // Check if 500 milliseconds have passed
  if (currentMillis - previousMillis >= interval) {
    read_sms();
    //Serial.println("Check Message");
    previousMillis = currentMillis;
  }
  AddButton();
  ModeButton();
  SubButton();
  switch (currentState) {
    case IDLE:
      idle();
      break;
    case MODE:
      mode();
      break;
    case NORMAL_MODE:
      normal_mode();
      break;
    case NORMAL_MODE_COUNT:
      normal_Mode_Count();
      break;
    case PANDEMIC_MODE:
      pandemic_mode();
      break;
    case PANDEMIC_MODE_COUNT:
      pandemic_Mode_Count();
      break;
    case CHECK_WEIGHT:
      check_weight();
      break;
    case READ_SMS:
      read_sms();
      break;
  }
}
void idle() {
  ModeButton();
  AddButton();
  SubButton();
  if (Mode == 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RUNNING MODE");
    lcd.setCursor(0, 1);
    lcd.print("NORMAL MODE");
    delay(2000);
    currentState = NORMAL_MODE_COUNT;
  } else if (Mode == 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RUNNING MODE");
    lcd.setCursor(0, 1);
    lcd.print("PANDEMIC MODE");
    delay(2000);
    currentState = PANDEMIC_MODE_COUNT;
  }
}
void normal_Mode_Count() {
  ModeButton();
  AddButton();
  SubButton();

  sensorState1 = digitalRead(sensorPin1);
  sensorState2 = digitalRead(sensorPin2);
  if (sensorState1 == LOW) {
    count++;
    delay(500);
  }

  if (sensorState2 == LOW) {
    count--;
    delay(500);
  }

  Serial.println("Count: " + String(count));
  unsigned long currentMillis_2 = millis();
  if (currentMillis_2 - previousMillis_2 >= interval_2) {
    if (count <= 0) {
      count = 0;
      digitalWrite(Buzzer, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("LIFT STATUS");
      lcd.setCursor(0, 1);
      lcd.print("NO PEOPLE");

    } else if (count > normal_max_people) {
      digitalWrite(Buzzer, HIGH);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ALERT MAX PEOPLE");
      // lcd.setCursor(0, 1);
      // lcd.print("ALERT MAX PEOPLE");
      lcd.print(count);
    } else if (count < normal_max_people) {
      digitalWrite(Buzzer, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("LIFT STATUS");
      lcd.setCursor(0, 1);
      lcd.print("PEOPLE: ");
      lcd.print(count);
    }
    previousMillis_2 = currentMillis_2;
  }
}
void pandemic_Mode_Count() {
  ModeButton();
  AddButton();
  SubButton();
  sensorState1 = digitalRead(sensorPin1);
  sensorState2 = digitalRead(sensorPin2);
  if (sensorState1 == LOW) {
    count++;
    delay(500);
  }

  if (sensorState2 == LOW) {
    count--;
    delay(500);
  }

  if (count <= 0) {
    Serial.println("No visitors");
    digitalWrite(Buzzer, LOW);
  } else if (count > pandemic_max_people) {
    digitalWrite(Buzzer, HIGH);
  } else if (count < pandemic_max_people) {
    digitalWrite(Buzzer, LOW);
  }
}
void mode() {
  ModeButton();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1. NORMAL MODE");
  lcd.setCursor(0, 1);
  lcd.print("2. PANDEMIC MODE");
  delay(500);
}
void normal_mode() {
  ModeButton();
  AddButton();
  SubButton();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("NORMAL MODE");
  lcd.setCursor(0, 1);
  lcd.print("MAX PEOPLE: ");
  lcd.print(normal_max_people);
  delay(500);
}
void pandemic_mode() {
  ModeButton();
  AddButton();
  SubButton();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PANDEMIC MODE");
  lcd.setCursor(0, 1);
  lcd.print("MAX PEOPLE: ");
  lcd.print(pandemic_max_people);
  delay(500);
}
void check_weight() {
}
void read_sms() {
  if (sim800l.available()) {                         // check if there is a message available
    String message = sim800l.readString();           // read the message
    Serial.println("Received message: " + message);  // print the message to the serial monitor
    if (message.indexOf("NORMAL") != -1) {           // if the message contains "ON"
      Mode = 0;
      EEPROM.write(ModeAddress, Mode);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("UPDATED MODE");
      lcd.setCursor(0, 1);
      lcd.print("NORMAL MODE");
      delay(3000);
    } else if (message.indexOf("PANDEMIC") != -1) {  // if the message contains "OFF"
      Mode = 1;
      EEPROM.write(ModeAddress, Mode);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("UPDATED MODE");
      lcd.setCursor(0, 1);
      lcd.print("PANDEMIC MODE");
      delay(3000);
    }
  }
  delay(1000);  // wait for 1 second before checking for new messages
}
void AddButton() {
  if (millis() - lastDebounceTime1 > debounceDelay1) {
    buttonState1 = digitalRead(Add_Button);
    if (buttonState1 == LOW && lastButtonState1 == HIGH) {
      Serial.println("Add Button Pressed!");
      if (currentState == NORMAL_MODE || currentState == NORMAL_MODE_COUNT || currentState == PANDEMIC_MODE_COUNT) {
        normal_max_people++;
        if (EEPROM.read(normalpeopleAdddress) != normal_max_people) {
          EEPROM.write(normalpeopleAdddress, normal_max_people);
        }
      } else if (currentState == PANDEMIC_MODE || currentState == NORMAL_MODE_COUNT || currentState == PANDEMIC_MODE_COUNT) {
        pandemic_max_people++;
        if (EEPROM.read(pandemicpeopleAddress) != pandemic_max_people) {
          EEPROM.write(pandemicpeopleAddress, pandemic_max_people);
        }
      } else if (currentState = MODE) {
        currentState = NORMAL_MODE;
      }
    }
    lastButtonState1 = buttonState1;
    lastDebounceTime1 = millis();
  }
}
void ModeButton() {
  if (millis() - lastDebounceTime2 > debounceDelay2) {
    buttonState2 = digitalRead(Mode_Button);
    if (buttonState2 == LOW && lastButtonState2 == HIGH) {
      Serial.println("Mode Button Pressed!");
      if (currentState == IDLE || currentState == NORMAL_MODE_COUNT || currentState == PANDEMIC_MODE_COUNT) {
        currentState = MODE;
      } else if (currentState == MODE || currentState == NORMAL_MODE || currentState == PANDEMIC_MODE || currentState == NORMAL_MODE_COUNT || currentState == PANDEMIC_MODE_COUNT) {
        currentState = IDLE;
      }
    }
    lastButtonState2 = buttonState2;
    lastDebounceTime2 = millis();
  }
}
void SubButton() {
  if (millis() - lastDebounceTime3 > debounceDelay3) {
    buttonState3 = digitalRead(Sub_Button);
    if (buttonState3 == LOW && lastButtonState3 == HIGH) {
      Serial.println("Sub Button Pressed!");
      if (currentState == NORMAL_MODE || currentState == NORMAL_MODE_COUNT || currentState == PANDEMIC_MODE_COUNT) {
        normal_max_people--;
        if (EEPROM.read(normalpeopleAdddress) != normal_max_people) {
          EEPROM.write(normalpeopleAdddress, normal_max_people);
        }
      } else if (currentState == PANDEMIC_MODE || currentState == NORMAL_MODE_COUNT || currentState == PANDEMIC_MODE_COUNT) {
        pandemic_max_people--;
        if (EEPROM.read(pandemicpeopleAddress) != pandemic_max_people) {
          EEPROM.write(pandemicpeopleAddress, pandemic_max_people);
        }
      } else if (currentState = MODE) {
        currentState = PANDEMIC_MODE;
      }
    }
    lastButtonState3 = buttonState3;
    lastDebounceTime3 = millis();
  }
}
void Read_Weight() {
  Serial.print("Reading: ");
  units = scale.get_units(10);  // Get average of 10 readings
  if (units < 0) {
    units = 0.00;  // Ensure we don't show negative values
  }
  ounces = units * 0.035274;  // Convert grams to ounces
  Serial.print(units);
  Serial.println(" grams");
  delay(1000);
}
