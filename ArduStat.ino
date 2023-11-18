#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <U8g2lib.h>
#include <virtuabotixRTC.h>
#include <Keypad.h>
#include <avr/wdt.h>
#include <EEPROM.h>
//nastavení obrazovky
enum Screen { BASIC,
              SETTINGS,
              SET_TIME,
              SET_TEMP,
              SOFT_RESET,
              SET_DATE,
              SET_T_OFF };

Screen currentScreen = BASIC;
enum ConfigState {
  SELECT_DAY,
  SELECT_INTERVAL,
  SET_MIN_TEMP,
  SET_MAX_TEMP,
  CONFIRMATION
};
enum RelayOverride {
  AUTO,
  ON,
  OFF
};
RelayOverride relayOverride = AUTO;
ConfigState configState = SELECT_DAY;
int selectedDay = 0;
int selectedInterval = 0;
float newMinTemp, newMaxTemp;
const int DAYS_IN_WEEK = 7;
const int TIME_INTERVALS = 4;
float temperatureThresholds[DAYS_IN_WEEK][TIME_INTERVALS][2];
//nastaveni delaye
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 1000;
//adresa pro senzor teploty a tlaku
#define BMP280_ADRESA (0x76)
Adafruit_BMP280 bmp;
float temperatureOffset = 0.0;
const float DEFAULT_TEMPERATURE_OFFSET = 0.0;
//inicializace displeje
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
int selectedTimePart = 0;
int selectedDatePart = 0;
//nastavení RTC
const int kPin_CLK = 3;
const int kPin_DAT = 4;
const int kPin_RST = 5;
virtuabotixRTC myRTC(kPin_CLK, kPin_DAT, kPin_RST);
int setHours, setMinutes, setSeconds;
int timeSettingStage = 0;
int setDay, setMonth, setYear, setDOTW;
int dateSettingStage = 0;
//nastavení pro relé
const int relayPin = 2;
bool relayEnabled = true;
const float minTemp = 24.0;
const float maxTemp = 25.0;
//nastavení pro klávesnici
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 13, 12, 11, 10 };
byte colPins[COLS] = { 9, 8, 7, 6 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  int address = 0;
  if (EEPROM.read(0) != 255) {
    for (int day = 0; day < DAYS_IN_WEEK; ++day) {
      for (int interval = 0; interval < TIME_INTERVALS; ++interval) {
        EEPROM.get(address, temperatureThresholds[day][interval]);
        address += sizeof(temperatureThresholds[day][interval]);
      }
    }
  } else {
    for (int day = 0; day < DAYS_IN_WEEK; ++day) {
      for (int interval = 0; interval < TIME_INTERVALS; ++interval) {
        if (interval == 0) {
          temperatureThresholds[day][interval][0] = 21.0;
          temperatureThresholds[day][interval][1] = 23.0;
        } else if (interval == 1) {
          temperatureThresholds[day][interval][0] = 19.0;
          temperatureThresholds[day][interval][1] = 21.0;
        } else if (interval == 2) {
          temperatureThresholds[day][interval][0] = 22.0;
          temperatureThresholds[day][interval][1] = 24.0;
        } else if (interval == 3) {
          temperatureThresholds[day][interval][0] = 24.0;
          temperatureThresholds[day][interval][1] = 26.0;
        }
      }
    }
  }
  int offsetAddress = DAYS_IN_WEEK * TIME_INTERVALS * sizeof(temperatureThresholds[0][0]) * 3;
  if (EEPROM.read(offsetAddress) != 255) {
    EEPROM.get(offsetAddress, temperatureOffset);
  } else {
    temperatureOffset = DEFAULT_TEMPERATURE_OFFSET;
  }
  pinMode(relayPin, OUTPUT);
  if (!bmp.begin(BMP280_ADRESA)) {
    while (1)
      ;
  }
  u8g2.begin();
}
void softwareReset() {
  wdt_enable(WDTO_15MS);
  while (1) {
  }
}
void loop() {
  myRTC.updateTime();
  const char* daysOfWeek[] = { "Pondeli", "Utery", "Streda", "Ctvrtek", "Patek", "Sobota", "Nedele" };
  const char* timeIntervals[] = { "0-5", "5-12", "12-20", "20-0" };
  int currentHour = myRTC.hours;
  int currentDayOfWeek = myRTC.dayofweek;
  int currentInterval;
  unsigned long currentMillis = millis();
  int korekce = 32;
  float teplota = bmp.readTemperature() + temperatureOffset;
  float tlak = (bmp.readPressure() / 100.00) + korekce;
  //nastavení relé
  if (relayOverride == AUTO) {
    if (currentHour < 5) currentInterval = 0;
    else if (currentHour < 12) currentInterval = 1;
    else if (currentHour < 20) currentInterval = 2;
    else currentInterval = 3;
    float currentMinTemp = temperatureThresholds[currentDayOfWeek][currentInterval][0];
    float currentMaxTemp = temperatureThresholds[currentDayOfWeek][currentInterval][1];

    if (teplota <= currentMinTemp && !relayEnabled) {
      digitalWrite(relayPin, LOW);
      relayEnabled = true;
    } else if (teplota >= currentMaxTemp && relayEnabled) {
      digitalWrite(relayPin, HIGH);
      relayEnabled = false;
    }
  } else {
    if (relayOverride == ON) digitalWrite(relayPin, LOW);
    if (relayOverride == OFF) digitalWrite(relayPin, HIGH);
  }
  // Čtení klávesnice
  char key = keypad.getKey();
  if (currentScreen == BASIC) {
    switch (key) {
      case 'B':
        currentScreen = SETTINGS;
        break;
      case 'C':
        relayOverride = static_cast<RelayOverride>((relayOverride + 1) % 3);
        break;
    }
  }
  if (currentScreen == SETTINGS) {
    switch (key) {
      case '1':
        currentScreen = SET_TIME;
        setHours = myRTC.hours;
        setMinutes = myRTC.minutes;
        setSeconds = myRTC.seconds;
        timeSettingStage = 0;
        break;
      case '3':
        currentScreen = SET_DATE;
        setDay = myRTC.dayofmonth;
        setMonth = myRTC.month;
        setYear = myRTC.year - 2000;
        setDOTW = myRTC.dayofweek;
        dateSettingStage = 0;
        break;
      case '7':
        currentScreen = SET_TEMP;
        selectedDay = 0;
        selectedInterval = 0;
        newMinTemp = 20.0;
        newMaxTemp = 25.0;
        configState = SELECT_DAY;
        break;
      case '9':
        currentScreen = SET_T_OFF;
        break;
      case '0':
        softwareReset();
        break;
      case 'A':
        currentScreen = BASIC;
        break;
    }
  }
  if (currentScreen == SET_TIME) {
    switch (key) {
      case '4':
        selectedTimePart = (selectedTimePart + 2) % 3;
        break;
      case '6':
        selectedTimePart = (selectedTimePart + 1) % 3;
        break;
      case '2':
        if (selectedTimePart == 0) setHours = (setHours + 1) % 24;
        else if (selectedTimePart == 1) setMinutes = (setMinutes + 1) % 60;
        else if (selectedTimePart == 2) setSeconds = (setSeconds + 1) % 60;
        break;
      case '8':
        if (selectedTimePart == 0) setHours = (setHours + 23) % 24;
        else if (selectedTimePart == 1) setMinutes = (setMinutes + 59) % 60;
        else if (selectedTimePart == 2) setSeconds = (setSeconds + 59) % 60;
        break;
      case 'D':
        myRTC.setDS1302Time(setSeconds, setMinutes, setHours, myRTC.dayofweek, myRTC.dayofmonth, myRTC.month, myRTC.year);
        currentScreen = SETTINGS;
        break;
    }
  }
  if (currentScreen == SET_DATE) {
    switch (key) {
      case '4':
        selectedDatePart = (selectedDatePart + 3) % 4;
        break;
      case '6':
        selectedDatePart = (selectedDatePart + 1) % 4;
        break;
      case '2':
        if (selectedDatePart == 0) setDay = (setDay % 31) + 1;
        else if (selectedDatePart == 1) setMonth = (setMonth % 12) + 1;
        else if (selectedDatePart == 2) setYear = (setYear % 99) + 1;
        else if (selectedDatePart == 3) setDOTW = (setDOTW % 7) + 1;
        break;
      case '8':
        if (selectedDatePart == 0) setDay = (setDay + 29) % 31 + 1;
        else if (selectedDatePart == 1) setMonth = (setMonth + 10) % 12 + 1;
        else if (selectedDatePart == 2) setYear = (setYear + 97) % 99 + 1;
        else if (selectedDatePart == 3) setDOTW = (setDOTW + 5) % 7 + 1;
        break;
      case 'D':
        int fullYear = 2000 + setYear;
        myRTC.setDS1302Time(myRTC.seconds, myRTC.minutes, myRTC.hours, setDOTW, setDay, setMonth, fullYear);  // Toto je jen příklad, aktualizujte podle vaší knihovny RTC
        currentScreen = SETTINGS;
        break;
    }
  }
  if (currentScreen == SET_TEMP) {
    switch (key) {
      case '4':
        configState = (ConfigState)((configState + 3) % 4);
        break;
      case '6':
        configState = (ConfigState)((configState + 1) % 4);
        break;
      case '2':
        if (configState == SELECT_DAY) selectedDay = (selectedDay + 1) % DAYS_IN_WEEK;
        else if (configState == SELECT_INTERVAL) selectedInterval = (selectedInterval + 1) % TIME_INTERVALS;
        else if (configState == SET_MIN_TEMP) temperatureThresholds[selectedDay][selectedInterval][0] += 0.5;
        else if (configState == SET_MAX_TEMP) temperatureThresholds[selectedDay][selectedInterval][1] += 0.5;
        break;
      case '8':
        if (configState == SELECT_DAY) selectedDay = (selectedDay + DAYS_IN_WEEK - 1) % DAYS_IN_WEEK;
        else if (configState == SELECT_INTERVAL) selectedInterval = (selectedInterval + TIME_INTERVALS - 1) % TIME_INTERVALS;
        else if (configState == SET_MIN_TEMP) temperatureThresholds[selectedDay][selectedInterval][0] -= 0.5;
        else if (configState == SET_MAX_TEMP) temperatureThresholds[selectedDay][selectedInterval][1] -= 0.5;
        break;
      case 'D':
        int address = 0;
        for (int day = 0; day < DAYS_IN_WEEK; ++day) {
          for (int interval = 0; interval < TIME_INTERVALS; ++interval) {
            EEPROM.put(address, temperatureThresholds[day][interval]);
            address += sizeof(temperatureThresholds[day][interval]);
          }
        }
        currentScreen = SETTINGS;
        break;
    }
  }
  if (currentScreen == SET_T_OFF) {
    switch (key) {
      case '2':
        temperatureOffset += 0.1;
        break;
      case '8':
        temperatureOffset -= 0.1;
        break;
      case 'D':
        int offsetAddress = DAYS_IN_WEEK * TIME_INTERVALS * sizeof(temperatureThresholds[0][0]) * 3;
        EEPROM.put(offsetAddress, temperatureOffset);
        currentScreen = SETTINGS;
        break;
    }
  }
  if (currentMillis - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentMillis;
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_ncenB08_tr);
      switch (currentScreen) {
        case BASIC:
          u8g2.setCursor(0, 15);
          u8g2.print(teplota, 1);
          u8g2.print(" °C, ");
          u8g2.print(tlak, 1);
          u8g2.print(" hPa, ");
          u8g2.print("T: ");
          u8g2.print(relayEnabled ? "ON" : "OFF");
          u8g2.setCursor(0, 30);
          u8g2.print(myRTC.hours);
          u8g2.print(":");
          u8g2.print(myRTC.minutes);
          u8g2.print(":");
          u8g2.print(myRTC.seconds);
          u8g2.print(", ");
          u8g2.print(daysOfWeek[myRTC.dayofweek]);
          u8g2.print("/");
          u8g2.print(timeIntervals[currentInterval]);
          u8g2.setCursor(0, 45);
          u8g2.print("RT: ");
          switch (relayOverride) {
            case AUTO:
              u8g2.print("AUTO");
              break;
            case ON:
              u8g2.print("ON");
              break;
            case OFF:
              u8g2.print("OFF");
              break;
          }
          break;

        case SETTINGS:
          u8g2.drawStr(0, 15, "             Nastaveni");
          u8g2.drawStr(0, 30, "1. Cas           3. Datum");
          u8g2.drawStr(0, 45, "7. Teploty   9. T-OFF");
          u8g2.drawStr(0, 60, "0. Restart");

          break;
        case SET_TIME:
          u8g2.setCursor(0, 15);
          u8g2.print("Nastavit cas: ");
          u8g2.setCursor(0, 30);

          if (selectedTimePart == 0) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          } else {
            u8g2.setFont(u8g2_font_ncenB08_tr);
          }
          u8g2.print(setHours);

          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.print(":");

          if (selectedTimePart == 1) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          } else {
            u8g2.setFont(u8g2_font_ncenB08_tr);
          }
          u8g2.print(setMinutes);

          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.print(":");

          if (selectedTimePart == 2) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          } else {
            u8g2.setFont(u8g2_font_ncenB08_tr);
          }
          u8g2.print(setSeconds);
          break;

        case SET_DATE:
          u8g2.setCursor(0, 15);
          u8g2.print("Nastavit datum: ");
          u8g2.setCursor(0, 30);

          if (selectedDatePart == 0) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          } else {
            u8g2.setFont(u8g2_font_ncenB08_tr);
          }
          u8g2.print(setDay);

          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.print("/");

          if (selectedDatePart == 1) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          } else {
            u8g2.setFont(u8g2_font_ncenB08_tr);
          }
          u8g2.print(setMonth);

          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.print("/");

          if (selectedDatePart == 2) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          } else {
            u8g2.setFont(u8g2_font_ncenB08_tr);
          }
          u8g2.print(setYear);

          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.print("/");

          if (selectedDatePart == 3) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          } else {
            u8g2.setFont(u8g2_font_ncenB08_tr);
          }
          u8g2.print(setDOTW);
          break;
        case SET_TEMP:
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.setCursor(0, 15);
          u8g2.print("Den: ");
          if (configState == SELECT_DAY) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          }
          u8g2.print(daysOfWeek[selectedDay]);
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.setCursor(0, 30);
          u8g2.print("Interval: ");
          if (configState == SELECT_INTERVAL) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          }
          u8g2.print(timeIntervals[selectedInterval]);
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.setCursor(0, 45);
          u8g2.print("Min: ");
          if (configState == SET_MIN_TEMP) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          }
          u8g2.print(temperatureThresholds[selectedDay][selectedInterval][0]);
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.setCursor(0, 60);
          u8g2.print("Max: ");
          if (configState == SET_MAX_TEMP) {
            u8g2.setFont(u8g2_font_ncenB14_tr);
          }
          u8g2.print(temperatureThresholds[selectedDay][selectedInterval][1]);
          break;
        case SET_T_OFF:
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.setCursor(0, 15);
          u8g2.print("Nastavte offset: ");
          u8g2.setCursor(0, 30);
          u8g2.setFont(u8g2_font_ncenB14_tr);
          u8g2.print(temperatureOffset, 1);
          u8g2.print(" C");
          break;
      }
    } while (u8g2.nextPage());
  }
}
