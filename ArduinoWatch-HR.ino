// PulseSensorPlayground ライブラリ
#define USE_ARDUINO_INTERRUPTS true 
#include <PulseSensorPlayground.h>  
const int PulseWire = 34;             // 紫のケーブルをつないだ端子番号（A0-26）
const int LED13 = 13;                 // Arduino上のLED13
int Threshold = 550;                  // 心拍として計測するためのしきい値。環境に応じてこの値を変えて下さい。 
PulseSensorPlayground pulseSensor;  // pulseSensorインスタンスの作成

void setup() {
  Serial.begin(115200); // シリアルモニタ開始

  // PulseSensorオブジェクトの作成 
  pulseSensor.analogInput(PulseWire);
  pulseSensor.blinkOnPulse(LED13);
  // PulseSensorオブジェクトの開始 
  if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object !");  //スタート時1度だけ出力されます。  
  }
  Serial.println();
}

void loop() {
 // 心拍センサーの開始
  int myBPM = pulseSensor.getBeatsPerMinute();  // PulseSensorオブジェクトの呼び出し                   
  if (pulseSensor.sawStartOfBeat()) {           // Constantly test to see if "a beat happened". 
    Serial.println("♥  A HeartBeat Happened ! "); // If test is "true", print a message "a heartbeat happened".
    Serial.print("BPM:");
    Serial.println(myBPM);
    Serial.print("\n");
  }
  delay(20);
}
