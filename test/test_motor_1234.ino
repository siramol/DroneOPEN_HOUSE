#include <ESP32Servo.h>

Servo mot1, mot2, mot3, mot4;

// กำหนดขาตามที่คุณต้องการ
const int mot1_pin = 13;
const int mot2_pin = 12; 
const int mot3_pin = 14;
const int mot4_pin = 27;

const int MIN_US = 1000;
const int TEST_US = 1100; // ความเร็วทดสอบ (ปลอดภัย แต่แรงพอให้หมุน)

// ฟังก์ชันสั่งหยุดมอเตอร์ทุกตัว
void stopAll() {
  mot1.writeMicroseconds(MIN_US);
  mot2.writeMicroseconds(MIN_US);
  mot3.writeMicroseconds(MIN_US);
  mot4.writeMicroseconds(MIN_US);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // จอง Timer ของ ESP32 ให้ PWM ทำงานเสถียรขึ้น
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  mot1.setPeriodHertz(50);
  mot2.setPeriodHertz(50);
  mot3.setPeriodHertz(50);
  mot4.setPeriodHertz(50);

  mot1.attach(mot1_pin, MIN_US, 2000);
  mot2.attach(mot2_pin, MIN_US, 2000);
  mot3.attach(mot3_pin, MIN_US, 2000);
  mot4.attach(mot4_pin, MIN_US, 2000);

  // ส่งค่าต่ำสุดไปปลดล็อค (Arm) ESC ตอนเปิดเครื่อง
  stopAll(); 

  Serial.println("\n===============================");
  Serial.println("Quadcopter Motor Test");
  Serial.println("พิมพ์คำสั่งแล้วกด Enter:");
  Serial.println("  1 = เทสต์มอเตอร์ 1 (ขา 13)");
  Serial.println("  2 = เทสต์มอเตอร์ 2 (ขา 12)");
  Serial.println("  3 = เทสต์มอเตอร์ 3 (ขา 14)");
  Serial.println("  4 = เทสต์มอเตอร์ 4 (ขา 27)");
  Serial.println("  5 = เทสต์หมุนพร้อมกัน 4 ตัว");
  Serial.println("  0 = หยุดมอเตอร์ทั้งหมด (STOP)");
  Serial.println("===============================\n");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == '1') {
      stopAll(); // หยุดตัวอื่นก่อน
      mot1.writeMicroseconds(TEST_US);
      Serial.println(">> สั่งหมุนมอเตอร์ 1 (ขา 13)");
    } 
    else if (cmd == '2') {
      stopAll();
      mot2.writeMicroseconds(TEST_US);
      Serial.println(">> สั่งหมุนมอเตอร์ 2 (ขา 12)");
    } 
    else if (cmd == '3') {
      stopAll();
      mot3.writeMicroseconds(TEST_US);
      Serial.println(">> สั่งหมุนมอเตอร์ 3 (ขา 14)");
    } 
    else if (cmd == '4') {
      stopAll();
      mot4.writeMicroseconds(TEST_US);
      Serial.println(">> สั่งหมุนมอเตอร์ 4 (ขา 27)");
    } 
    else if (cmd == '5') {
      mot1.writeMicroseconds(TEST_US);
      mot2.writeMicroseconds(TEST_US);
      mot3.writeMicroseconds(TEST_US);
      mot4.writeMicroseconds(TEST_US);
      Serial.println(">> สั่งหมุนพร้อมกัน 4 ตัว!");
    } 
    else if (cmd == '0') {
      stopAll();
      Serial.println(">> หยุดมอเตอร์ทั้งหมด");
    }
  }
}