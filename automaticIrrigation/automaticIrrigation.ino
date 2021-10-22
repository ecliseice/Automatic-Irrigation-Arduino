#include <Wire.h>
#include <LCD_1602_RUS.h>
#include <DS1307RTC.h>
#include <TimeLib.h>
#include "GyverEncoder.h"

LCD_1602_RUS lcd(0x27, 16, 2);
int DS1307_I2C_ADDRESS = 0x68;

//порты энкодера
#define CLK 2
#define DT 3
#define SW 4
Encoder enc1(CLK, DT, SW, TYPE1);

#define RELE 7

const char* MonthDict[] = {
  "Янв", "Фев", "Мар", "Апр", "Май", "Июн",
  "Июл", "Авг", "Сен", "Окт", "Ноя", "Дек"
};

int is_working = false;

int MAIN_MENU_PAGE = 0;
int SUB_MENU_PAGE = 0;
bool EDIT_MODE = false;
int WATER_HOUR_ADDRESS = 22;
int WATER_MINUTE_ADDRESS = 23;
int WATER_TIME_ADDRESS = 24;

unsigned long last_clear = millis();
unsigned long last_action = millis();
unsigned long working_time;

tmElements_t tm;

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial) ;
  lcd.init();
  lcd.setBacklight(true);
  pinMode(RELE, OUTPUT);
  digitalWrite(RELE, HIGH );

  //  tm.Hour = 12;
  //  tm.Minute = 28;
  //  tm.Second = 53;
  //  RTC.write(tm);

  //set custom time
  //writeAddress(WATER_HOUR_ADDRESS, 8);
  //writeAddress(WATER_MINUTE_ADDRESS, 55);
  //  writeAddress(WATER_TIME_ADDRESS, 2);
}

void loop() {
  enc1.tick();
  if (RTC.read(tm)) {
    showLcdData();
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }
    delay(9000);
  }

  if (millis() - last_action > 20000) {
//    lcd.setBacklight(false);
  }
}

void showLcdData() {
  if (MAIN_MENU_PAGE == 0) {
    showMainPage();
  } else if (SUB_MENU_PAGE == 0) {
    mainMenuSwitch();
  } else {
    subMenuSwitch();
  }
  if (millis() - last_action > 10000) {
    exitToMain();
  }
}

void exitToMain() {
  MAIN_MENU_PAGE = 0;
  SUB_MENU_PAGE = 0;
}

void showMainPage() {

  printTime(1, 0);
  lcd.print(" ");
  printDate();

  printMemoriedTime(1, 1);

  //время работы полива
  int time = readAddress(WATER_TIME_ADDRESS);
  lcd.print("  ");
  lcd.print(time);
  lcd.print("мин");

  lcd.setCursor(15, 1);
  if (is_working) lcd.print("*");

  if (enc1.isClick()) {
    MAIN_MENU_PAGE++;
    lcd.clear();
    last_action = millis();
    lcd.setBacklight(true);
    delay(500);
  }
  if (enc1.isTurn()) {
    lcd.setBacklight(true);
    last_action = millis();
  }


  //проверить время для полива
  int memored_time = readAddress(WATER_HOUR_ADDRESS) * 60 + readAddress(WATER_MINUTE_ADDRESS);
  int current_time = tm.Hour * 60 + tm.Minute;
  if (memored_time - current_time == 0 && !is_working) {
    startWorking();
  }

  int working_duration = readAddress(WATER_TIME_ADDRESS);
  
  if (is_working && 
    (
      (memored_time - current_time == -working_duration) ||
      ((millis() - working_time) / 60000 >= working_duration)
    )
  ) {
    stopWorking();
  }


  if (millis() - last_clear > 10000) {
    lcd.clear();
    last_clear = millis();
  }
}

void mainMenuSwitch() {
  if (enc1.isRight()) {
    MAIN_MENU_PAGE++;
    lcd.clear();
    last_action = millis();
  }
  if (enc1.isLeft()) {
    MAIN_MENU_PAGE--;
    lcd.clear();
    last_action = millis();
  }

  switch (MAIN_MENU_PAGE) {
    case 1:
      lcd.setCursor(1, 0);
      lcd.print("Настройки");
      lcd.setCursor(1, 1);
      lcd.print("полива");
      break;
    case 2:
      lcd.setCursor(1, 0);
      lcd.print("Настройки");
      lcd.setCursor(1, 1);
      lcd.print("времени");
      lcd.setCursor(1, 0);
      break;
    case 3:
      lcd.setCursor(1, 0);
      lcd.print("Запустить");
      lcd.setCursor(1, 1);
      lcd.print("полив");
      lcd.setCursor(1, 0);
      break;
    default:
      MAIN_MENU_PAGE = 0;
  }
  if (enc1.isHolded()) {
    MAIN_MENU_PAGE = 0;
    lcd.clear();
    last_action = millis();
    delay(1000);
  }

  if (MAIN_MENU_PAGE != 0 && enc1.isClick()) {
    SUB_MENU_PAGE = 1;
    lcd.clear();
    last_action = millis();
  }
}

