#define BLYNK_TEMPLATE_ID ""                         //ใส่ ID Template ของตัว Blynk
#define BLYNK_TEMPLATE_NAME ""                 //ใส่ ชื่อ Template ของตัว Blynk
#define BLYNK_AUTH_TOKEN ""       //ใส่ Token Template ของตัว Blynk
#define LINE_TOKEN ""  //ใส่ Token ของตัวการแจ้งเตือนของ LINE

//ใช้งาน Library ต่างๆที่จำเป็นต่องานนี้
#include <WiFiManager.h>           //Library การจัดการ Wifi
#include <ESP8266WiFi.h>           //Library ของ ESP8266WiFi
#include <BlynkSimpleEsp8266.h>    //Library การเชื่อมต่อกับ Blynk
#include <TridentTD_LineNotify.h>  //Library ของตัว LINE NOTIFY
#include <DHT.h>                   //Library DHT คือตัววัดอุณหภูมิ

#define DHTPIN D5      //การกำหนดขาให้ตัวรับอุณหภูมิ
#define DHTTYPE DHT11  //การกำหนดประเภทให้ตัวรับอุณหภูมิ
#define RELAY_PIN D6   //การกำหนดประเภทให้ตัว Relay เอาไว้เปิด-ปิดน้ำ

DHT dht(DHTPIN, DHTTYPE);  //สร้างตัวแปรของตัววัดอุณหภูมิ ให้มารวมกันในชื่อ DHT

const int buttonPin = D3;                //ใช้งาน Digital pin ที่3 (คือปุ่ม Flash ที่อยู่บนบอร์ด ESP8266)
const int ledPin = D4;                   //ใช้งาน LED Pin ที่4 (คือไฟของที่อยู่บนบอร์ด ESP8266)
bool buttonState = LOW;                  //กำหนดให้ชื่อตัวแปร buttonState มันเป็นค่า LOW
unsigned long buttonPressStartTime = 0;  //กำหนดให้ชื่อตัวแปร buttonPressStartTime ให้เป็น 0
const int soilMoisturePin = A0;          //กำหนดให้ soilMoisturePin หรือก็คือตัววัดความชื้นในดินมันอ่านค่าโดยใช้ Analog Pin A0

void setup() {
  pinMode(RELAY_PIN, OUTPUT);    //สร้างการกำหนด PinMode RELAY_PIN
  digitalWrite(RELAY_PIN, LOW);  //สั่งการให้ตัว RELAY ปิดการใช้งานก่อน

  //(เริ่มต้นที่มีชื่อคำว่า Serial คือการ Log ข้อมูล หรือที่เรียกว่ารายงานโชว์ข้อมูลออกมาทาง Serial Monitor)
  Serial.begin(115200);            //ในที้นี้คือการ Set ให้ Serial Monitor มันเรียกดู Band ที่ 115200 ถึงจะมองเห็นข้อมูลที่ Log ออกมา
  LINE.setToken(LINE_TOKEN);       //set line token ของตัวการแจ้งเตือนผ่านไลน์
  Blynk.config(BLYNK_AUTH_TOKEN);  //set Blynk token ในการใช้งาน Blynk

  WiFiManager wm;  //เรียกใช้ WiFiManager และกำหนดให้อยู่ในชื่อตัวแปรของคำว่า wm

  //ในส่วนนี้คือการตั้งค่า IP ให้ Wifi ที่ปล่อยออกมาจากตัว ESP8266 เป็นการกำหนดบังคับเลข IP ไม่ให้หลุดไปเลขอื่น
  IPAddress staticIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  wm.setAPStaticIPConfig(staticIP, gateway, subnet);

  if (!wm.autoConnect("!Plant_wartering?", "11111111")) {  //กำหนดชื่อ รหัสผ่านของ Wifi ESP8266
    Serial.println("เชื่อมต่อผิดพลาด");                        //ถ้าหาก เชื่อมต่อ Wifi ของตัวที่ ESP8266 ปล่อยออกมาผิดพลาด มันจะโชว์คำว่า เชื่อมต่อผิดพลาด
  } else {
    Serial.println("เชื่อมต่อสำเร็จแล้ว");                  //ถ้าเกิดว่ามันเชื่อมต่อได้แล้วจะโชว์คำนี้ เชื่อมต่อสำเร็จแล้ว ใน Serial Monitor
    LINE.notify("เชื่อมต่อกับระบบรดน้ำต้นไม้ของคุณสำเร็จแล้ว");  //ทำการส่งการแจ้งเตือนเข้าไปใน LINE เมื่อเชื่อมต่อสำเร็จ
    Blynk.connect();                                   //ทำการเชื่อมต่อกับ บริการของ Blynk
    Blynk.virtualWrite(V0, "1");                       //ในหน้าจอของ Blynk กำหนดให้ Switch มันเลื่อนเปิด ก็คือ Switch การเชื่อมต่อ Wifi
  }

  //กำหนด Pin Mode ของ buttonPin กับ ledPin
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  dht.begin();  //ใช้งานตัวแปรของตัววัดอุณหภูมิ
}

