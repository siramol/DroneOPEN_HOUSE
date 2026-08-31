
//THERE IS NO WARRANTY FOR THE SOFTWARE, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR 
//OTHER PARTIES PROVIDE THE SOFTWARE “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
//OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE SOFTWARE IS WITH THE CUSTOMER. SHOULD THE 
//SOFTWARE PROVE DEFECTIVE, THE CUSTOMER ASSUMES THE COST OF ALL NECESSARY SERVICING, REPAIR, OR CORRECTION EXCEPT TO THE EXTENT SET OUT UNDER THE HARDWARE WARRANTY IN THESE TERMS.

// ===================== HOW TO USE THIS FILE =====================
// 1. ปิดไฟ/ถอดแบตเตอรี่ที่จ่ายให้ ESC ก่อน (ESP32 เสียบ USB ได้ตามปกติ)
// 2. อัปโหลดโค้ดนี้ แล้วเปิด Serial Monitor (115200) ทิ้งไว้
// 3. เมื่อเห็นข้อความ ">>> เสียบไฟ ESC ตอนนี้! <<<" ให้เสียบแบตเตอรี่ ESC ทันที
// 4. รอฟังเสียง "บี๊บ" จาก ESC ยืนยันว่ารับค่า MAX แล้ว
// 5. โค้ดจะสลับไปส่งค่า MIN เองอัตโนมัติ รอฟังเสียงบี๊บอีกครั้ง -> คาลิเบรตเสร็จ
// 6. หลังจากนั้นโค้ดจะเข้าสู่โหมดปกติ ให้ลองขยับจอยเพื่อทดสอบมอเตอร์
// ===================================================================

#include <ESP32Servo.h>

Servo mot1;
Servo mot2;
Servo mot3;
Servo mot4;
const int mot1_pin = 13;
const int mot2_pin = 12;
const int mot3_pin = 14;
const int mot4_pin = 27;

volatile uint32_t current_time;
volatile uint32_t last_channel_1 = 0;
volatile uint32_t last_channel_2 = 0;
volatile uint32_t last_channel_3 = 0;
volatile uint32_t last_channel_4 = 0;
volatile uint32_t last_channel_5 = 0;
volatile uint32_t last_channel_6 = 0;
volatile uint32_t timer_1;
volatile uint32_t timer_2;
volatile uint32_t timer_3;
volatile uint32_t timer_4;
volatile uint32_t timer_5;
volatile uint32_t timer_6;
volatile int ReceiverValue[6];
const int channel_1_pin = 34;
const int channel_2_pin = 35;
const int channel_3_pin = 32;
const int channel_4_pin = 33;
const int channel_5_pin = 25;
const int channel_6_pin = 26;

uint32_t LoopTimer;

float InputThrottle;

// Shared between Core 1 (writer) and Core 0 (reader/motor task).
volatile float MotorInput1, MotorInput2, MotorInput3, MotorInput4;

// ===================== Calibration state machine =====================
// 0 = ยังไม่เริ่ม, 1 = กำลังส่ง MAX (รอเสียบไฟ+บี๊บ), 2 = กำลังส่ง MIN, 3 = จบแล้ว/โหมดปกติ
volatile int calState = 0;
const uint32_t CAL_MAX_HOLD_MS = 8000; // เวลาค้าง MAX ให้พอเสียบไฟ+ได้ยินบี๊บ
const uint32_t CAL_MIN_HOLD_MS = 4000; // เวลาค้าง MIN ให้ ESC บันทึกค่า

