
#include <AccelStepper.h>
#include <MultiStepper.h>
// #define L_U 0
// #define L_D 1  // -
// #define R_U 2  // -
// #define R_D 3  //
#define BUT_STOP 30
#define BUT_START 31
#define L_U 0
#define L_D 1 // -
#define R_U 2 // -
#define R_D 3 //
// Định nghĩa chân điều khiển
const int dir[4] = {8, 11, 21, 18};
const int step[4] = {9, 12, 20, 17};
const int en[4] = {10, 13, 19, 16};
long pos[4] = {0, 0, 0, 0};
long new_pos[4];
long last_pos[4] = {0, 0, 0, 0};
// Khởi tạo các đối tượng stepper
float x_start_begin=70;
float y_start_begin=40;
const float length_to_step = 100.0;
const float step_size = 1.0;      // Step size for movement
float x_now = x_start_begin;
float y_now = y_start_begin; // Current position
float height = 115;
float point_arr[][2]={
  {40,30},
  {40,60},
  {50,60},
  {50,30},
  {60,30},
  {60,60},
  {x_start_begin,y_start_begin}
};
float width = 136;
float width_square = 26.6;
float height_square = 20.2;
float square_half[2] = {width_square / 2.0, height_square / 2.0}; // Half dimensions of the square
float origin_points[4][2] = {
    {0.0, 0.0},
    {0.0, height},
    {width, height},
    {width, 0.0}}; // Origin points of the square
bool is_moving = false;
void calculateCableLengths(float x, float y, float lengthNew[4])
{
  float newSquarePoints[4][2] = {
      {x - square_half[0], y - square_half[1]},
      {x - square_half[0], y + square_half[1]},
      {x + square_half[0], y + square_half[1]},
      {x + square_half[0], y - square_half[1]}};

  for (int i = 0; i < 4; i++)
  {
    lengthNew[i] = sqrt(
        pow(newSquarePoints[i][0] - origin_points[i][0], 2) +
        pow(newSquarePoints[i][1] - origin_points[i][1], 2));
  }
}

void getNewLength(float s0[4], float s1[4], float result[4])
{
  for (int i = 0; i < 4; i++)
  {
    result[i] = s1[i] - s0[i];
  }
}


AccelStepper steppers[4] = {
    AccelStepper(AccelStepper::DRIVER, step[0], dir[0]),
    AccelStepper(AccelStepper::DRIVER, step[1], dir[1]),
    AccelStepper(AccelStepper::DRIVER, step[2], dir[2]),
    AccelStepper(AccelStepper::DRIVER, step[3], dir[3])};
