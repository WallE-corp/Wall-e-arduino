#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <MeAuriga.h>

MeEncoderOnBoard Encoder_1(SLOT1);
MeEncoderOnBoard Encoder_2(SLOT2);
MeLightSensor lightsensor_12(12);
MeLineFollower linefollower_8(8);
MeUltrasonicSensor ultrasonic_10(10);
MeGyro gyro_0(0, 0x69);

String object = "No object detected";

void isr_process_encoder1(void)
{
  if(digitalRead(Encoder_1.getPortB()) == 0){
    Encoder_1.pulsePosMinus();
  }else{
    Encoder_1.pulsePosPlus();
  }
}
void isr_process_encoder2(void)
{
  if(digitalRead(Encoder_2.getPortB()) == 0){
    Encoder_2.pulsePosMinus();
  }else{
    Encoder_2.pulsePosPlus();
  }
}
void move(int direction, int speed)
{
  int leftSpeed = 0;
  int rightSpeed = 0;
  if(direction == 1){
    leftSpeed = -speed;
    rightSpeed = speed;
  }else if(direction == 2){
    leftSpeed = speed;
    rightSpeed = -speed;
  }else if(direction == 3){
    leftSpeed = -speed;
    rightSpeed = -speed;
  }else if(direction == 4){
    leftSpeed = speed;
    rightSpeed = speed;
  }
  Encoder_1.setTarPWM(leftSpeed);
  Encoder_2.setTarPWM(rightSpeed);
}
void Move_Forward (){
    move(1, 50 / 100.0 * 255);
}

void Turn_Left (){
    Encoder_1.setTarPWM(-1 * 50/100.0*255);
    Encoder_2.setTarPWM(0/100.0*255);
}
void Stop_moving (){
    Encoder_1.setTarPWM(0);
    Encoder_2.setTarPWM(0);
    _delay(0.5);

}
void Turn_Right (){
    Encoder_1.setTarPWM(-1 * 0/100.0*255);
    Encoder_2.setTarPWM(50/100.0*255);
}

void Move_Backward (){
    move(2, 50 / 100.0 * 255);
}
  
void _delay(float seconds) {
  if(seconds < 0.0){
    seconds = 0.0;
  }
  long endTime = millis() + seconds * 1000;
  while(millis() < endTime) _loop();
}

void setup() {
  Serial.begin(9600);
  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(CS11) | _BV(WGM12);
  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS21);
  attachInterrupt(Encoder_1.getIntNum(), isr_process_encoder1, RISING);
  attachInterrupt(Encoder_2.getIntNum(), isr_process_encoder2, RISING);
  gyro_0.begin();
  randomSeed((unsigned long)(lightsensor_12.read() * 123456));
  String mode = "";
  char command = 'O';
  char old_command = 'O';
  
  while(1) {
    if (Serial.available() > 0 ) {
      mode = Serial.readStringUntil('\n');
      command = mode[0];
    } 
    switch(command) {
      case 'A':                                           
        if(!checkObstacle()){
          checkBorder();
        }
        move(1, 40 / 100.0 * 255);
        break;
      case 'S':
        Stop_moving();
        break;
      case 'L':
        Turn_Left();
        break;
      case 'R':
        Turn_Right();
        break;
      case 'F':
        Move_Forward();
        break;
      case 'B':
        Move_Backward();
        break;
      default:
        Stop_moving();
        break;
    }
    //get angle x from gyroscope
    float x_angle = gyro_0.getAngle(3);
    //x-angle (converted to radians from degrees since math.h does cos calculations in radians
    x_angle = (x_angle * M_PI / 180);

    //get current rotational speed per minute - r/m
    float rpm_motor_1 = Encoder_1.getCurrentSpeed();
    float rpm_motor_2 = Encoder_2.getCurrentSpeed();
    float average_rpm = (rpm_motor_1 + rpm_motor_2) / 2;

    //calculate angular velocity
    float angular_velocity = average_rpm * 2 * M_PI / 60;

    //calculate velocity(cm/s)
    float velocity = angular_velocity * 0.02 * 100 * (-1);

    //distance (hypotenuse) = velocity * t (refresh rate (in seconds))
    float distance = velocity * 1;

    //calculate distance traveled in the x-axis
    float xDistance = distance * cos(x_angle);

    //calculate distance traveled in the y-axis
    float yDistance = distance * sin(x_angle);

    Serial.print(object);
    Serial.print(",");
    //send the x and y distance traveled to the raspberry pi
    Serial.print(xDistance);
    Serial.print(",");
    Serial.print(yDistance);
    Serial.println();
    
    _loop();
  }
}

void _loop() {
  Encoder_1.loop();
  Encoder_2.loop();
  gyro_0.update();
}

void loop() {
  _loop();
}

void waitUntilStopped(){
    while((Encoder_1.getCurrentSpeed() != 0))
    {
      _loop();
    }
}
bool checkObstacle(){
  if(ultrasonic_10.distanceCm() < 10){
    object = ("Object detected");
  
    move(2, 40 / 100.0 * 255);
    _delay(1);
    move(2, 0);
    move(4, 40 / 100.0 * 255);
    _delay(1);
    move(4, 0);
    waitUntilStopped();
    return true;
    }
        
  else{
    object = ("No object detected");
    return false;
  }
}

void checkBorder(){
    if(linefollower_8.readSensors() == 0.000000){
      object = ("Border detected");
      move(2, 40 / 100.0 * 255);
      _delay(1);
      move(2, 0);
      move(4, 40 / 100.0 * 255);
      _delay(1);
      move(4, 0);
      waitUntilStopped();
    }
    else {
      object = ("No object detected");
    }
}
