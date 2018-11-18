// MPU-6050加速度センサー設定
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

void setup() {
  Serial.begin(115200); // シリアルモニタ開始

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
    } else if (laststate == true && state == false) {
      laststate = state;
    }
    // カウンタ出力
    Serial.print(i);
    Serial.print("\t");
    Serial.print(accel);
    Serial.print("\t");
    Serial.print("steps:");
    Serial.print(stepcount);
    Serial.print("\n");
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
  Serial.print("\n");
}
