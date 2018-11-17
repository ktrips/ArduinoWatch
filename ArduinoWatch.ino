#include <NTPClient.h>
#include <WiFi.h> // for WiFi shield
#include <WiFiUdp.h>
const char *ssid     = "xxx";
const char *password = "yyy";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
const char* host = "maker.ifttt.com";
const char* event = "awatch_steps";
const char* secretkey = "zzz";
int ifttt_flag = 0;

#define USE_ARDUINO_INTERRUPTS true    // Set-up low-level interrupts for most acurate BPM math.
#include <PulseSensorPlayground.h>     // PulseSensorPlayground ライブラリ   
//  Variables
const int PulseWire = 34;             // 紫のケーブルをつないだ端子番号（A0-26）
const int LED13 = 13;                 // Arduino上のLED13
int Threshold = 550;                  // 心拍として計測するためのしきい値。環境に応じてこの値を変えて下さい。 
PulseSensorPlayground pulseSensor;  // pulseSensorインスタンスの作成

#include <Wire.h>
// I2Cアドレスの設定
#define MPU6050_ADDR         0x68 // MPU-6050デバイスアドレス
#define MPU6050_SMPLRT_DIV   0x19 // MPU-6050レジスタアドレス
#define MPU6050_CONFIG       0x1a
#define MPU6050_ACCEL_CONFIG 0x1c
#define MPU6050_WHO_AM_I     0x75
#define MPU6050_PWR_MGMT_1   0x6b

float interval, preInterval;
float acc_x, acc_y, acc_z;
float gx, gy, gz;
int stepcount,state,laststate=0;
float total,threshold,hysteresis=0;

//I2c書き込み
void writeMPU6050(byte reg, byte data) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}
//i2C読み込み
byte readMPU6050(byte reg) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(true);
  Wire.requestFrom(MPU6050_ADDR, 1/*length*/); 
  byte data =  Wire.read();
  return data;
}

//Affectationピンの設定
const uint8_t SCLK_OLED = 5; //SCLK (SPI Clock)
const uint8_t MOSI_OLED = 18; //MOSI (Master Output Slave Input)
const uint8_t CS_OLED   = 12; //OLED ChipSelect
const uint8_t DC_OLED   = 13; //OLED DC (Data/Command)
const uint8_t RST_OLED  = 14; //OLED Reset RST

// 色の設定
#define BLACK 0x0000 // 黒
#define BLUE 0x001F // 青
#define RED 0xF800 // 赤
#define GREEN 0x07E0 // 緑
#define CYAN 0x07FF // シアン
#define MAGENTA 0xF81F // マゼンダ
#define WHITE 0xFFFF // 白

// ライブラリの設定
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
Adafruit_SSD1331 afficheur = Adafruit_SSD1331(CS_OLED, DC_OLED, MOSI_OLED, SCLK_OLED, RST_OLED);

void setup() {
  // initialization of display objcet
  afficheur.begin();
  Serial.begin(115200); // For Serial Monitor

  //Wifi & Time setting
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(32400);

  // PulseSensorオブジェクトの作成 
  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED13);
  // PulseSensorオブジェクトの開始 
  if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object !");  //スタート時1度だけ出力されます。  
  }

  // 加速度センサの設定
  Wire.begin(26, 25);
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  //正常に接続されているかの確認
  if (readMPU6050(MPU6050_WHO_AM_I) != 0x68) {
    Serial.println("\nWHO_AM_I error.");
    while (true) ;
  }
  
  //設定を書き込む
  writeMPU6050(MPU6050_SMPLRT_DIV, 0x00);   // sample rate: 8kHz/(7+1) = 1kHz
  writeMPU6050(MPU6050_CONFIG, 0x00);       // disable DLPF, gyro output rate = 8kHz
  writeMPU6050(MPU6050_ACCEL_CONFIG, 0x00); // 加速度範囲: ±2g
  writeMPU6050(MPU6050_PWR_MGMT_1, 0x01);   // disable sleep mode, PLL with X gyro

  //キャリブレーション
  Serial.print("Calculate Calibration");
  for(int i = 0; i < 3000; i++){
    int16_t raw_acc_x, raw_acc_y, raw_acc_z, raw_t;
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 14, true);
    raw_acc_x = Wire.read() << 8 | Wire.read();
    raw_acc_y = Wire.read() << 8 | Wire.read();
    raw_acc_z = Wire.read() << 8 | Wire.read();
    raw_t = Wire.read() << 8 | Wire.read();
    if(i % 1000 == 0){
      Serial.print(".");
    }
  }
  Serial.println();
}

