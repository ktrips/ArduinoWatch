// OLEDディスプレイ設定
// OLEDピン定義
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
  Serial.begin(115200); // シリアルモニタ開始
  afficheur.begin(); // OLEDディスプレイの開始
  Serial.println();
}


void loop() {

  int myBPM = 60;
  int stepcount = 1000;
  int calorie = 100;
  
  afficheur.fillScreen(BLACK); // バックグラウンド黒
  afficheur.setCursor(0,0); // cursor is in x=0 and y=15
  afficheur.setTextSize(2); // テキストサイズ
  afficheur.setTextColor(WHITE); // テキストカラー白
  afficheur.print("A-Watch"); // テキスト表示
  //afficheur.print(jp_time);
  afficheur.setCursor(0,16); // cursor is in x=0 and y=15
  afficheur.setTextColor(RED); // テキストカラー赤
  afficheur.print("BPM:");  // テキスト表示
  afficheur.print(myBPM);
  afficheur.setCursor(0,31); // cursor is in x=0 and y=15
  afficheur.setTextColor(BLUE); // テキストカラー青
  afficheur.print("STP:");  // テキスト表示
  afficheur.print(stepcount); 
  afficheur.setCursor(0,46); // cursor is in x=0 and y=15
  afficheur.setTextColor(GREEN); // テキストカラー緑
  afficheur.print("CAL:");  // テキスト表示
  afficheur.print(calorie); 

  delay(1000); // 1000 ms(1秒)毎に更新
}