void subMenuSwitch() {
  if (!EDIT_MODE) {
    if (enc1.isRight()) {
      SUB_MENU_PAGE++;
      lcd.clear();
      last_action = millis();
    }
    if (enc1.isLeft()) {
      SUB_MENU_PAGE--;
      lcd.clear();
      last_action = millis();
    }
  }
  int value;
  switch (MAIN_MENU_PAGE) {
    case 1:
      lcd.setCursor(1, 0);
      lcd.print("Полив -> ");
      switch (SUB_MENU_PAGE) {
        case 1:
          lcd.print("Часы");
          lcd.setCursor(7, 1);
          value = readAddress(WATER_HOUR_ADDRESS);
          lcd.print(value);

          if (EDIT_MODE) {
            changeWaterTime("hour", WATER_HOUR_ADDRESS);
          }
          break;
        case 2:
          lcd.print("Минуты");
          lcd.setCursor(7, 1);
          value = readAddress(WATER_MINUTE_ADDRESS);
          lcd.print(value);

          if (EDIT_MODE) {
            changeWaterTime("minute", WATER_MINUTE_ADDRESS);
          }
          break;
        case 3:
          lcd.print("Время");
          lcd.setCursor(7, 1);
          value = readAddress(WATER_TIME_ADDRESS);
          lcd.print(value);

          if (EDIT_MODE) {
            changeWaterTime("time", WATER_TIME_ADDRESS);
          }
          break;
        default:
          SUB_MENU_PAGE = 1;
      }
      break;
    case 2:
      lcd.setCursor(1, 0);
      lcd.print("Время -> ");
      switch (SUB_MENU_PAGE) {
        case 1:
          lcd.print("Часы");
          lcd.setCursor(7, 1);
          value = tm.Hour;
          lcd.print(value);

          if (EDIT_MODE) {
            changeTime("hour");
          }
          break;
        case 2:
          lcd.print("Минуты");
          lcd.setCursor(7, 1);
          value = tm.Minute;
          lcd.print(value);

          if (EDIT_MODE) {
            changeTime("minute");
          }
          break;
        default:
          SUB_MENU_PAGE = 1;
      }
      break;
    case 3:
      lcd.setCursor(1, 0);
      lcd.print("Запуск -> Уверены?");
      lcd.setCursor(7, 1);
      lcd.print("OK");
      if (enc1.isClick()) {
        startWorking();
        exitToMain();
      }
      break;
  }
  //если вырано не меню принудительного запуска
  if (enc1.isClick() && MAIN_MENU_PAGE != 3) {
    EDIT_MODE = !EDIT_MODE;
    lcd.clear();
    last_action = millis();
  }

  if (enc1.isHolded()) {
    SUB_MENU_PAGE = 0;
    lcd.clear();
    last_action = millis();
    delay(1000);
  }
}

void changeWaterTime(String type, int address) {
  lcd.setCursor(0, 1);
  lcd.print(">");

  int data = readAddress(address);

  if (enc1.isRight()) {
    data++;
    if (type == "hour" && data > 23) data = 0;
    else if (type == "minute" && data > 59) data = 0;
    lcd.clear();
  }
  if (enc1.isLeft()) {
    data--;
    if (type == "hour" && data < 0) data = 23;
    else if (type == "minute" && data < 0) data = 59;
    lcd.clear();
  }
  last_action = millis();
  writeAddress(address, data);
}

void changeTime(String type) {
  lcd.setCursor(0, 1);
  lcd.print(">");

  int data;
  if (type == "hour") data = tm.Hour;
  else if (type == "minute") data = tm.Minute;

  if (enc1.isRight()) {
    data++;
    if (type == "hour" && data > 23) data = 0;
    else if (type == "minute" && data > 59) data = 0;
    lcd.clear();
  }
  if (enc1.isLeft()) {
    data--;
    if (type == "hour" && data < 0) data = 23;
    else if (type == "minute" && data < 0) data = 59;
    lcd.clear();
  }
  if (type == "hour") tm.Hour = data;
  else if (type == "minute") tm.Minute = data;

  last_action = millis();

  RTC.write(tm);
}

void printMemoriedTime(int col, int row) {
  lcd.setCursor(col, row);

  //  int hour = print2digits(readAddress(WATER_HOUR_ADDRESS), false);
  print2digits(readAddress(WATER_HOUR_ADDRESS), false);
  //  lcd.print(hour);
  //  lcd.print(":");
  print2digits(readAddress(WATER_MINUTE_ADDRESS), true);
  //  lcd.print(minutes);
}

void printDate() {
  lcd.print((int)tm.Day);
  lcd.print("/");

  lcd.print(MonthDict[(int)tm.Month - 1]);
  //    lcd.print("/");

  //    lcd.print(tmYearToCalendar(tm.Year) - 2000);
}

void printTime(int col, int row) {
  lcd.setCursor(col, row);

  print2digits(tm.Hour, false);
  print2digits(tm.Minute, false);
  print2digits(tm.Second, true);

}

void print2digits(int number, bool last) {
  if (number >= 0 && number < 10) {
    lcd.print("0");
  }
  lcd.print(number);
  if (!last) lcd.print(":");
}

void writeAddress(int address, byte val)
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(address);

  Wire.write(val);
  Wire.endTransmission();
}

byte readAddress(int address)
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 1);

  return (int)Wire.read();
}

void startWorking() {
  working_time = millis();
  is_working = true;
  digitalWrite(RELE, LOW);
}

void stopWorking() {
  is_working = false;
  digitalWrite(RELE, HIGH);
}
