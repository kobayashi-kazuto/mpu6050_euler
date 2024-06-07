#include <Arduino.h>

volatile int interruptCounter;//カウンタ変数
int totalInterruptCounter;//割込み数のカウンタ
 
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
	Serial.begin(115200);
	delay(2000);
	Serial.println("*** setup start ***");
 
	timer = timerBegin(0, 40, true);//タイマの初期化(何番のタイマか(0~3),プリスケーラ(クロックは40MHzなので指定した数で割った信号が得られる今回だと1MHzになるはず),trueで割込みをエッジタイプに)
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
		 
		Serial.print("An interrupt as occurred. Total number: ");
		Serial.println(totalInterruptCounter);
 
	}
}

// ---------------------------------------------------------------