//#define _LCD_TYPE 1
//#include <LCD_1602_RUS_ALL.h>
#include <LiquidCrystal_I2C.h>
#include <Gyver433.h>
#include <Adafruit_AHTX0.h>
#include <Wire.h>
#include "DS1307.h"
#include <EncButton2.h>
#include <TimerMs.h>
#include "buildTime.h"
#include <EEPROM.h>

TimerMs tmr1(10000, 1, 1);
TimerMs tmr2(1000, 1, 1);
TimerMs tmr3(3000, 1, 1);
//TimerMs tmr4(120000, 1, 1);

Gyver433_RX<7, 12> rx;  //пин приемника и размер буфера
//LCD_1602_RUS lcd2(0x27, 16, 2);
LiquidCrystal_I2C lcd(0x27, 16, 2);

Adafruit_AHTX0 aht;
DS1307 clock;

#define INIT_ADDR 1023 // номер резервной ячейки
#define INIT_KEY 50 // ключ первого запуска. 0-254, на выбор
#define BTN1_PIN 3  // пин кнопки ВВЕРХ
#define BTN2_PIN 11  // пин кнопки ОК
#define BTN3_PIN 2  // пин кнопки ВНИЗ
#define tonePin 10  // пин пищалки
#define battery_min 3000 // минимальный уровень заряда батареи
#define battery_max 4200 // максимальный уровень заряда батареи
// диапазон для 3 пальчиковых/мизинчиковых батареек: 3000 - 4700
// диапазон для одной банки литиевого аккумулятора: 3000 - 4200

EncButton2<EB_BTN> btn_up(INPUT, BTN3_PIN);
EncButton2<EB_BTN> btn_down(INPUT, BTN2_PIN);
EncButton2<EB_BTN> btn_ok(INPUT, BTN1_PIN);

//данные
struct DataPack {
  int8_t Temperature_out;
  int32_t Pressure_out;
  int8_t precipitation;
};

