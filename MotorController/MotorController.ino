#include <Servo.h>

const int base_pin = 11, hand_pin = 4, upper_arm_pin = 7, lower_arm_pin = 8, wrist_pin = 6;

Servo base, lower_arm, upper_arm, wrist, hand;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setTimeout(5);

  base.attach(base_pin);
  lower_arm.attach(lower_arm_pin);
  upper_arm.attach(upper_arm_pin);
  hand.attach(hand_pin);
  wrist.attach(wrist_pin);

  base.write(69);
  lower_arm.write(87);
  upper_arm.write(34);
  hand.write(100);
  wrist.write(90);
}

void loop() {

  // put your main code here, to run repeatedly:
  while(Serial.read() != '>') {}
  int base_val = Serial.parseInt();
  int lower_arm_val = Serial.parseInt();
  int upper_arm_val = Serial.parseInt();
  int hand_val = Serial.parseInt();
  int wrist_val = Serial.parseInt();

  
  base.write(base_val);
  lower_arm.write(lower_arm_val);
  upper_arm.write(upper_arm_val);
  hand.write(hand_val);
  wrist.write(wrist_val);


  Serial.print("Recieved: ");
  Serial.print(base_val);
  Serial.print(" ");
  Serial.print(lower_arm_val);
  Serial.print(" ");
  Serial.print(upper_arm_val);
  Serial.print(" ");
  Serial.print(hand_val);
  Serial.print(" ");
  Serial.println(wrist_val);
}

