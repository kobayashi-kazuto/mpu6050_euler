#include <Arduino.h>
#include<Wire.h>

volatile int interruptCounter;//カウンタ変数
int totalInterruptCounter;//割込み数のカウンタ
const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

hw_timer_t * timer = NULL;//タイマ設定用のポインタ
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;//同期処理用の宣言?

 
// ---------------------------------------------------------------
void IRAM_ATTR onTimer() {
	portENTER_CRITICAL_ISR(&timerMux);
	interruptCounter++;
	portEXIT_CRITICAL_ISR(&timerMux);
}
 
// ---------------------------------------------------------------
void setup()
{
	Wire.begin(21,22);
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x6B);  // PWR_MGMT_1 register
    Wire.write(0);     // set to zero (wakes up the MPU-6050)
    Wire.endTransmission(true);
	Serial.begin(115200);
	delay(200);
	Serial.println("*** setup start ***");
 
	timer = timerBegin(0, 80, true);//タイマの初期化(何番のタイマか(0~3),プリスケーラ(クロックは80MHzなので指定した数で割った信号が得られる今回だと1MHzになるはず),trueで割込みをエッジタイプに)
	timerAttachInterrupt(timer, &onTimer, true);//第三引数はそろえる
	timerAlarmWrite(timer, 1000000, true);//第二引数でカウント何回で割込みするか指定(1Mごとだから1秒ごと)
	timerAlarmEnable(timer);

	Serial.println("*** setup end ***"); 
}
 
// ---------------------------------------------------------------
void loop()
{
 
	if (interruptCounter > 0) {
 
		portENTER_CRITICAL(&timerMux);
		interruptCounter--;
		portEXIT_CRITICAL(&timerMux);
 
		totalInterruptCounter++;
		 
		//Serial.print("An interrupt as occurred. Total number: ");
		//Serial.println(totalInterruptCounter);


		//1Hz

		Wire.beginTransmission(MPU_addr);
		Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  		Wire.endTransmission(false);
  		Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
  		AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
  		AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  		AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  		Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  		GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  		GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  		GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)



		if(totalInterruptCounter%10 == 0){
			Serial.print("AcX = "); Serial.print(AcX);
  			Serial.print(" | AcY = "); Serial.print(AcY);
  			Serial.print(" | AcZ = "); Serial.print(AcZ);
  			Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);  //equation for temperature in degrees C from datasheet
  			Serial.print(" | GyX = "); Serial.print(GyX);
  			Serial.print(" | GyY = "); Serial.print(GyY);
  			Serial.print(" | GyZ = "); Serial.println(GyZ);
		}
 
	}
}

// ---------------------------------------------------------------