#include <Arduino.h>
#include<Wire.h>
//Z-Y-X系の
volatile int interruptCounter;//カウンタ変数
int totalInterruptCounter;//割込み数のカウンタ
const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
float acceleration_x = 0;
float acceleration_y = 0;
float acceleration_z = 0;

float rad_velocity_roll = 0;
float rad_velocity_pitch = 0;
float rad_velocity_yaw = 0;

float rad_velocity_roll_offset = 0;
float rad_velocity_yaw_offset = 0;
float rad_velocity_pitch_offset = 0;

int16_t rad_velocity_roll_1 = 0;
int16_t rad_velocity_pitch_1 = 0;
int16_t rad_velocity_yaw_1 = 0;

float rad_roll = 0;
float rad_pitch = 0;
float rad_yaw = 0;

float rad_roll_offset = 0;
float rad_pitch_offset = 0;
float rad_yaw_offset = 0;

float deg_roll = 0;
float deg_pitch = 0;
float deg_yaw = 0;

float acc_rad_roll = 0;
float acc_rad_pitch = 0;
float acc_deg_roll = 0;
float acc_deg_pitch = 0;

float conv_radv = 2000;

int32_t GyX_total = 0;
int32_t GyY_total = 0;
int32_t GyZ_total = 0;

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
	timerAlarmWrite(timer, 500, true);//第二引数でカウント何回で割込みするか指定(1Mごとだから2000Hz)
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


		//2kHz

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

		GyX = GyX - rad_velocity_roll_offset;
		GyY = GyY - rad_velocity_pitch_offset;
		GyZ = GyZ - rad_velocity_yaw_offset;

		rad_velocity_roll_1 = rad_velocity_roll;
		rad_velocity_pitch_1 = rad_velocity_pitch;
		rad_velocity_yaw_1 = rad_velocity_yaw;

		rad_velocity_roll = (GyX + GyY*sin(rad_roll)*tan(rad_pitch) + GyZ*cos(rad_roll)*tan(rad_pitch))/conv_radv;
		rad_velocity_pitch = (GyY*cos(rad_roll) - GyZ*sin(rad_roll))/conv_radv;
		rad_velocity_yaw = (GyY*sin(rad_roll)/cos(rad_pitch) +GyZ*cos(rad_roll)/cos(rad_pitch))/conv_radv;

		
		if(totalInterruptCounter <= 200){
			GyX_total += GyX;
			GyY_total += GyY;
			GyZ_total += GyZ;
		}

		
		if(totalInterruptCounter == 200){
			rad_velocity_roll_offset = GyX_total/200;
			rad_velocity_pitch_offset = GyY_total/200;
			rad_velocity_yaw_offset = GyZ_total/200;
			rad_roll = 0;
			rad_pitch = 0;
			rad_yaw = 0;
		}

		//if(totalInterruptCounter%2 == 0){
			rad_roll += (rad_velocity_roll +rad_velocity_roll_1)/2000/2;
			rad_pitch += (rad_velocity_pitch + rad_velocity_pitch_1)/2000/2;
			rad_yaw += (rad_velocity_yaw + rad_velocity_yaw_1)/2000/2;

			acc_rad_roll = atan2(AcY,AcZ);
			acc_rad_pitch = atan2(-AcX,sqrt(AcY*AcY + AcZ*AcZ));

			acc_deg_roll = acc_rad_roll*180/M_PI;
			acc_deg_pitch = acc_rad_pitch*180/M_PI;

			deg_roll = rad_roll*180/M_PI;
			deg_pitch = rad_pitch*180/M_PI;
			deg_yaw = rad_yaw*180/M_PI;


		//}

		if(totalInterruptCounter%20 == 0){
			Serial.print("AcX = "); Serial.print(acc_deg_roll);
  			Serial.print(" | AcY = "); Serial.print(acc_deg_pitch);
  			Serial.print(" | GyX = "); Serial.print(deg_roll);
  			Serial.print(" | GyY = "); Serial.print(deg_pitch);
  			Serial.print(" | GyZ = "); Serial.println(deg_yaw);
		}
 
	}
}

// ---------------------------------------------------------------