void IRAM_ATTR channelInterruptHandler()
{
  current_time = micros();
  if (digitalRead(channel_1_pin)) { if (last_channel_1 == 0) { last_channel_1 = 1; timer_1 = current_time; } }
  else if (last_channel_1 == 1) { last_channel_1 = 0; ReceiverValue[0] = current_time - timer_1; }

  if (digitalRead(channel_2_pin)) { if (last_channel_2 == 0) { last_channel_2 = 1; timer_2 = current_time; } }
  else if (last_channel_2 == 1) { last_channel_2 = 0; ReceiverValue[1] = current_time - timer_2; }

  if (digitalRead(channel_3_pin)) { if (last_channel_3 == 0) { last_channel_3 = 1; timer_3 = current_time; } }
  else if (last_channel_3 == 1) { last_channel_3 = 0; ReceiverValue[2] = current_time - timer_3; }

  if (digitalRead(channel_4_pin)) { if (last_channel_4 == 0) { last_channel_4 = 1; timer_4 = current_time; } }
  else if (last_channel_4 == 1) { last_channel_4 = 0; ReceiverValue[3] = current_time - timer_4; }

  if (digitalRead(channel_5_pin)) { if (last_channel_5 == 0) { last_channel_5 = 1; timer_5 = current_time; } }
  else if (last_channel_5 == 1) { last_channel_5 = 0; ReceiverValue[4] = current_time - timer_5; }

  if (digitalRead(channel_6_pin)) { if (last_channel_6 == 0) { last_channel_6 = 1; timer_6 = current_time; } }
  else if (last_channel_6 == 1) { last_channel_6 = 0; ReceiverValue[5] = current_time - timer_6; }
}