//กำหนดตัวแปรของ ค่าความชื้น กับ ค่าอุณหภูมิ ให้เป็น 0.0
float previousHumidity = 0.0;
float previousTemperature = 0.0;

int previoussoilMoisture = 0;
bool isNotify = false;
bool isNotify2 = false;

void loop() {

  //เมื่อตัว ESP8266 เริ่มทำงานให้ใช้งานโค้ดพวกนี้
  Blynk.run();                                       //ให้ Blynk เริ่มทำงาน
  bool currentButtonState = digitalRead(buttonPin);  //สร้างตัวแปรเพิ่มเติมในการที่จะเอาเช็คปุ่ม Flash บนบอร์ด ESP8266 ในชื่อ currentButtonState

  //เมื่อปุ่มถูกกดจะบันทึกเวลาที่เริ่ม กดปุ่มลงใน buttonPressStartTime ด้วย buttonPressStartTime = millis();
  if (currentButtonState == LOW && buttonState == HIGH) {
    buttonPressStartTime = millis();
  }

  //เมื่อปุ่มถูกปล่อย (currentButtonState == LOW และ buttonState == LOW) และผ่านไปเวลามากกว่าหรือเท่ากับ 3000 milliseconds หรือเมื่อกดค้างเป็นเวลา 3 วินาที จะทำคำสั่งภายในเงื่อนไข
  if (currentButtonState == LOW && buttonState == LOW && millis() - buttonPressStartTime >= 3000) {
    Serial.println("Disconnecting WiFi...");  //ปริ้นข้อความ "Disconnecting WiFi..." ผ่าน Serial Monitor
    LINE.notify("ยกเลิกการเชื่อมต่อ..");          //แจ้งเตือนผ่าน LINE ว่ายกเลิกการเชื่อมต่อ
    Blynk.virtualWrite(V1, "ความชื้น: --");     //เซ็ตค่าการแสดงผลบน Blynk ให้เป็นค่าว่างหรือ 0
    Blynk.virtualWrite(V2, "อุณหภูมิ: --");      //เซ็ตค่าการแสดงผลบน Blynk ให้เป็นค่าว่างหรือ 0
    Blynk.virtualWrite(A0, 0);                //เซ็ตค่าการแสดงผลบน Blynk ให้เป็นค่าว่างหรือ 0
    Blynk.disconnect();                       //ยกเลิกการเชื่อมต่อ Blynk
    WiFi.disconnect(true);                    //ยกเลิกการเชื่อมต่อ Wifi
    digitalWrite(ledPin, LOW);                //เปลี่ยนสถานะของ LED เป็น LOW
    digitalWrite(RELAY_PIN, LOW);             //เปลี่ยนสถานะของ RELAY เป็น LOW
    ESP.restart();                            //รีสตาร์ท ESP8266
  }
  buttonState = currentButtonState;

  //อ่านค่าความชื้นและอุณหภูมิจากเซ็นเซอร์ DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  //นำค่าที่ได้มาเปรียบเทียบกับค่าก่อนหน้าเพื่อตรวจสอบว่ามีการเปลี่ยนแปลงหรือไม่ และถ้ามีการเปลี่ยนแปลง จะทำการอัพเดทค่าบน Blynk และทำการแสดงผลทาง Serial Monitor และ LINE

  if (!isnan(humidity) && !isnan(temperature)) {
    // delay(1000);
    if (humidity != previousHumidity || temperature != previousTemperature) {
      previousHumidity = humidity;
      previousTemperature = temperature;
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");
      LINE.notify("ค่าความชื้น: " + String(humidity) + " %\nอุณหภูมิ: " + String(temperature) + " °C");  //แจ้งเตือนค่าความชื้น และ อุณหภูมิ เข้าไปใน LINE
      Blynk.virtualWrite(V1, "ความชื้น: " + String(humidity) + " %");                                //แจ้งเตือนค่าความชื้น เข้าไปใน Blynk
      Blynk.virtualWrite(V2, "อุณหภูมิ: " + String(temperature) + " °C");                             //แจ้งเตือนอุณหภูมิ เข้าไปใน Blynk
    }
  }

  //กำหนดให้ค่าเริ่มต้นของ ตัววัดความชื้นในดินมันเป็น 0
  int soilMoistureValue = analogRead(soilMoisturePin);  //การใช้งาน analogRead คือการอ่านค่าของตัววัดความชื้นในดิน soilMoisturePin

  //ตรวจสอบค่าความชื้นในดิน (soilMoistureValue) เพื่อดูว่ามีการเปลี่ยนแปลงหรือไม่ และถ้ามีการเปลี่ยนแปลง จะทำการแสดงค่าผ่าน Serial Monitor และ LINE และทำการอัพเดทค่าบน Blynk
  if (soilMoistureValue != previoussoilMoisture) {
    previoussoilMoisture = soilMoistureValue;
    if (!isNotify2) {
      LINE.notify("ความชื้นในดิน: " + String(soilMoistureValue));
      isNotify2 = true;
    }
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoistureValue);
    LINE.notify("ความชื้นในดิน: " + String(soilMoistureValue));  //แจ้งเตือนความชื้นในดินเข้าไปใน LINE
    isNotify2 = false;
    Blynk.virtualWrite(A0, soilMoistureValue);  //แจ้งเตือนความชื้นในดินเข้าไปใน Blynk

    if (soilMoistureValue >= 800) {
      digitalWrite(RELAY_PIN, HIGH);
      if (!isNotify) {
        LINE.notify("เปิดวาล์วน้ำอัตโนมัติ");
        isNotify = true;
      }
      Blynk.virtualWrite(V3, 1);
    } else {
      digitalWrite(RELAY_PIN, LOW);
      if (!isNotify) {
        LINE.notify("ปิดวาล์วน้ำแล้ว..");
        isNotify = true;
      }
      Blynk.virtualWrite(V3, 0);
    }
    isNotify = false;
  }
}