void loop() {
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  //Time display
  timeClient.update();
  String jp_time = timeClient.getFormattedTime();
  Serial.println(jp_time);

 // 心拍センサーの設定
  int myBPM = pulseSensor.getBeatsPerMinute();  // PulseSensorオブジェクトの呼び出し                   
  if (pulseSensor.sawStartOfBeat()) {           // Constantly test to see if "a beat happened". 
    Serial.println("♥  A HeartBeat Happened ! "); // If test is "true", print a message "a heartbeat happened".
    Serial.print("BPM:");
    Serial.println(myBPM);
    Serial.print("\t");
  }
  delay(20);

  // 加速度センサの開始
  for ( int i=1;i<=100;i++ ) { 
    int error;
    float dT;
    int16_t raw_acc_x, raw_acc_y, raw_acc_z, raw_t;
    
    //レジスタアドレス0x3Bから、計14バイト分のデータを出力するようMPU6050へ指示
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 14, true);
    
    //出力されたデータを読み込み、ビットシフト演算
    raw_acc_x = Wire.read() << 8 | Wire.read();
    raw_acc_y = Wire.read() << 8 | Wire.read();
    raw_acc_z = Wire.read() << 8 | Wire.read();
    raw_t = Wire.read() << 8 | Wire.read();
    
    //単位Gへ変換
    acc_x = ((float)raw_acc_x) / 16384.0;
    acc_y = ((float)raw_acc_y) / 16384.0;
    acc_z = ((float)raw_acc_z) / 16384.0;
    // XYZ軸の合成
    float accel;
    accel = sqrt( sq(acc_x) + sq(acc_y) + sq(acc_z) );
    // 歩数カウントしきい値設定
    if (i != 100) {
      total += accel;
    } else {
      threshold = total/i;
      hysteresis = threshold / 10;
      total = 0;
    }
    // 歩数カウントしきい値判定
    if ( accel > (threshold + hysteresis) ) {
      state = true;
    } else if ( accel < (threshold - hysteresis) ) {
      state = false;
    }
    // 歩数カウント
    if (laststate == false && state == true) {
      stepcount++;
      laststate = state;
      ifttt_flag = 0;
    } else if (laststate == true && state == false) {
      laststate = state;
    }
    // カウンタ出力
    /*Serial.print("\t");
    Serial.print(i);
    Serial.print("\t");
    Serial.print(accel);
    Serial.print("\t");
    Serial.print("steps:");
    Serial.print(stepcount);
    Serial.print("\t");*/
  }
  // 歩数の出力
  Serial.print("Steps:");
  Serial.print(stepcount);
  Serial.print("\t");

  // 簡易カロリー計算
  int calorie;
  calorie = stepcount * 0.1;
  Serial.print("Cal:");
  Serial.print(calorie);
  Serial.print("\t");
  Serial.print("\n");
  
  afficheur.fillScreen(BLACK); // バックグラウンド黒
  afficheur.setCursor(0,0); // cursor is in x=0 and y=15
  afficheur.setTextSize(2); // テキストサイズ
  afficheur.setTextColor(WHITE); // テキストカラー白
  //afficheur.print("A-Watch"); // テキスト表示
  afficheur.print(jp_time);

  afficheur.setCursor(0,16); // cursor is in x=0 and y=15
  afficheur.setTextColor(RED); // テキストカラー赤
  afficheur.print("BPM:"); // display text
  afficheur.print(myBPM); // display text
  
  afficheur.setCursor(0,31); // cursor is in x=0 and y=15
  afficheur.setTextColor(BLUE); // テキストカラー青
  afficheur.print("STP:"); // display text
  afficheur.print(stepcount); // display text

  afficheur.setCursor(0,46); // cursor is in x=0 and y=15
  afficheur.setTextColor(GREEN); // テキストカラー緑
  afficheur.print("CAL:"); // display text
  afficheur.print(calorie); // display text

  if (stepcount%100 == 0 && ifttt_flag == 0) {
    // IFTTTに投げるURIを定義します。
    String url = "/trigger/";
    url += event;
    url += "/with/key/";
    url += secretkey;
    url += "?value1=";
    url += String(jp_time);
    url += "&value2=";
    url += String(stepcount);
    url += "&value3=";
    url += String(myBPM);
    Serial.print("Requesting URL: ");
    Serial.println(url);
  
    // IFTTTにリクエストを送信します。
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
    delay(10);
    // リプライをプリントします。
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    ifttt_flag = 1;
  }
  delay(1000); // 1000 ms(1秒)毎に更新
  //afficheur.fillScreen(BLACK); // バックグラウンド黒
  //delay(1000); // 1000 ms(1秒)毎に更新
}