//int Hour = 0, Min = 0, Sec = 0;
int voltage, alarm_hour = 24, alarm_min = 60;
int32_t Pressure_out = 0, Temperature_out = 0, precipitation = 0;
//float Temperature = 23, Humidity = 70;
bool alarm_status = false, lcd_info = false, screen = true;
float my_vcc_const = 1.080, mm_rt_sb = 0.00750062;
//
byte home[8] =
{
  B00000,
  B00100,
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
};
byte outsite[8] =
{
  B00000,
  B11111,
  B10101,
  B11111,
  B10101,
  B11111,
  B00000,
};
byte vlag[8] =
{
  B00000,
  B00100,
  B01110,
  B11111,
  B11111,
  B01110,
  B00000,
};
byte alarm[8] =
{
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B00100,
  B00000,
  B00000,
};
byte bat_100[8] = {
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte bat_80[8] = {
  B01110,
  B11111,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte bat_60[8] = {
  B01110,
  B11111,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111,
  B11111
};
byte bat_40[8] = {
  B01110,
  B11111,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111,
  B11111
};
byte bat_20[8] = {
  B01110,
  B11111,
  B10001,
  B10001,
  B10001,
  B10001,
  B11111,
  B11111
};

void setup() {
  //Serial.begin(9600);
   lcd.init();
   lcd.backlight();
   lcd.createChar(1, home);
   lcd.createChar(2, outsite);
   lcd.createChar(3, vlag);
   lcd.createChar(4, alarm);
   lcd.createChar(5, bat_100);
   lcd.createChar(6, bat_80);
   lcd.createChar(7, bat_60);
   lcd.createChar(8, bat_40);
   lcd.createChar(0, bat_20);
   clock.begin();
   clock.fillByYMD(BUILD_YEAR, BUILD_MONTH, BUILD_DAY);
   clock.fillByHMS(BUILD_HOUR, BUILD_MIN, BUILD_SEC);
   //clock.fillDayOfWeek(TUE);
   clock.setTime();
   aht.begin();
   tmr1.setTimerMode();
   tmr2.setTimerMode();
   tmr3.setTimerMode();
   pinMode(tonePin, OUTPUT);
   if (EEPROM.read(INIT_ADDR) != INIT_KEY) { // первый запуск
      EEPROM.write(INIT_ADDR, INIT_KEY); // записали ключ
      EEPROM.put(0, alarm_status);
      EEPROM.put(1, alarm_hour);
      EEPROM.put(2, alarm_min);
   }
   EEPROM.get(0, alarm_status);
   EEPROM.get(1, alarm_hour);
   EEPROM.get(2, alarm_min);
   //data_check();
   //tmr4.force();
}

void loop() {
  //test_key();
  //data_check();
  btn_up.tick();
  btn_down.tick();
  btn_ok.tick();
  if (btn_ok.held()) {
    if (alarm_status == true) {
      alarm_status = false;
      EEPROM.put(0, alarm_status);
    } else {
      alarm_status = true;
      EEPROM.put(0, alarm_status);
      }
    }
    if (btn_up.held()) {
      lcd.clear();
      set_time();
    }
    if (btn_down.held()) {
      lcd.clear();
      set_alarm();
    }
    info_disp();
    check_alarm();
    delay(1000);
}
void yield() {
  data_check();
}
/*
void test_key() {
  btn_up.tick();
  btn_down.tick();
  btn_ok.tick();
  btn_al.tick();
  if (btn_up.click()) {
    Serial.println("Левая");
  }
  if (btn_ok.click()) {
    Serial.println("Ок");
  }
  if (btn_down.click()) {
    Serial.println("Правая");
  }
  if (btn_al.click()) {
    Serial.println("Сенсорная");
  }
}
*/
void data_check() {
  //while (true) {
  //  if (tmr2.tick()) {
  //    break;
  //  }
    if (rx.tickWait()) {
      DataPack data;
      if (rx.readData(data)) {  // переписываем данные в неё
        Pressure_out = data.Pressure_out * mm_rt_sb;
        Temperature_out = data.Temperature_out;
        precipitation = data.precipitation;
        /*
        Serial.print("Temperature - ");
        Serial.print(data.Temperature_out);
        Serial.print(" C");
        Serial.println();
        Serial.print("Pressure - ");
        Serial.print(Pressure_out);
        Serial.print(" mm.rt.ct");
        Serial.println();
        Serial.print("Precipitation - ");
        Serial.print(data.precipitation);
        Serial.println("%");
        //lcd.clear(); 
        */
        //break;
      }
    }
  //}
}

void set_alarm() {
   int hour, minute;
   if ((alarm_hour > 23) || (alarm_min > 59)) {
      clock.getTime();
      hour = clock.hour;
      minute = clock.minute;
   } else {
      hour = alarm_hour;
      minute = alarm_min;
   }
   bool time = true;
   lcd.setCursor(0, 0);
   //lcd.print("Будиль");
   lcd.print("Budil");
   lcd.setCursor(0, 1);
   //lcd.print("ник");
   lcd.print("nik");
   tmr1.start();
   while (true) {
      btn_up.tick();
      btn_down.tick();
      btn_ok.tick();
      if ((tmr1.tick()) || (btn_down.held())) {
         lcd.print("Otmena");
         lcd.setCursor(0, 1);
         lcd.print("   ");
         tmr1.start();
         delay(1000);
         lcd.clear();
         break;
      }
      if (btn_ok.click()) {
         if (time == true) {
            time = false;
         } else {
            time = true;
         }
         tmr1.start();
      }
      if (btn_ok.held()) {
         alarm_hour = hour;
         alarm_min = minute;
         EEPROM.put(1, alarm_hour);
         EEPROM.put(2, alarm_min);
         lcd.setCursor(0, 0);
         lcd.print("OK    ");
         lcd.setCursor(0, 1);
         lcd.print("   ");
         tmr1.start();
         delay(1000);
         lcd.clear();
         tmr1.start();
         break;
      }
      lcd.setCursor(8, 0);
      if (hour < 10) {
         lcd.print("0");
      } 
      lcd.print(hour, DEC);
      lcd.print(":");
      if (minute < 10) {
         lcd.print("0");
      } 
      lcd.print(minute, DEC);
      lcd.setCursor(8, 1);
      switch (time) {
         case true:
            lcd.print("--   ");
            if (btn_down.click()) {
               hour += 1;
               if (hour > 23) {
                  hour = 0;
               }
               tmr1.start();
            }
            if (btn_up.click()) {
               hour -= 1;
               if (hour < 0) {
                  hour = 23;
               }
               tmr1.start();
            }
            break;
         case false:
            lcd.print("   --");
            if (btn_down.click()) {
               minute += 1;
               if (minute > 59) {
                  minute = 0;
               }
               tmr1.start();
            }
            if (btn_up.click()) {
               minute -= 1;
               if (minute < 0) {
                  minute = 59;
               }
               tmr1.start();
            }
            break;
      }
   }
}

void set_time() {
   clock.getTime();
   int hour = clock.hour, minute = clock.minute;
   bool time = true;
   lcd.setCursor(0, 0);
   //lcd.print("Время");
   lcd.print("Vremya");
   tmr1.start();
   while (true) {
      if ((tmr1.tick()) || (btn_up.held())) {
         lcd.setCursor(0, 0);
         lcd.print("Otmena");
         delay(1000);
         lcd.clear();
         break;
      }
      btn_up.tick();
      btn_down.tick();
      btn_ok.tick();
      if (btn_ok.click()) {
         if (time == true) {
            time = false;
         } else {
            time = true;
         }
         tmr1.start();
      }
      if (btn_ok.held()) {
         clock.fillByHMS(hour, minute, 30); // часы, минуты, секунды
         clock.setTime();
         lcd.setCursor(0, 0);
         lcd.print("OK    ");
         delay(1000);
         lcd.clear();
         tmr1.start();
         break;
      }
      lcd.setCursor(8, 0);
      if (hour < 10) {
         lcd.print("0");
      } 
      lcd.print(hour, DEC);
      lcd.print(":");
      if (minute < 10) {
         lcd.print("0");
      } 
      lcd.print(minute, DEC);
      lcd.setCursor(8, 1);
      switch (time) {
         case true:
            lcd.print("--   ");
            if (btn_down.click()) {
               hour += 1;
               if (hour > 23) {
                  hour = 0;
               }
               tmr1.start();
            }
            if (btn_up.click()) {
               hour -= 1;
               if (hour < 0) {
                  hour = 23;
               }
               tmr1.start();
            }
            break;
         case false:
            lcd.print("   --");
            if (btn_down.click()) {
               minute += 1;
               if (minute > 59) {
                  minute = 0;
               }
               tmr1.start();
            }
            if (btn_up.click()) {
               minute -= 1;
               if (minute < 0) {
                  minute = 59;
               }
               tmr1.start();
            }
            break;
      }
   }
}

void info_disp() {
   sensors_event_t humidity, temp;
   aht.getEvent(&humidity, &temp);
   //показываем информацию в доме
   lcd.setCursor(0, 0);
   lcd.write(1);
   int8_t temp_home = temp.temperature;
   lcd.print(temp_home, DEC);
   lcd.write(223);
   lcd.setCursor(6, 0);
   lcd.write(3);
   int8_t hum_home = humidity.relative_humidity;
   lcd.print(hum_home, DEC);
   lcd.print("%");
   //показываем информацию на улице
   lcd.setCursor(0, 1);
   lcd.write(2);
   lcd.print(Temperature_out, DEC);
   lcd.write(223);
   lcd.setCursor(5, 1);
   if (tmr3.tick()) {
      if (lcd_info == true) {
         lcd_info = false;
      } else {
         lcd_info = true;
      }
      check_bat();
      tmr3.start();
   }
  if (lcd_info == false) {
      if (Pressure_out < 700) {
         //lcd.print(" Шторм    ");
         lcd.print(" Shtorm   ");
      } else if (Pressure_out < 720) {
         //lcd.print(" Дождь    ");
         lcd.print(" Dozhd    ");
      } else if (Pressure_out < 740) {
         //lcd.print(" Ветер    ");
         lcd.print(" Veter    ");
      } else if (Pressure_out < 760) {
         //lcd.print(" Переменно");
         lcd.print(" Peremenno");
      } else if (Pressure_out < 770) {
         //lcd.print(" Ясно     ");
         lcd.print(" Yasno    ");
      } else if (Pressure_out < 800) {
         //lcd.print(" Сушь     ");
         lcd.print(" Sush     ");
      }
   } else {
      //lcd.print("Осадки ");
      lcd.print("Osadki ");
      lcd.print(precipitation, DEC);
      lcd.print("%");
   }
   // показываем время, дату состояние будильника
   lcd.setCursor(10, 0);
   if (alarm_status == true) {
      lcd.write(4);
   } else {
      lcd.print(" ");
   }
   clock.getTime();
   lcd.setCursor(11, 0);
   if (clock.hour < 10) {
      lcd.print("0");
   }
   lcd.print(clock.hour, DEC);
   lcd.print(":");
   if (clock.minute < 10) {
      lcd.print("0");
      }
   lcd.print(clock.minute, DEC);
   lcd.setCursor(15, 1);
   //lcd.print(voltage);
   if (voltage < 100) {
      lcd.write(5);
   } else if (voltage < 80) {
      lcd.write(6);
   } else if (voltage < 60) {
      lcd.write(7);
   } else if (voltage < 40) {
      lcd.write(8);
   } else if (voltage < 20) {
      lcd.write(0);
   }
//   }
/*
   lcd.setCursor(11, 1);
   lcd.print(Day);
   lcd.print(".");
   lcd.print(Month);
 //  lcd.print(Year);
 */
}

void check_alarm() {
   clock.getTime();
   if ((alarm_status == true) && (alarm_hour == clock.hour) && (alarm_min == clock.minute)) {
      uint8_t time = 30;  // сколько будет пищать будильник сек
      bool tone = true;
      tmr2.start();
      while (true) {
         btn_up.tick();
         btn_down.tick();
         btn_ok.tick();
         if ((btn_up.hold()) || (btn_down.hold()) || (btn_ok.hold()) || (time == 0)) {
            digitalWrite(tonePin, LOW);
            alarm_status = false;
            break;
         }
         lcd.setCursor(10, 0);
         if (tmr2.tick()) {
            if (tone == true) {
              digitalWrite(tonePin, HIGH);
              lcd.print(" ");
              tone = false;
            } else {
              digitalWrite(tonePin, LOW);
              lcd.write(4);
              tone = true;
            }
            time -= 1;
            tmr2.start();
         }
      }
   }
}

void check_bat() {
   voltage = readVcc(); // считать напряжение питания
   voltage = map(voltage, battery_min, battery_max, 0, 100);
   voltage = constrain(voltage, 0, 100);
}

long readVcc() { //функция чтения внутреннего опорного напряжения, универсальная (для всех ардуин)
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC));
  uint8_t low  = ADCL;
  uint8_t high = ADCH;
  long result = (high << 8) | low;

  result = my_vcc_const * 1023 * 1000 / result; // расчёт реального VCC
  return result; // возвращает VCC
}