//การปิด Switch ของการเชื่อมต่อ Wifi ในแอพ Blynk ให้ทำการยกเลิกการเชื่อมต่อ Wifi แจ้งเตือนเข้าไปใน LINE และ reset ค่าความชื้น กับ อุณหภูมิ และ ค่าความชื้นในดินให้เป็นค่าว่างหรือ 0
BLYNK_WRITE(V0) {
  int value = param.asInt();
  if (value == 0) {  //เมื่อกดปิดการเชื่อมต่อ Wifi
    Serial.println("Disconnecting WiFi...");
    LINE.notify("ยกเลิกการเชื่อมต่อ..");       //ส่งการแจ้งเตือนเข้าไปใน LINE
    Blynk.virtualWrite(V1, "ความชื้น: --");  //Reset ค่าให้เป็นค่าว่างใน Blynk
    Blynk.virtualWrite(V2, "อุณหภูมิ: --");   //Reset ค่าให้เป็นค่าว่างใน Blynk
    Blynk.virtualWrite(A0, 0);             //Reset ค่าให้เป็นค่า 0 ใน Blynk
    Blynk.disconnect();                    //ตัดการเชื่อมต่อของ Blnk
    WiFi.disconnect(true);                 //ตัดการเชื่อมต่อของ WiFi
    digitalWrite(ledPin, LOW);             //กำหนดให้ไฟบนบอร์ด ESP8266 ดับ
    digitalWrite(RELAY_PIN, LOW);          //ปิดการใช้งาน RELAY
    ESP.restart();                         //ทำการ Restart บอร์ด ESP8266
  }
}


//การบังคับการใช้งาน Relay หรือเปิด-ปิดน้ำ
BLYNK_WRITE(V3) {
  int valve = param.asInt();
  if (valve == 1) {  //เมื่อกดเปิดการใช้งาน Relay หรือเปิดน้ำ
    // delay(500); //delay 0.5 วินาที
    digitalWrite(RELAY_PIN, HIGH);  //กำหนดให้ Relay ทำงาน
    LINE.notify("เปิดวาล์วน้ำแล้ว💧");   //แจ้งเตือนเข้าไปใน LINE ว่า เปิดวาล์วน้ำแล้ว
  } else {                          //เมื่อกดปิด
    // delay(500); //delay 0.5 วินาที
    digitalWrite(RELAY_PIN, LOW);  //กำหนดให้ Relay หยุดทำงาน
    LINE.notify("ปิดวาล์วน้ำแล้ว..");  //แจ้งเตือนเข้าไปใน LINE ว่า ปิดวาล์วน้ำแล้ว
  }
}