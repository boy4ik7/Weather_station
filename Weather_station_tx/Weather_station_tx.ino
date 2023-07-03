// передача структуры данных

//#define G433_SPEED 1000   // скорость 100-10000 бит/с, по умолч. 2000 бит/с
#define G433_SLOW // отправляю раз в секунду на SYN480R

#include <Adafruit_BMP085.h>
#include <Gyver433.h>
Gyver433_TX<7> tx;  // указали пин

// формат пакета для отправки
struct DataPack {
  int8_t Temperature_out;
  int32_t Pressure_out;
  int8_t precipitation;  
};
DataPack data;

unsigned long pressure, aver_pressure, pressure_array[6], time_array[6];
float a;

Adafruit_BMP085 bmp;

void setup() {
  bmp.begin();
    pressure = aver_sens();          // найти текущее давление по среднему арифметическому
  for (byte i = 0; i < 6; i++) {   // счётчик от 0 до 5
    pressure_array[i] = pressure;  // забить весь массив текущим давлением
    time_array[i] = i;             // забить массив времени числами 0 - 5
  }
  delay(100);
}

void loop() {
pressure = aver_sens();                          // найти текущее давление по среднему арифметическому
    for (byte i = 0; i < 5; i++) {                   // счётчик от 0 до 5 (да, до 5. Так как 4 меньше 5)
      pressure_array[i] = pressure_array[i + 1];     // сдвинуть массив давлений КРОМЕ ПОСЛЕДНЕЙ ЯЧЕЙКИ на шаг назад
    }
    pressure_array[5] = pressure;                    // последний элемент массива теперь - новое давление
    unsigned long sumX, sumY, sumX2, sumXY;
    sumX = 0;
    sumY = 0;
    sumX2 = 0;
    sumXY = 0;
    for (int i = 0; i < 6; i++) {    // для всех элементов массива
      sumX += time_array[i];
      sumY += (long)pressure_array[i];
      sumX2 += time_array[i] * time_array[i];
      sumXY += (long)time_array[i] * pressure_array[i];
    }
    a = 0;
    a = (long)6 * sumXY;  // расчёт коэффициента наклона приямой
    a = a - (long)sumX * sumY;
    a = (float)a / (6 * sumX2 - sumX * sumX);
    int angle, delta;
    delta = a * 6; 
    angle = map(delta, -250, 250, 100, 0);  // пересчитать в проценты
 //   angle = constrain(angle, 0, 100);      // ограничить диапазон
  data.Temperature_out = bmp.readTemperature();                 // температура
  data.Pressure_out = bmp.readPressure();   // давление
  data.precipitation = angle;
  tx.sendData(data);
  delay(1000);
}

long aver_sens() {
  pressure = 0;
  for (byte i = 0; i < 10; i++) {
    pressure += bmp.readPressure();
  }
  aver_pressure = pressure / 10;
  return aver_pressure;
}
