#include <ESP32Servo.h>

Servo mot1;
Servo mot2;
Servo mot3;
Servo mot4;

const int mot1_pin = 13;
const int mot2_pin = 12; // แนะนำให้ระวังขา Strapping Pin ตอนอัปโหลด
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

float InputThrottle;
float MotorInput1, MotorInput2, MotorInput3, MotorInput4;

void channelInterruptHandler()
{
  current_time = micros();
  
  // Channel 1
  if (digitalRead(channel_1_pin))
  {
    if (last_channel_1 == 0) { last_channel_1 = 1; timer_1 = current_time; }
  }
  else if (last_channel_1 == 1) { last_channel_1 = 0; ReceiverValue[0] = current_time - timer_1; }

  // Channel 2
  if (digitalRead(channel_2_pin))
  {
    if (last_channel_2 == 0) { last_channel_2 = 1; timer_2 = current_time; }
  }
  else if (last_channel_2 == 1) { last_channel_2 = 0; ReceiverValue[1] = current_time - timer_2; }

  // Channel 3 (Throttle)
  if (digitalRead(channel_3_pin))
  {
    if (last_channel_3 == 0) { last_channel_3 = 1; timer_3 = current_time; }
  }
  else if (last_channel_3 == 1) { last_channel_3 = 0; ReceiverValue[2] = current_time - timer_3; }

  // Channel 4
  if (digitalRead(channel_4_pin))
  {
    if (last_channel_4 == 0) { last_channel_4 = 1; timer_4 = current_time; }
  }
  else if (last_channel_4 == 1) { last_channel_4 = 0; ReceiverValue[3] = current_time - timer_4; }

  // Channel 5
  if (digitalRead(channel_5_pin))
  {
    if (last_channel_5 == 0) { last_channel_5 = 1; timer_5 = current_time; }
  }
  else if (last_channel_5 == 1) { last_channel_5 = 0; ReceiverValue[4] = current_time - timer_5; }

  // Channel 6
  if (digitalRead(channel_6_pin))
  {
    if (last_channel_6 == 0) { last_channel_6 == 1; timer_6 = current_time; }
  }
  else if (last_channel_6 == 1) { last_channel_6 = 0; ReceiverValue[5] = current_time - timer_6; }
}

void neutralPositionAdjustment()
{
  int min = 1490;
  int max = 1510;
  if (ReceiverValue[0] < max && ReceiverValue[0] > min) { ReceiverValue[0] = 1500; } 
  if (ReceiverValue[1] < max && ReceiverValue[1] > min) { ReceiverValue[1] = 1500; } 
  if (ReceiverValue[3] < max && ReceiverValue[3] > min) { ReceiverValue[3] = 1500; } 
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(15, OUTPUT);
  digitalWrite(15, LOW);
  delay(200);
  digitalWrite(15, HIGH);
  delay(200);
  digitalWrite(15, LOW);

  // ตั้งค่าพินรับสัญญาณรีโมท (ขา 34, 35 เป็น Input ล้วน ไม่มี Internal Pullup)
  pinMode(channel_1_pin, INPUT);
  pinMode(channel_2_pin, INPUT);
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
  
  // จอง Timer สำหรับ Servo library บน ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  mot1.setPeriodHertz(50);
  mot2.setPeriodHertz(50);
  mot3.setPeriodHertz(50);
  mot4.setPeriodHertz(50);

  mot1.attach(mot1_pin, 1000, 2000);
  mot2.attach(mot2_pin, 1000, 2000);
  mot3.attach(mot3_pin, 1000, 2000);
  mot4.attach(mot4_pin, 1000, 2000);

  // สั่งค่าต่ำสุดเพื่อ Arm ESC ตอนเริ่มต้น
  mot1.writeMicroseconds(1000);
  mot2.writeMicroseconds(1000);
  mot3.writeMicroseconds(1000);
  mot4.writeMicroseconds(1000);
  
  delay(2000); // รอให้ ESC พร้อม
}

void loop() {
  neutralPositionAdjustment();

  // ดึงค่าจากช่อง 3 (ReceiverValue[2]) มาเป็น Throttle
  InputThrottle = ReceiverValue[2];

  // ป้องกันค่าหลุดช่วงความปลอดภัย
  if (InputThrottle < 900)  InputThrottle = 1000; // กรณีสัญญาณหลุด
  if (InputThrottle > 2000) InputThrottle = 2000;

  // ส่งค่า Throttle ไปยังมอเตอร์ทั้ง 4 ตัว
  MotorInput1 = InputThrottle;
  MotorInput2 = InputThrottle;
  MotorInput3 = InputThrottle;
  MotorInput4 = InputThrottle;

  // สั่งงานมอเตอร์ด้วยฟังก์ชัน microseconds (เสถียรและแม่นยำกว่า map เป็นองศา)
  mot1.writeMicroseconds(MotorInput1);
  mot2.writeMicroseconds(MotorInput2);
  mot3.writeMicroseconds(MotorInput3);
  mot4.writeMicroseconds(MotorInput4);

  // พิมพ์ค่าตรวจสอบทาง Serial Monitor
  Serial.print("RC: ");
  for(int i=0; i<6; i++) {
    Serial.print(ReceiverValue[i]);
    Serial.print("\t");
  }
  Serial.print(" | Motor: ");
  Serial.println(InputThrottle);

  delay(20); // หน่วงเวลาเล็กน้อยเพื่อความเสถียรในการพิมพ์ Serial
}