MultiStepper steppp;
void moveAuto()
{
  size_t arr_size = sizeof(point_arr) / sizeof(point_arr[0]);
  for(size_t i=0;i<arr_size;i++)
  {
    if(!moveToPosition(point_arr[i][0],point_arr[i][1]))break;
    
  }
}
bool moveToPosition(float x_target, float y_target)
{
  float dx = (x_target > x_now) ? step_size : -step_size;
  float dy = (y_target > y_now) ? step_size : -step_size;

  int steps_x = abs(x_target - x_now) / step_size;
  int steps_y = abs(y_target - y_now) / step_size;
  int steps = max(steps_x, steps_y);

  float cableLengthFirst[4] = {0}, cableLengthNow[4] = {0};
  float totalLength[4] = {0};

  is_moving = true;
  int step=0;
  for (; step < steps; step++)
  {
    if (!is_moving)
      break;

    if (abs(x_now - x_target) < step_size)
    {
      x_now = x_target;
    }
    else
    {
      x_now += dx;
    }

    if (abs(y_now - y_target) < step_size)
    {
      y_now = y_target;
    }
    else
    {
      y_now += dy;
    }
    calculateCableLengths(x_now, y_now, cableLengthNow);
    if (step == 0)
    {
      memcpy(cableLengthFirst, cableLengthNow, sizeof(cableLengthFirst));
    }
    float newLengths[4];
    getNewLength(cableLengthFirst, cableLengthNow, newLengths);
    memcpy(cableLengthFirst, cableLengthNow, sizeof(cableLengthFirst));
    for (int i = 0; i < 4; i++)
    {
      totalLength[i] += newLengths[i];
    }
    // sendDataToSerial(newLengths);
    long positions[4];
    // long pos_move_now[4];
    // for (int j = 0; j < 4; j++)
    // {
    //   // pos_move_now[j] = new_pos[j] + pos[j];
    //   new_pos[j] += pos[j];
    //   pos_move_now[j] = new_pos[j];
    //   if (j == L_D || j == R_D || j == R_U)
    //     pos_move_now[j] = -pos_move_now[j];
    // }
    for (int i = 0; i < 4; i++)
    {
      positions[i] += (newLengths[i] * length_to_step); // Chuyển đổi độ dài sang bước
      if (i == L_D || i == R_D || i == R_U)
      {
        positions[i] = -positions[i];
      }
    }
    steppp.moveTo(positions);

    // Đợi cho đến khi tất cả động cơ hoàn thành
    while (steppp.run())
    {
      // Chờ động cơ hoàn tất
      if(!digitalRead(BUT_STOP))
      {
        Serial.println("STOP");
        is_moving=false;
        break;
      }

    }
  }
  is_moving = false;
  if(step==steps)return true;
  Serial.println("STOP BY BUTTON");
  return false;
  
}
void setup()
{
  Serial.begin(57600);

  // Cấu hình chân EN và bật các driver
  pinMode(BUT_START,INPUT_PULLUP);
  pinMode(BUT_STOP,INPUT_PULLUP);
  for (int i = 0; i < 4; i++)
  {
    pinMode(en[i], OUTPUT);
    digitalWrite(en[i], LOW);         // LOW để bật driver
    steppers[i].setMaxSpeed(200);     // Tốc độ tối đa
    steppers[i].setAcceleration(500); // Gia tốc
    steppp.addStepper(steppers[i]);
  }

  // Serial.println("Ready to receive commands in the format: motor/steps");
}
void loop()
{
  // Kiểm tra nếu có dữ liệu từ Serial
  static bool start_wait = 0;
  static bool still_running = 0;
  static bool auto_now=0;
  if(!digitalRead(BUT_START))
  {
    moveAuto();

  }
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil(';'); // Đọc chuỗi lệnh đến khi gặp newline
    // 10/20*30$50;
    int slashIndex = command.indexOf('/');
    if (slashIndex > 0)
    {
       float x_get = command.substring(0, slashIndex).toFloat();
       float y_get = command.substring(slashIndex+1).toFloat();
       moveToPosition(x_get,y_get);
      // for (int i = 0; i < 4; i++) last_pos[i] = pos[i];
      // Serial.println(new_pos[0]);
      // Serial.println(new_pos[1]);
      // Serial.println(new_pos[2]);
      // Serial.println(new_pos[3]);
      // Kiểm tra số động cơ hợp lệ
    }
  }
  // if (Serial.available() > 0)
  // {
  //   String command = Serial.readStringUntil(';'); // Đọc chuỗi lệnh đến khi gặp newline
  //   // 10/20*30$50;
  //   int slashIndex = command.indexOf('/');
  //   if (slashIndex > 0)
  //   {
  //     int mor2_index = command.indexOf('*');
  //     int mor3_index = command.indexOf('$');
  //     pos[L_D] = command.substring(0, slashIndex).toInt();
  //     pos[L_U] = command.substring(slashIndex + 1, mor2_index).toInt();
  //     pos[R_U] = command.substring(mor2_index + 1, mor3_index).toInt();
  //     pos[R_D] = command.substring(mor3_index + 1).toInt();
  //     long pos_move_now[4];
  //     for (int j = 0; j < 4; j++)
  //     {
  //       // pos_move_now[j] = new_pos[j] + pos[j];
  //       new_pos[j] += pos[j];
  //       pos_move_now[j] = new_pos[j];
  //       if (j == L_D || j == R_D || j == R_U)
  //         pos_move_now[j] = -pos_move_now[j];
  //     }
  //     start_wait = 1;
  //     steppp.moveTo(pos_move_now);
  //     // for (int i = 0; i < 4; i++) last_pos[i] = pos[i];
  //     // Serial.println(new_pos[0]);
  //     // Serial.println(new_pos[1]);
  //     // Serial.println(new_pos[2]);
  //     // Serial.println(new_pos[3]);
  //     // Kiểm tra số động cơ hợp lệ
  //   }
  // }

  // Gọi run() để thực thi lệnh di chuyển cho từng động cơ
  // for (int i = 0; i < 4; i++) {
  //   steppers[i].runToNewPosition(pos[i]);
  // }
  still_running = steppp.run(); // true = dang chay false=done
  // Serial.println(still_running);
  if (start_wait && !still_running)
  {

    start_wait = 0;
    Serial.println("done");
  }
  // static unsigned long time_ = millis();
  // if (millis() - time_ > 10) {
  //   Serial.print(steppers[0].currentPosition());
  //   Serial.print(",");
  //   Serial.print(steppers[1].currentPosition());
  //   Serial.print(",");
  //   Serial.print(steppers[2].currentPosition());
  //   Serial.print(",");
  //   Serial.println(steppers[3].currentPosition());
  // }
}
