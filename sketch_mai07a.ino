#include <GPRS_Shield_Arduino.h> //подключаем библиотеку для работы с GSM модемом
#include <SoftwareSerial.h> //подключаем библиотеку для работы с последовательным портом
#include <EEPROM.h> // подключаем библиотеку для работы с энергонезависимой памятью

#include "DHT.h" //Подключаем библиотеку для работы с датчиком температуры и влаги

#define DHTPIN 12 //Цифровой вход к которому подключен датчик, в данном случае  пин D12

#define DHTTYPE DHT21 //Здесь выбираем какой у нас датчик.


DHT dht(DHTPIN, DHTTYPE); //датчик температуры и влаги


SoftwareSerial mySerial(7,8); //объявляем последовательный порт
GPRS gprs(mySerial); //объявляем gsm модем

//строки для формирования http запроса
String quesGercon = "&d=";
String quesHumidity = "&h=";
String quesId = "&id=";
String endHttppara = "\"";
String contain = "AT+HTTPPARA=\"URL\",\"178.176.220.161/arduino.php?t=";

const int GERCON_PIN = 10; //порт с датчиком открытых дверей 
const int GSMPOWER = 9; //порт управления питанием модема

const String id = "1"; //идентификатор устройства в базе данных
int timeReboot = 0;

int gerconStatus; // статус открытой/закрытой двери
float humidity; // влажность
float temperature; //температура

//подключение к GSM сети и инициализация HTTP протокола
void initGSM(){
  //ждем когда модем станет доступен для микроконтроллера
  while(!gprs.init()){
    delay(1000);
  }
  mySerial.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""); // задаем контекст - GPRS
  delay(4000);
  mySerial.println("AT+SAPBR=3,1,\"APN\",\"internet\""); // настройки APN для Megafon
  delay(4000);
  mySerial.println("AT+SAPBR=3,1,\"USER\",\"gdata\""); // имя пользователя 
  delay(4000);
  mySerial.println("AT+SAPBR=3,1,\"PWD\",\"gdata\""); //пароль
  delay(4000);
  mySerial.println("AT+SAPBR=1,1"); //применить настройки APN
  delay(4000);
  mySerial.println("AT+SAPBR=2,1"); //получить IP адресс
  delay(4000);
  mySerial.println("AT+HTTPINIT"); //инициализировать HTTP протокол
  delay(4000);
  mySerial.println("HTTPPARA=\"CID\",1");
  delay(4000);
}

 
 
void setup()
{
  Serial.begin(9600); //инициализировать последовательный порт для отладки
  int licenseKey = EEPROM.read(0); //считать из энергонезависимой памяти лицензионый ключ
  /*проверка ключа
   * если ключ неверный, то программа уйдет в бесконечный цикл
   */
  if(licenseKey!=195){
    Serial.println("Нелицензионное устройство, запуск невозможен");
    while(1){
      
    }
  }
  dht.begin(); //инициализация датчика температуры и влаги
  pinMode(GERCON_PIN,INPUT); //инициализация датчика открытых дверей
  pinMode(GSMPOWER,OUTPUT); //инициализация порта, управляющего питанием GSM модема
  //включение модема
  digitalWrite(GSMPOWER,HIGH);
  delay(2000);
  digitalWrite(GSMPOWER,LOW);
  delay(2000);
  
  mySerial.begin(9600); //инициализировать последовательный порт для управления модемом
  initGSM(); //инициализировать модем
}
 
void loop()
{
  delay(3000); //тайм-аут опроса датчиков
  temperature = dht.readTemperature(); //получить температуру
  humidity = dht.readHumidity(); //получить влажность
  //если датчик не вернул значения, то указать аварийные значения
  if(isnan(temperature)||isnan(humidity)){
    temperature=999;
    humidity=999;
  }
  gerconStatus = !digitalRead(GERCON_PIN); //получить инвертированное состояние открытия двери
  //формирование HTTP запроса
  contain += temperature + quesGercon + gerconStatus + quesHumidity + humidity + quesId + id;
  delay(100);
  contain += endHttppara;
  delay(100);
  
  mySerial.println(contain); //отправка запроса на модем
  delay(250);
  mySerial.println("AT+HTTPACTION=0"); //указываем метод GET, отправка запроса на сервер
  delay(500);
  contain = "AT+HTTPPARA=\"URL\",\"178.176.220.161/arduino.php?t=";
  timeReboot++;
  if(timeReboot>150){
    timeReboot = 0;
    //перезапуск модема
    digitalWrite(GSMPOWER,HIGH);
    delay(2000);
    digitalWrite(GSMPOWER,LOW);
    delay(2000);
    digitalWrite(GSMPOWER,HIGH);
    delay(2000);
    digitalWrite(GSMPOWER,LOW);
    delay(8000);
    
    initGSM();//инициализация модема
  }
}