// ===================== Core 0 task: motor writing + calibration sequence =====================
// รันทุกอย่างที่เกี่ยวกับ mot.write()/attach() บน Core 0 ทั้งหมด
// เพื่อไม่ให้ critical section ของ ESP32Servo ไปชนกับ interrupt รับสัญญาณบน Core 1
void motorTask(void *pvParameters)
{
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  mot1.attach(mot1_pin, 1000, 2000);
  mot2.attach(mot2_pin, 1000, 2000);
  mot3.attach(mot3_pin, 1000, 2000);
  mot4.attach(mot4_pin, 1000, 2000);

  // -------- ขั้นที่ 1: ส่ง MAX ค้างไว้ ให้เสียบไฟ ESC ตอนนี้ --------
  calState = 1;
  Serial.println();
  Serial.println("=====================================================");
  Serial.println(" ขั้นตอนคาลิเบรต ESC เริ่มแล้ว");
  Serial.println(" >>> เสียบไฟ ESC ตอนนี้! <<< (กำลังส่งค่า MAX = 2000us)");
  Serial.println(" รอฟังเสียงบี๊บจาก ESC ยืนยันว่ารับค่า MAX แล้ว");
  Serial.println("=====================================================");
  uint32_t t0 = millis();
  while (millis() - t0 < CAL_MAX_HOLD_MS)
  {
    mot1.writeMicroseconds(2000);
    mot2.writeMicroseconds(2000);
    mot3.writeMicroseconds(2000);
    mot4.writeMicroseconds(2000);
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  // -------- ขั้นที่ 2: สลับเป็น MIN ให้ ESC บันทึกค่า --------
  calState = 2;
  Serial.println();
  Serial.println(" กำลังส่งค่า MIN = 1000us -> รอฟังเสียงบี๊บอีกครั้ง (ยืนยัน MIN)");
  t0 = millis();
  while (millis() - t0 < CAL_MIN_HOLD_MS)
  {
    mot1.writeMicroseconds(1000);
    mot2.writeMicroseconds(1000);
    mot3.writeMicroseconds(1000);
    mot4.writeMicroseconds(1000);
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  // -------- คาลิเบรตเสร็จ: เข้าสู่โหมดปกติ ควบคุมตามจอย --------
  calState = 3;
  Serial.println();
  Serial.println(" คาลิเบรตเสร็จแล้ว! ESC บันทึกค่า MIN/MAX ไว้แล้ว");
  Serial.println(" เข้าสู่โหมดทดสอบปกติ (ควบคุมด้วยจอย CH3 = throttle)");
  Serial.println("=====================================================");
  Serial.println();

  for (;;)
  {
    mot1.writeMicroseconds((int)MotorInput1);
    mot2.writeMicroseconds((int)MotorInput2);
    mot3.writeMicroseconds((int)MotorInput3);
    mot4.writeMicroseconds((int)MotorInput4);

    vTaskDelay(pdMS_TO_TICKS(4)); // ~250Hz update rate for the motors
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(channel_1_pin, INPUT_PULLUP);
  pinMode(channel_2_pin, INPUT_PULLUP);
  pinMode(channel_3_pin, INPUT_PULLUP);
  pinMode(channel_4_pin, INPUT_PULLUP);
  pinMode(channel_5_pin, INPUT_PULLUP);
  pinMode(channel_6_pin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(channel_1_pin), channelInterruptHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(channel_2_pin), channelInterruptHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(channel_3_pin), channelInterruptHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(channel_4_pin), channelInterruptHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(channel_5_pin), channelInterruptHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(channel_6_pin), channelInterruptHandler, CHANGE);

  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  digitalWrite(15, HIGH);
  delay(500);
  digitalWrite(15, LOW);
  delay(500);
  digitalWrite(15, HIGH);
  delay(500);
  digitalWrite(15, LOW);
  delay(500);

  MotorInput1 = 1000;
  MotorInput2 = 1000;
  MotorInput3 = 1000;
  MotorInput4 = 1000;

  // Pin the motor-writing + calibration task to Core 0. setup()/loop()
  // run on Core 1 by default on Arduino-ESP32, keeping the two separated.
  xTaskCreatePinnedToCore(
      motorTask,   // task function
      "motorTask", // name
      4096,        // stack size (bytes)
      NULL,        // parameters
      1,           // priority
      NULL,        // task handle
      0            // core 0
  );

  LoopTimer = micros();
}

void loop()
{
  InputThrottle = ReceiverValue[2];

  float m1 = InputThrottle;
  float m2 = InputThrottle;
  float m3 = InputThrottle;
  float m4 = InputThrottle;

  if (m1 > 2000) m1 = 1999;
  if (m2 > 2000) m2 = 1999;
  if (m3 > 2000) m3 = 1999;
  if (m4 > 2000) m4 = 1999;

  if (m1 < 1000) m1 = 1000;
  if (m2 < 1000) m2 = 1000;
  if (m3 < 1000) m3 = 1000;
  if (m4 < 1000) m4 = 1000;

  // ค่านี้จะถูกใช้จริงก็ต่อเมื่อ calState == 3 (คาลิเบรตเสร็จแล้ว) เท่านั้น
  // ระหว่างคาลิเบรต (calState 1,2) motorTask ควบคุม PWM เองโดยตรง ไม่แตะค่านี้
  MotorInput1 = m1;
  MotorInput2 = m2;
  MotorInput3 = m3;
  MotorInput4 = m4;

  // แสดงสถานะคาลิเบรตกำกับไว้ในบรรทัด Serial ปกติ จะได้รู้ว่าตอนนี้อยู่ขั้นไหน
  Serial.print(ReceiverValue[0]);
  Serial.print(" - ");
  Serial.print(ReceiverValue[1]);
  Serial.print(" - ");
  Serial.print(ReceiverValue[2]);
  Serial.print(" - ");
  Serial.print(ReceiverValue[3]);
  Serial.print(" - ");
  Serial.print(ReceiverValue[4]);
  Serial.print(" - ");
  Serial.print(ReceiverValue[5]);
  Serial.print(" -- calState=");
  Serial.print(calState);
  Serial.print(" -- ");

  Serial.print("  ");
  Serial.print(MotorInput1);
  Serial.print("  ");
  Serial.print(MotorInput2);
  Serial.print("  ");
  Serial.print(MotorInput3);
  Serial.print("  ");
  Serial.print(MotorInput4);
  Serial.println("   ");

  while (micros() - LoopTimer < 4000);
  LoopTimer = micros();
}
