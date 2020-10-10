#include <WiFi.h> //Connect to WiFi 
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu6050_esp32.h>
#include <string.h>  //used for some string handling and processing.
// We Import Our Needed Headers
SemaphoreHandle_t semaphore;


MPU6050 imu; // We Call Our IMU object
TFT_eSPI tft = TFT_eSPI();// We Call Our TFT Object

const int LOOP_PERIOD = 40; // We Define Our Looping Period In Milliseconds
const int F_0 = 1000; // We Define The Force Applied To Balls
unsigned long looptimer; // We Define Our Loop Timer
unsigned long primary_timer; //We Define A Timer For Our Power-Ups 
int CONST = 10000; //For Cursor Movement

uint32_t last_time; //used for timing

const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int GETTING_PERIOD = 5000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response


char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
//Extracting Dimensions From Server GET Request
char response_buffer[OUT_BUFFER_SIZE];
char response_buffer1[OUT_BUFFER_SIZE];
char request_buffer1[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char REQUEST_BUFFER[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
//Extracting Dimensions From Server GET Request
char RESPONSE_BUFFER[OUT_BUFFER_SIZE]; 
char OLD_RESPONSE_BUFFER[OUT_BUFFER_SIZE];

//The Following Is For POSTing And GETing The Player's Best Scores
char score_request[IN_BUFFER_SIZE];
char score_response[OUT_BUFFER_SIZE];
int body_len;
char body[200]; 

char username[1000];

//Define default response buffer for comparison
char default_buff[1000] = "0,0,0,0\n";

char network[] = "ATTp7DDXtA";  //SSID for 6.08 Lab
char password[] = "f4urpsewse6k"; //Password for 66.08 Lab

//Stores dimensions for last block put in
int DIMS[4] = {};
int blocks[40] = {};
int num_blocks = 0;

//Used to allow IMU Ball input for powerups
float BALL_X_FORCE = 0.0;
float BALL_Y_FORCE = 0.0;

//Used for activating IMU Ball powerup
bool imu_ball = false;
int imu_ball_timer;

//For Letting User Know Last Powerup Color Displayed
char last_powerup[100];

//For Timing Purposes
int timestamp = 0;
int second = 1000;
//For Scoring Purposes
float hitcounter = 0;
float game_score = 0;
float session_best = 0;
float player_best = 0;
float updating = 0;

float factor = 0.15;
TaskHandle_t Task1;
char bricks[60];

//We Define Our Colors
#define BACKGROUND TFT_BLACK
#define BALL_COLOR TFT_WHITE

//We Define Our Two Input Pins
const uint8_t BUTTON1 = 16; 
const uint8_t BUTTON2 = 5; 

//We Define Our Brick Dimensions
const int W = 16;
const int H = 8;

//We Define Our Counter For The Ball Falling Below The Screen And Another Indicator For Exiting Purposes
int boundcount = 0;
int exited = 0;

//Constants For Cursor
int cursorH = 8;
int cursorW = 40;

// Display State Variable
int display_state;
// Powerup State Variable
int powerup_state;
// Intro Screen(s) State Variable
int intro_state;

//We Define Variables Responsible For Telling Whether We Are Resetting The Ball
int start = 0;
int reset = 0;

//We Define Our Display States
#define START 0
#define GAME 1

#define NORMAL 2
#define BALL_SPEED 3
#define BALL_SLOW 4
#define CURSOR_SPEED 5
#define BLOCK_DELETE 6
#define IMU_BALL 7
#define SCREEN_1 8
#define SCREEN_2 9

//We Define Our Termination Timer And Username
unsigned long terminationtime;
//char username[100] = "Benedictus";

//We Define Button Class
class Button{
  public:
  uint32_t t_of_state_2;
  uint32_t t_of_button_change;    
  uint32_t debounce_time;
  uint32_t long_press_time;
  uint8_t pin;
  uint8_t flag;
  bool button_pressed;
  uint8_t state; // This is public for the sake of convenience
  Button(int p) {
    flag = 0;  
    state = 0;
    pin = p;
    t_of_state_2 = millis(); //init
    t_of_button_change = millis(); //init
    debounce_time = 10;
    long_press_time = 1000;
    button_pressed = 0;
  }
  void read() {
    uint8_t button_state = digitalRead(pin);  
    button_pressed = !button_state;
  }
  int update() {
    read();
    flag = 0;
    if (state==0) {
      if (button_pressed) {
        state = 1;
        t_of_button_change = millis();
      }
    } else if (state==1) {
        if (!button_pressed){
          state = 0;
          t_of_button_change = millis();
        } else {
          if ((millis() - t_of_button_change) >= debounce_time){
            state = 2;
            t_of_state_2 = millis();
          }
        }
    } else if (state==2) {
        if (button_pressed){
          if ((millis()-t_of_state_2) >= long_press_time){
            state = 3;
          }
        } else {
          state = 4;
          t_of_button_change = millis();
        }
    } else if (state==3) {
        if (!button_pressed){
          state = 4;
          t_of_button_change = millis();
        }
    } else if (state==4) {        
        if (!button_pressed){
          if ((millis() - t_of_button_change) >= debounce_time){
            state = 0;
            if ((millis() - t_of_state_2 < long_press_time)){
              flag = 1;
            } else {
              flag = 2;
            }
          }
        } else {
            if ((millis() - t_of_state_2) < long_press_time){
              state = 2;
              t_of_button_change = millis();
            } else if ((millis() - t_of_state_2) >= long_press_time){
              state = 3;
              t_of_button_change = millis();
            }
        }
    }
    return flag;
  }
};

//We Define Our Ball Class
class Ball {
  
    float x_pos = 64; //x position
    float y_pos = 75; //y position
    float x_vel; //x velocity
    float y_vel; //y velocity
    float x_accel; //x acceleration
    float y_accel; //y acceleration
    TFT_eSPI* local_tft; //tft library
    int BALL_CLR; //ball color
    int BKGND_CLR; // background color
    float MASS; //for calculation purposes
    int RADIUS; //ball's radius
    float K_FRICTION;  //friction coefficient
    float K_SPRING;  //spring coefficient
    int LEFT_LIMIT; //screen's left border
    int RIGHT_LIMIT; //screen's right border
    int TOP_LIMIT; //screen's upper border
    int BOTTOM_LIMIT; //screen's lower border
    int DT; //time infinitesimal
    int boundcounter; //counter for the ball going out of bounds
    int resetting; //meant for the resetting of the ball
    unsigned long termination; //for termination based on idleness or trapped
    
  public:
  
    //We Define Our Class Initialization
    Ball(TFT_eSPI* tft_to_use, int dt, int rad = 4, float ms = 1,
         int ball_color = TFT_WHITE, int background_color = BACKGROUND,
         int left_lim = 0, int right_lim = 127, int top_lim = 0, int bottom_lim = 159,
         int bcount = 0, int resetted = 0) {
      x_pos = 64; //x position
      y_pos = 75; //y position
      x_vel = rand()%200 - 100; //x velocity
      y_vel = 100; //y velocity
      x_accel = 0; //x acceleration
      y_accel = 0; //y acceleration
      local_tft = tft_to_use; //our TFT library
      BALL_CLR = ball_color; //ball color
      BKGND_CLR = background_color; //background color
      MASS = ms; //for starters
      RADIUS = rad; //radius of ball
      K_FRICTION = 0;  //friction coefficient,
      K_SPRING = 1;  //spring coefficient,
      LEFT_LIMIT = left_lim + RADIUS; //ball's left border
      RIGHT_LIMIT = right_lim - RADIUS; //ball's right border
      TOP_LIMIT = top_lim + RADIUS; //ball's upper border
      BOTTOM_LIMIT = bottom_lim - RADIUS; //ball's lower border
      DT = dt; //timing for integration
      boundcounter = bcount; //counter for ball going out of bounds
      resetting = 0;
    }

    //We Define A Timestep For Our Ball
    void step(float x_force = 0, float y_force = 0 ) {

      //We Erase Our Previous Ball
      local_tft->fillCircle(x_pos, y_pos, RADIUS, BKGND_CLR);

      //We Update Our Force Based On Friction
      x_force -= K_FRICTION*x_vel;
      y_force -= K_FRICTION*y_vel;
      //Newton's Second Law
      x_accel = x_force/MASS;
      y_accel = y_force/MASS;
      //We Integrate To Get Our New Velocity
      if (!imu_ball) { //Integrate normally if not using imu_ball powerup, otherwise just use the acceleration
        x_vel = x_vel + 0.001*DT*x_accel; 
        y_vel = y_vel + 0.001*DT*y_accel; 
      } else {
        x_vel = 0.001*DT*x_accel;
        y_vel = 0.001*DT*y_accel;
      }
      //We Move Our Ball
      moveBall();

      //We Draw Our New Ball
      local_tft->fillCircle(x_pos, y_pos, RADIUS, BALL_CLR);
    }

    //We Define Our Reset Function
    void reset(int change) {
      resetting = change; //Meant For The Reset Of The Ball
    }
    
    //We Define A Function Meant For Debugging
    int getX() {
      return x_pos;
    }
    //We Define A Function Meant For Debugging
    int getY() {
      return y_pos;
    }

    //We Define A Function Meant For Retrieving Members
    float getDT() {
      return DT;
    }

    //We Define A Function Meant For Retrieving Members
    float getxvel() {
      return x_vel;
    }

    //We Define A Function Meant For Retrieving Members
    float getyvel() {
      return y_vel;
    }

    //We Define A Function Meant For Retrieving Members
    float getspring() {
      return K_SPRING;
    }

    //We Define A Function Meant To Set The X Position
    int setxpos(int xpos) {
      x_pos = xpos;
    }

    //We Define A Function Meant To Set The Y Position
    int setypos(int ypos) {
      y_pos = ypos;
    }

    //We Define A Function Meant To Set The X Velocity
    void setxvel(float xvel) {
      x_vel = xvel;
    }

    //We Define A Function Meant To Set The Y Velocity
    void setyvel(float yvel) {
      y_vel = yvel;
    }

    //We Define A Function To Get Back A Count For The Ball Going Out Of Bounds
    int getBoundCount() {
      return boundcounter;
    }

    //We Define A Function Responsible For Starting The Ball's Movement From The Cursor
    void start(float xvelocity, float yvelocity) {
      x_vel = xvelocity;
      y_vel = yvelocity;
    }

    //We Define A Setter Function For The Boundcounter For Match Resetting Purposes
    void setBoundCount(int boundcount) {
      boundcounter = boundcount;
    }

    //We Define A Getter Function For Receiving The Information For Whether The Ball Is Resetting
    int getReset() {
      return resetting;
    }

    //We Define A Getter Function For Getting Our Termination Timer
    int getTermination() {
      return termination;
    }

    //We Define A Function To Reset Our Termination Timer
    void resetTermination() {
      termination = millis();
    }
  
  private:
  
    //We Define Our Function Responsible For Changing The Position Of The Ball
    void moveBall(){
      float diff; // This Is Used For Wall Collisions

      //If Our Ball Ends Up To The Left Of The Screen
      if (0.001*DT*x_vel+x_pos < LEFT_LIMIT) {
         
        diff = LEFT_LIMIT - (0.001*DT*x_vel+x_pos);
        x_pos = LEFT_LIMIT + diff*K_SPRING;
        x_vel = -K_SPRING*x_vel;
        reset(0);
        resetTermination();
      }

      //If Our Ball Ends Up To The Right Of The Screen
      else if (0.001*DT*x_vel+x_pos > RIGHT_LIMIT) {
        
        diff = (0.001*DT*x_vel+x_pos) - RIGHT_LIMIT;
        x_pos = RIGHT_LIMIT - diff*K_SPRING;
        x_vel = -K_SPRING*x_vel;
        reset(0);
        resetTermination();
      }

      //We Integrate Normally
      else {
        x_pos += 0.001*DT*x_vel;
        reset(0);
      }

      //If Our Ball Ends Up Above The Screen
      if (0.001*DT*y_vel+y_pos < TOP_LIMIT) {
         
        diff = TOP_LIMIT - (0.001*DT*y_vel+y_pos);
        y_pos = TOP_LIMIT + diff*K_SPRING;
        y_vel = -K_SPRING*y_vel;
        reset(0);
        resetTermination();
      }

      //If Our Ball Ends Up Below The Screen
      else if (0.001*DT*y_vel+y_pos - RADIUS > BOTTOM_LIMIT) {
        //We Are Out Of Bounds Now
        //We Erase Our Ball
        local_tft->fillCircle(x_pos, y_pos, RADIUS, BKGND_CLR);
        boundcounter += 1;
        reset(1);
        resetTermination();
      }

      //We Integrate Normally
      else {
        y_pos += 0.001*DT*y_vel;
        reset(0);
      }
    }
    
};

//We Define Our Brick Class
class Brick {

  int xposition; //x position
  int yposition; //y position
  int Width; //rectangle width
  int Height; //rectangle height
  TFT_eSPI* local_tft; //tft library
  int color; //ball color
  int background; // background color
  int left; //left border
  int right; //right border
  int up; //upper border
  int down; //lower border

  public:

    //We Define Our Class Initialization
    Brick(TFT_eSPI* tft_to_use, int x_pos, int y_pos,
         int brick_color = TFT_WHITE, int background_color = BACKGROUND,
         int brick_width = W, int brick_height = H) {
      xposition = x_pos; //x position
      yposition = y_pos; //y position
      local_tft = tft_to_use; //our TFT library
      color = brick_color; //ball color
      background = BACKGROUND; //background color
      Width = brick_width; //placement purposes and collision
      Height = brick_height; //placement purposes and collision 
      left = xposition; //ball's left border
      right = xposition + Width; //ball's right border
      up = yposition; //ball's upper border
      down = yposition + Height; //ball's lower border
    }

    //We Define How We Generate A Brick
    void generate(){

      local_tft->fillRect(xposition, yposition, Width, Height, color);
    
    }

    //We Define How We Remove A Brick
    void remove() {

      local_tft->fillRect(xposition, yposition, Width, Height, BACKGROUND);

    }

    //We Define How To Return Our Width
    int getW() {

      return Width;
    }

    //We Define How To Return Our Height
    int getH() {

      return Height;
    }

    //We Define How To Set Our Brick's xposition
    void setX(int x) {

      xposition = x;
    }

    //We Define How To Set Our Brick's yposition
    void setY(int y) {

      yposition = y;
    }
};

//We Define Our Cursor Class
class Cursor {
    int HEIGHT = cursorH;
    int WIDTH = cursorW;
    int LEFT_LIMIT = 0;
    int RIGHT_LIMIT = 127 - WIDTH;
    float TOP_LEFT_X;
    float TOP_LEFT_Y;
    float CURSOR_VEL;
    int LOOP_PERIOD;
    int CURSOR_COLOR;;
    int CONSTANT;
    
    TFT_eSPI* cursor_tft; //object point to TFT (screen)
    
  public:
    Cursor(TFT_eSPI* tft_to_use, int loop_period) {
      cursor_tft = tft_to_use;
      TOP_LEFT_X = 40;    //initial placement (subject to change)
      TOP_LEFT_Y = 130;
      CURSOR_VEL = 0;
      LOOP_PERIOD = loop_period;
      CURSOR_COLOR = TFT_WHITE;
      CONSTANT = 1;
    }
    void step(float acc) {
      CURSOR_VEL = 0.001 * LOOP_PERIOD * acc; // Taking acceleration and integrating to velocity
      cursor_tft->fillRect(TOP_LEFT_X, TOP_LEFT_Y, WIDTH, HEIGHT, BACKGROUND); // Clearing what was there before
      moveCursor();
      cursor_tft->fillRect(TOP_LEFT_X, TOP_LEFT_Y, WIDTH, HEIGHT, CURSOR_COLOR); // Drawing updated cursor after movement
    }
   void moveCursor() {
      TOP_LEFT_X += 0.001 * LOOP_PERIOD * CURSOR_VEL; // Integrating velocity into position and adding onto the top left x value of cursor rectangle
      
      if (TOP_LEFT_X < LEFT_LIMIT) { // Hits left wall
         TOP_LEFT_X = LEFT_LIMIT;
      }
      if (TOP_LEFT_X > RIGHT_LIMIT) { // Hits right wall
         TOP_LEFT_X = RIGHT_LIMIT;
      }
   }
   
   int getY() { //Returns top left Y coordinate
      return TOP_LEFT_Y;
   }
   int getX() { //Returns top left X coordinate
      return TOP_LEFT_X;
   }
   //We Get The Velocity Of The Cursor
   float getvel() {
      return CURSOR_VEL;
   }
   //We Set The Cursor's Velocity
   void setvel(float new_vel) {
      CURSOR_VEL = new_vel;
   }
   // Set new color for cursor
   void setColor(int color){
      CURSOR_COLOR = color;
   }
   // We Set The Constant Factor In The Cursor's Acceleration
   void setConst(int num) {
      CONSTANT = num;
   }
};


//Class For User 2 Blocks
class Block {

  //We Define The Dimensions Of The Blocks
  int blockW;
  int blockH;
  int blockX;
  int blockY;

  TFT_eSPI* block_tft; //object point to TFT (screen)

  public:
    //We Define The Relevant Attributes
    Block(TFT_eSPI* tft_to_use, int x = 0, int y = 0, int width = 0, int height = 0) {
      block_tft = tft_to_use;
      blockX = x;
      blockY = y;
      blockW = width;
      blockH = height;
    }

    //We Define Our We Draw The Adversarial Block
    void step(){
      if (strcmp(RESPONSE_BUFFER,OLD_RESPONSE_BUFFER) != 0 && strcmp(RESPONSE_BUFFER,default_buff)!= 0) {
        strcpy(OLD_RESPONSE_BUFFER, RESPONSE_BUFFER);
        char RESPONSE_BUFFER_COPY[OUT_BUFFER_SIZE];
        strcpy(RESPONSE_BUFFER_COPY,RESPONSE_BUFFER);
        extract_block(RESPONSE_BUFFER_COPY);
        setX(DIMS[0]);
        setY(DIMS[1]);
        setW(DIMS[2]);
        setH(DIMS[3]);
        blocks[num_blocks * 4] = DIMS[0];
        blocks[num_blocks * 4 + 1] = DIMS[1];
        blocks[num_blocks * 4 + 2] = DIMS[2];
        blocks[num_blocks * 4 + 3] = DIMS[3];
        num_blocks += 1;
      }
      draw();
    }

    //We Draw The Adversarial Block
    void draw(){
      block_tft->fillRect(blockX,blockY,blockW,blockH,TFT_WHITE);
    }

    //We Draw The Adversarial Block
    void remove(){
      block_tft->fillRect(blockX,blockY,blockW,blockH,BACKGROUND);
      blocks[(num_blocks - 1) * 4] = 0;
      blocks[(num_blocks - 1) * 4 + 1] = 0;
      blocks[(num_blocks - 1) * 4 + 2] = 0;
      blocks[(num_blocks - 1) * 4 + 3] = 0;
      blockX = 0;
      blockY = 0;
      blockW = 0;
      blockH = 0;
      num_blocks -= 1;
    }

    //We Define How We Set The Dimensions Of The Block
    void setX(int x) {
      blockX = x;
    }
    void setY(int y) {
      blockY = y;
    }
    void setW(int w) {
      blockW = w;
    }
    void setH(int h) {
      blockH = h;
    }
    int getX() {
      return blockX;
    }
    int getY() {
      return blockY;
    }
    int getW() {
      return blockW;
    }
    int getH() {
      return blockH;
    }
    //We Define How We Retrieve The Relevant Dimensions Of The Block From A HTTP Request Buffer
    void extract_block(char* BUFFER) { //Assuming BUFFER is structured as follows: "x,y,width,height"
      char* PARAMS;
      PARAMS = strtok(BUFFER, ",");
      DIMS[0] = atoi(PARAMS);
      PARAMS = strtok(NULL,",");
      DIMS[1] = atoi(PARAMS);
      PARAMS = strtok(NULL,",");
      DIMS[2] = atoi(PARAMS);
      PARAMS = strtok(NULL,",");
      DIMS[3] = atoi(PARAMS);
  }
};

class UsernameGetter {
    char alphabet[50] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char message[400] = {0}; //contains previous query response
    char query_string[50] = {0};
    int char_index;
    int state;
    unsigned long scrolling_timer;
    const int scrolling_threshold = 150;
    const float angle_threshold = 0.3;
  public:

    UsernameGetter() {
      state = 0;
      memset(message, 0, sizeof(message));
      strcat(message, " ");
      char_index = 0;
      scrolling_timer = millis();
    }
    void update(float angle, int button, char* output) {
      if (state == 0){
        strcpy(output,message);
        if (button == 2){ // flag for long press
          state = 1;
          char_index = 0;
          memset(query_string, 0, sizeof(query_string));
          scrolling_timer = millis();
        }
      } else if (state == 1){ // text entry
         char letter[2];
         memset(output, 0, sizeof(output));
         
         if (button == 1){
            letter[0] = alphabet[char_index];
            strcat(query_string, letter);
            char_index = 0;
            
            strcat(output, query_string);
            
         } else if (button == 2){
            letter[0] = alphabet[char_index];
            strcat(query_string, letter);
            char_index = 0;
            
            strcat(output, query_string);
            
            state = 2;
         } else {
            if ((millis() - scrolling_timer) >= scrolling_threshold) {
                if (abs(angle) > angle_threshold) {
                  if (angle < 0) {
                    char_index -= 1;
                    if (char_index < 0) {
                      char_index = 37 + char_index;
                    }
                  } else {
                    char_index += 1;
                  } 
                }
                scrolling_timer = millis();
            }
           
            letter[0] = alphabet[char_index];
          
            strcat(output, query_string); 
            strcat(output, letter);
         }
       } else if (state == 2) {
            char password[50] = "pass1111"; 
            char body1[200];
            Serial.print("CHECKING THE POST SEND      ");
            sprintf(body1, "pass=%s&user=%s",password,query_string);
            Serial.println(body1);
            int body_len1 = strlen(body1); 
        sprintf(request_buffer1, "POST http://608dev-2.net/sandbox/sc/team110/isaakWeek2/passchecker.py HTTP/1.1\r\n");
        strcat(request_buffer1, "Host: 608dev-2.net\r\n");
        strcat(request_buffer1, "Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request_buffer1 + strlen(request_buffer1), "Content-Length: %d\r\n", body_len1); 
        strcat(request_buffer1, "\r\n"); 
        strcat(request_buffer1, body1); 
        strcat(request_buffer1, "\r\n"); 
        Serial.println(request_buffer1);
        do_http_request("608dev-2.net", request_buffer1, response_buffer1, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
        Serial.println(response_buffer1);
  
        }
    }

    void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}



    int getState(){
      return state;
    }

    void reset(){
      memset(message, 0, sizeof(message));
      memset(username, 0, sizeof(username));
      strcpy(query_string, "");
      scrolling_timer = millis();
      char_index = 0;
      state = 0;
    }

};


//We Define A Game Session
class Game {
  
    int score; // We Define Our Score
    int left_limit = 0; // Screen's Left Border
    int right_limit = 127; // Screen's Right Border
    int top_limit = 0; // Screen's Upper Border
    int bottom_limit = 159; // Screen's Bottom Border
    int radius = 4; // Our Ball's Radius
    TFT_eSPI* game_tft; // Our TFT Object
    int prevx; //For Drawing Purposes
    int prevy; //For Drawing Purposes
    //char bricks[60]; //So That The System "Remembers" Which Brick Is Which
    int done = 0; //To Indicate Whether A Game Has Ended
    int hits = 0; //For Scoring Purposes
    int lives = 3; //For Match Resetting Purposes
    //We Look At Our Count On The Number Of Times The Ball Has Gone Out Of Bounds
    int boundcounter = player_ball.getBoundCount();
    //We Define Relevant Values For When The Ball Is Stuck To The Cursor
    int previousx;
    int previousy;
    //We Define A Variable Responsible For Updating Our Score Based On Power-Up Decision
    float update = 0;
    int difficulty;
    
  public:
  
    Ball player_ball; // We Define Player's Ball
    Brick game_brick; //We Define A Prospective Brick
    Cursor game_cursor; //We Define Our Cursor
    Block game_block; //We Define User2 Block
    //We Define Our Ball Instance Based On Our Game Definition
    Game(TFT_eSPI* tft_to_use, int loop_speed, int ended = 0, int diff = 1):
      player_ball(tft_to_use, loop_speed, radius, 1, TFT_WHITE, BACKGROUND,
                  left_limit, right_limit, top_limit, bottom_limit, ended),
      game_brick(tft_to_use, 0, 0, TFT_WHITE, BACKGROUND, W, H),
      game_cursor(tft_to_use,loop_speed),
      game_block(tft_to_use)
    {
      game_tft = tft_to_use;
      score = 0;
      done = ended;
      difficulty = diff;
    }

    //We Define How We Set Up A Game
    void setup() {
      //We Define Our Bricks
      //for (int j = 10; j < 106; j += 10) {
      for (int j = 10; j < 21; j += 10) {
          //for (int i = 11; i < 109; i += 18) {
          for (int i = 11; i < 121; i += 18) {

              //We Create Our Ball Instance
              Brick game_brick(&tft, i, j, TFT_WHITE, BACKGROUND, W, H);
              //This Is A Temporary Solution To A Conflict Of The System "Remembering" Versus "Seeing" The Same Bricks
              if (i < 109) {
                //We Display A Brick
                game_brick.generate();
              }

              //We Have The System "Remember" The Particular Brick
              char* one = "1";
              strncat(bricks, one, 1);
              
          }
        }

      //We Set Our Ball
      //We Initialize Our Termination Timer
      player_ball.resetTermination();
      //We Stick The Ball To The Cursor
      int nextx = game_cursor.getX() + (cursorW)/2;
      int nexty = game_cursor.getY() - (radius);
      float newvel = game_cursor.getvel();
      player_ball.setxpos(nextx);
      player_ball.setypos(nexty);
      player_ball.setxvel(newvel);
      player_ball.setyvel(0);

      if (difficulty == 2){
        game_block.setX(50);
        game_block.setY(50);
        game_block.setW(25);
        game_block.setH(25);
        float yvel = player_ball.getyvel();
        player_ball.setyvel(yvel * 2);
        float xvel = player_ball.getxvel();
        player_ball.setxvel(xvel * 2);
        
      } else if (difficulty == 3) {
          game_block.setX(75);
          game_block.setY(75);
          game_block.setW(40);
          game_block.setH(40);
          float yvel = player_ball.getyvel();
          player_ball.setyvel(yvel * 3);
          float xvel = player_ball.getxvel();
          player_ball.setxvel(xvel * 3);
      }
    }

    //We Define Our Game's Time Step
    void step(float x, float y, float x_cursor,  int reset, int start) {

      //We Look At Our Count On The Number Of Times The Ball Has Gone Out Of Bounds
      boundcounter = player_ball.getBoundCount();

      //If The Ball Has Been Resetted
      if (reset == 1 && start == 0) {
        int nextx = 64;
        int nexty = 130 - (radius);
        float newvel = 0;
        game_tft->fillCircle(previousx, previousy, radius, BACKGROUND);
        previousx = nextx;
        previousy = nexty;
        //We Stick The Ball To The Cursor
        player_ball.setxpos(nextx);
        player_ball.setypos(nexty);
        player_ball.setxvel(newvel);
        player_ball.setyvel(0);

      }
      //If The Ball Has Been Shot By The Player
      else if (reset == 0 && start == 1) {
          
         float newxvel = factor*game_cursor.getvel(); //x velocity
         float newyvel = -60; //y velocity
         player_ball.start(newxvel, newyvel);

      }
      player_ball.step(x, y); //Our Ball Will Take A Step
      game_cursor.step(x_cursor); //Our Cursor Takes A Step
      game_block.step(); //Our Block Takes A Step

      //We Redraw The Bricks That Haven't Been Hit For The Sake Of Display
      int counting = 0;
      //for (int j = 10; j < 106; j += 10) {
      for (int j = 10; j < 21; j += 10) {
          for (int i = 11; i < 121; i += 18) {

              //See The Explanation Inside The Set Up Method For The Reason Behind This Condition
              if (i < 109) {
                //We Display A Brick That Has Not Been Broken
                if (strncmp(bricks+counting, "1", 1) == 0) {
                  game_brick.setX(i);
                  game_brick.setY(j);
                  game_brick.generate();
                  }
              }

              counting += 1;
          }

        }

      int new_score = collisionDetect(); //We Check If A Collision Has Occured In Order To Update Our Score
      //If Our Score Changed We Make Some Changes
      if (new_score != score) {
        score = new_score;
        game_brick.remove();

        game_tft->fillCircle(prevx, prevy, radius, BACKGROUND);
        game_tft->fillCircle(player_ball.getX(), player_ball.getY(), radius, TFT_WHITE);

      }

      //If The Player Has Lost All Their "Lives"
      if (boundcounter >= lives){
        done = 1;
      }

      //We Look At Our Bricks And Check If All Have Been Removed To End The Game
      int number = 0;

      for (int i = 0; i < 12; i += 1) {
        if (strncmp(bricks+i, "1", 1) == 0) {
          number += 1;
        }
      }
      if (number == 1) {
        done = 1;
      }

      //We Look At Our Timer For Particular Termination Conditions
      terminationtime = player_ball.getTermination();
      //Serial.print("Termination time is ");
     // Serial.println(terminationtime);
      if (millis() - terminationtime >= 6000) {
        done = 1;
      }

      //We Draw A Rectangle In The Background Color To Remove The Display Of Balls That Fell Out Of Bounds
      game_tft->fillRect(0, 150, 128, 10, BACKGROUND);
      //We Define Our We Write Out Our Score
      game_tft->setCursor(0, 150, 1);
      game_tft->setCursor(0, 150, 1);
      tft.setTextColor(TFT_WHITE, BACKGROUND);
      game_tft->print("Score: ");
      game_tft->println(game_score);
    }

    //We Define How We Know If A Game Match Has Ended
    int getstatus() {
      return done;
    }

    //We Define A Function That Helps Us Get The Score
    int getscore() {
      return score;
    }
    int getBallCordX(){
      return player_ball.getX();
    }
    int getBallCordY(){
      return player_ball.getY();
    }
    int getCursCordX(){
      return game_cursor.getX();
    }
    int getCursCordY(){
      return game_cursor.getY();
    }

    //We Define A Function That Returns How Many Times We Hit The Ball
    int gethits() {
      return hits;
    }

    //We Define A Function That Sets The Number Of Hits For The Purpose Of Resetting Matches
    void resethits() {
      hits = 0;
    }

    //We Define A Function That Sets The Status Of A Match For Restting Purposes
    void setstatus(int status) {
      done = status;
    }

    //We Define A Function That Sets The Number Of Times The Ball Can Go Out Of Bounds ("Lives" So To Say)
    void setlives(int living) {
      lives = living;
    }

    //We Define A Setter Function For The Ball's Bound Counter For Match Resetting Purposes
    void setboundcounter(int bounding) {
      player_ball.setBoundCount(bounding);
    }

    //We Define A Getter Function For The Ball's Bound Counter For Ball Resetting Purposes
    int getboundcounter() {
      return boundcounter;
    }

    //We Define A Getter Function For Getting The Status Of Resetting
    int getResetting() {
      int output = player_ball.getReset();
      return output;
    }

    //We Define Functions That Updates Our Score According To Power-Ups
    void setupdate(float updated) {
      update = updated;
    }

    int getupdate() {
      return update;
    }

  private:
  
    //We Define How To Deal With Collisions
    int collisionDetect() {

      int count = 0;

      //We Define The Brick And Its Relevant Attributes
      int xpos = player_ball.getX();
      int ypos = player_ball.getY();
      //We Define The Relevant Ball's Attributes
      float dt = player_ball.getDT();
      float xvel = player_ball.getxvel();
      float yvel = player_ball.getyvel();
      float kspring = player_ball.getspring();
      float diff;
      //We Define Our Relevant Attributes For The Cursor
      int cursorx = game_cursor.getX();
      int cursory = game_cursor.getY();
      float cursorvel = game_cursor.getvel();
      //We Define Our Relevant Attributes For User2 Block
      int blockLeft = game_block.getX();
      int blockRight = game_block.getX() + game_block.getW();
      int blockUp = game_block.getY();
      int blockDown = game_block.getY() + game_block.getH();
      float blockYcenter = (blockUp + blockDown)/2;
      float blockXcenter = (blockLeft + blockRight)/2;
      //For Updating
      float newx;
      float newy;
      float newxvel;
      float newyvel;

      //If The Ball Is Within The Span Of The Cursor
      if (cursorx <= xpos + 0.001*dt*xvel + radius && xpos + 0.001*dt*xvel - radius <= cursorx + cursorW) {

        //If The Ball Is Going To Hit The Upper Edge Of The Cursor
        if (cursorH + cursory > ypos + radius + 0.001*dt*yvel && ypos + radius + 0.001*dt*yvel > cursory) {
          tft.fillCircle(xpos, ypos, radius, TFT_WHITE);
          diff = (0.001*dt*yvel+ypos) - cursory;
          newy = (cursory - diff*kspring) - radius;
          newyvel = -kspring*yvel;
          newxvel = xvel + factor*cursorvel;
          
          //player_ball.setypos(newy);
          player_ball.setyvel(newyvel);
          player_ball.setxvel(newxvel);

          //We Increase Our Hits Counter
          hits += 1;

          //We Reset Our Termination Timer
          player_ball.resetTermination();

          return score;
        }

        //If The Ball Is Going To Hit The Lower Side Of The Cursor
        else if (ypos + radius + 0.001*dt*yvel >= cursory + cursorH) {
          tft.fillCircle(xpos, ypos, radius, TFT_WHITE);
          diff = (cursory + cursorH) - (0.001*dt*yvel+ypos);
          newy = (cursory + cursorH) + diff*kspring + radius;
          newyvel = -kspring*yvel;
          newxvel = xvel + cursorvel*factor;
          
          //player_ball.setypos(newy);
          player_ball.setyvel(newyvel);
          player_ball.setxvel(newxvel);

          //We Reset Our Termination Timer
          player_ball.resetTermination();

          return score;
        }
        
      }

      //If The Ball Can Hit By The Sides Of The Cursor
      else if (cursory < ypos + 0.001*dt*yvel + radius && ypos + 0.001*dt*yvel - radius < cursory + cursorH) {

        //If The Ball Hits The Right Side Of The Cursor
        if (cursorx + (1/2)*cursorW <= xpos && xpos <= cursorx + cursorW) {
          diff = (cursorx + cursorW) - (0.001*dt*xvel+xpos);
          newx = (cursorx + cursorW) + diff*kspring;
          newxvel = -kspring*xvel;

          //player_ball.setxpos(ballx);
          player_ball.setxvel(newxvel);

          //We Reset Our Termination Timer
          player_ball.resetTermination();

          return score;
        }

        //If The Ball Hits The Left Side Of The Cursor
        else if (cursorx <= xpos && xpos <= cursorx + (1/2)*cursorW){
          diff = (0.001*dt*xvel+xpos) - cursorx;
          newx = cursorx - diff*kspring;
          newxvel = -kspring*xvel;

          //player_ball.setxpos(ballx);
          player_ball.setxvel(newxvel);

          //We Reset Our Termination Timer
          player_ball.resetTermination();

          return score;
        }
      }

      for (int i = 0; i < 10; i++) { 
        
          int blockLeft = blocks[i*4];
          int blockRight = blocks[i*4] + blocks[i*4+2];
          int blockUp = blocks[i*4+1];
          int blockDown = blocks[i*4+1] + blocks[i*4+3];
          float blockYcenter = (blockUp + blockDown)/2;
          float blockXcenter = (blockLeft + blockRight)/2;
          
          //We See If We Have A Possible Collisions On The Upper Or Lower Sides Of The Block
          if (blockLeft <= xpos + radius && xpos - radius <= blockRight) {
    
            //Above The Block
            if (blockUp <= ypos + radius && ypos - radius <= blockYcenter) {
                prevx = xpos;
                prevy = ypos;
                
                diff = (0.001*dt*yvel+ypos) - blockUp;
                ypos = blockUp - diff*kspring;
                yvel = -kspring*yvel;
    
                player_ball.setyvel(yvel);
    
                return score;
                
            }
    
            //Below The Block
            else if (blockYcenter <= ypos + radius && ypos - radius <= blockDown) {
                prevx = xpos;
                prevy = ypos;
                
    
                diff = blockDown - (0.001*dt*yvel+ypos);
                ypos = blockDown + diff*kspring;
                yvel = -kspring*yvel;
    
                player_ball.setyvel(yvel); 
    
                return score;
              
            }
            
          }
    
          //We See If We Have A Possible Collisions On The Left Or Right Sides Of The Block
          else if (blockUp <= ypos + radius && ypos - radius <= blockDown) {
            //To The Left Of The Block
            if (blockLeft <= 0.001*dt*xvel + xpos + radius && 0.001*dt*xvel + xpos + radius <= blockXcenter) {
                prevx = xpos;
                prevy = ypos;
                
              
                diff = (0.001*dt*xvel+xpos) - blockLeft;
                xpos = blockLeft - diff*kspring;
                xvel = -kspring*xvel;
    
                player_ball.setxvel(xvel);
                  
                return score;
                
            }
    
            //To The Right Of The Block
            else if (blockXcenter <= 0.001*dt*xvel + xpos - radius && 0.001*dt*xvel + xpos - radius <= blockRight) {
                prevx = xpos;
                prevy = ypos;
                
                diff = blockRight - (0.001*dt*xvel+xpos);
                xpos = blockRight + diff*kspring;
                xvel = -kspring*xvel;
    
                player_ball.setxvel(xvel);
    
                return score;
              
            }
            
          }
      }
      
      //We Look At All Bricks
      //for (int j = 10; j < 106; j += 10) {
      for (int j = 10; j < 21; j += 10) {
          for (int i = 11; i < 109; i += 18) {

              //We Define The Brick's Boundaries
              int left = i;
              int right = i + W;
              int up = j;
              int down = j + H;
              float ycenter = (up + down)/2;
              float xcenter = (left + right)/2;
              //We Look At The Next Position
              int prospecty = ypos + 0.001*dt*yvel;
              int prospectx = xpos + 0.001*dt*xvel;
              //We Check If The Position We Are Considering Has A Brick
              char brick = bricks[count];

              if (brick == '1') {

                
                //We See If We Have A Possible Collisions On The Upper Or Lower Sides Of The Brick
                if (left <= xpos + radius && xpos - radius <= right) {
  
                  //Above The Brick
                  if (up <= ypos + radius && ypos - radius <= ycenter) {
  
                      prevx = xpos;
                      prevy = ypos;
                      
                      diff = (0.001*dt*yvel+ypos) - up;
                      ypos = up - diff*kspring;
                      yvel = -kspring*yvel;
  
                      player_ball.setypos(ypos);
                      player_ball.setyvel(yvel);
                      game_brick.setX(i);
                      game_brick.setY(j);

                      char* zero = "0";
                      strncpy((bricks+count), zero, 1);

                      //We Reset Our Termination Timer
                      player_ball.resetTermination();

                      return (score+1);
                      
                  }
  
                  //Below The Brick
                  else if (ycenter <= ypos + radius && ypos - radius <= down) {
  
                      prevx = xpos;
                      prevy = ypos;
                      
  
                      diff = down - (0.001*dt*yvel+ypos);
                      ypos = down + diff*kspring;
                      yvel = -kspring*yvel;
  
                      player_ball.setypos(ypos);
                      player_ball.setyvel(yvel);
                      game_brick.setX(i);
                      game_brick.setY(j);

                      char* zero = "0";
                      strncpy((bricks+count), zero, 1);

                      //We Reset Our Termination Timer
                      player_ball.resetTermination();

                      return (score+1);
                    
                  }
                  
                }
  
                //We See If We Have A Possible Collisions On The Left Or Right Sides Of The Brick
                else if (up <= ypos + radius && ypos - radius <= down) {
  
                  //To The Left Of The Brick
                  if (left <= xpos + radius && xpos - radius <= xcenter) {
  
                      prevx = xpos;
                      prevy = ypos;
                      
  
                      diff = (0.001*dt*xvel+xpos) - left;
                      xpos = left - diff*kspring;
                      xvel = -kspring*xvel;
  
                      player_ball.setxpos(xpos);
                      player_ball.setxvel(xvel);
                      game_brick.setX(i);
                      game_brick.setY(j);

                      char* zero = "0";
                      strncpy((bricks+count), zero, 1);

                      //We Reset Our Termination Timer
                      player_ball.resetTermination();

                      return (score+1);
                      
                  }
  
                  //To The Right Of The Brick
                  else if (xcenter <= xpos + radius && xpos - radius <= right) {
  
                      prevx = xpos;
                      prevy = ypos;
                      
                      diff = right - (0.001*dt*xvel+xpos);
                      xpos = right + diff*kspring;
                      xvel = -kspring*xvel;
  
                      player_ball.setxpos(xpos);
                      player_ball.setxvel(xvel);
                      game_brick.setX(i);
                      game_brick.setY(j);
                      
                      char* zero = "0";
                      strncpy((bricks+count), zero, 1);

                      //We Reset Our Termination Timer
                      player_ball.resetTermination();

                      return (score+1);
                    
                  }
                  
                }

              }

              count += 1;
              
          }

          count += 1;
      }
      
    }

};

//We Define Our Game Object Instance (Game Session)
Game game(&tft, LOOP_PERIOD, boundcount);

//We Define Our Buttons
Button left_button(BUTTON1);
Button right_button(BUTTON2);

UsernameGetter getuser;

void setup() {
  Serial.begin(115200); //Debugging Purposes
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */    
  //We Set-Up Our IMU
  semaphore = xSemaphoreCreateBinary();
  //We Set-Up Our IMU
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  
  //We Initialize Our TFT Settings
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); //set color of font to green foreground, black background
  delay(100); //wait a bit (100 ms)
  WiFi.begin(network,password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count<6) { //can change this to more attempts
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);  //acceptable since it is in the setup function.
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n",WiFi.localIP()[3],WiFi.localIP()[2],
                                            WiFi.localIP()[1],WiFi.localIP()[0], 
                                          WiFi.macAddress().c_str() ,WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  
  //We Initialize Our Buttons
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  //We Randomize Our Session
  randomSeed(analogRead(0)); 
  //We Update Our Timers
  looptimer = millis();
  timestamp = millis(); 

  //Initialize display state
  display_state = START;
  intro_state = SCREEN_1;
  powerup_state = NORMAL;
 
}
void Task1code( void * pvParameters )
{
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

    
  for(;;){
    char body[200];
        sprintf(body, "brickin=%s&xval1=%d&yval1=%d&xval2=%d&yval2=%d",bricks,game.getBallCordX(),game.getBallCordY(),game.getCursCordX(),game.getCursCordY());
        //sprintf(body, "rorc=1&x_pos=%d&y_pos=%d&width=%d&height=%d&color=%d",xposition,yposition,Width,Height,color); 
        int body_len = strlen(body); 
        sprintf(request_buffer, "POST http://608dev-2.net/sandbox/sc/team110/sampleLCD1.py HTTP/1.1\r\n");
        strcat(request_buffer, "Host: 608dev-2.net\r\n");
        strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); 
        strcat(request_buffer, "\r\n"); 
        strcat(request_buffer, body); 
        strcat(request_buffer, "\r\n"); 
        //Serial.println(request_buffer);
        do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
        //Serial.println(response_buffer);
    xSemaphoreTake(semaphore, portMAX_DELAY);
  } 
}
void loop() {
  game_fsm();
}


//We Define Our Menu Screen
void Intro(float imu_y) {
  int state = getuser.getState();
 
  switch(intro_state) {

    case SCREEN_1: // Simple Rules
      tft.setCursor(25,5,2);
      tft.println("Brick Breaker");
      tft.setCursor(42.5,25,1);
      tft.println("WELCOME!");
      tft.setCursor(0,40,1);
      tft.println("Control the cursor by");
      tft.setCursor(0,50,1);
      tft.println("tilting the board.");
      tft.setCursor(0,62.5,1);
      tft.println("Score by breaking the");
      tft.setCursor(0,72.5,1);
      tft.println("bricks.");
      tft.setCursor(0,85,1);
      tft.println("3 lives.");
      tft.setCursor(0,97.5,1);
      tft.println("Tilt to see POWERUPS");
      tft.setCursor(0,110,1);
      tft.println("LEFT BUTTON to START");
      tft.setCursor(0,125,1);
      tft.println("RIGHT BUTTON for NAME");
      tft.setCursor(0,140,1);
      tft.print("USERNAME: ");
      tft.println(username);
      

      if (state != 1) 
      {
        if (imu_y >= 0.5) {
          tft.fillScreen(BACKGROUND);
          intro_state = SCREEN_2;
        }
      }
      break;
      
    case SCREEN_2: // POWERUPS
        tft.setCursor(30,5,2);
        tft.println("POWERUPS");
        tft.setCursor(0,30,1);
        tft.setTextColor(TFT_BLUE, BACKGROUND);
        tft.println("BLUE - SPEED UP BALL BY 2X");
        tft.setTextColor(TFT_WHITE, BACKGROUND);
        tft.setCursor(0,55,1);
        tft.setTextColor(TFT_RED, BACKGROUND);
        tft.println("RED - SLOW DOWN BALL BY 2X");
        tft.setTextColor(TFT_WHITE, BACKGROUND);
        tft.setCursor(0,80,1);
        tft.setTextColor(TFT_GREEN, BACKGROUND);
        tft.println("GREEN - SPEED UP CURSOR BY 2X");
        tft.setCursor(0,105,1);
        tft.setTextColor(TFT_MAGENTA, BACKGROUND);
        tft.println("MAGENTA - DELETE MOSTRECENT BLOCK");
        tft.setCursor(0,130,1);
        tft.setTextColor(TFT_YELLOW, BACKGROUND);
        tft.println("YELLOW - CONTROL BALLWITH IMU");
        tft.setTextColor(TFT_WHITE, BACKGROUND);

        if (state != 1) 
        {
          if (imu_y <= -0.5) 
          {
             tft.fillScreen(BACKGROUND);
             intro_state = SCREEN_1;
          }
        }
        break;
  }
}



float POWERUP_COLORS(uint8_t left_button, uint8_t right_button, float game_score) { //Currently works by just cycling between each, giving 5 seconds to determine if you'd like to use powerup or no

 switch(powerup_state) {

    case NORMAL:
      tft.setTextColor(BACKGROUND);
      tft.setCursor(80,150,1);
      tft.println(last_powerup);
      game.game_cursor.setColor(TFT_WHITE);
      if (millis() - primary_timer >= 2000 || right_button > 1) 
      {
        powerup_state = BALL_SPEED;
        primary_timer = millis();
        game.setupdate(0);
      }
      break;
      
    case BALL_SPEED:
        if (game_score >= 2){
          tft.setTextColor(BACKGROUND);
          tft.setCursor(80,150,1);
          tft.println(last_powerup);
          game.game_cursor.setColor(TFT_BLUE);
          tft.setTextColor(TFT_BLUE);
          tft.setCursor(80,150,1);
          tft.println("SPEEDUP");
          strcpy(last_powerup, "SPEEDUP");
          if (left_button > 0) {
            float curr_vel = game.player_ball.getyvel();
            float doubled = curr_vel * 2.0;
            game.player_ball.setyvel(doubled);
            powerup_state = NORMAL;
            primary_timer = millis();
            game.setupdate(-1);
          } else if (millis() - primary_timer >= 2000 || right_button > 1) {
              powerup_state = BALL_SLOW;
              primary_timer = millis();
              game.setupdate(0);
          }
         } else {
              powerup_state = BALL_SLOW;
              primary_timer = millis();
              game.setupdate(0);
         }
        break;
        
    case BALL_SLOW:
      if (game_score >= 3) {
        tft.setTextColor(BACKGROUND);
        tft.setCursor(80,150,1);
        tft.println(last_powerup);
        game.game_cursor.setColor(TFT_RED);
        tft.setTextColor(TFT_RED);
        tft.setCursor(80,150,1);
        tft.println("SLOWDOWN");
        strcpy(last_powerup, "SLOWDOWN");
        if (left_button > 0) {
          float curr_vel = game.player_ball.getyvel();
          float halfed = curr_vel * 0.5;
          game.player_ball.setyvel(halfed);
          powerup_state = NORMAL;
          primary_timer = millis();
          game.setupdate(-2);
        }else if (millis() - primary_timer >= 2000 || right_button > 1) {
            powerup_state = CURSOR_SPEED;
            primary_timer = millis();
            game.setupdate(0);
        }
      }else {
           powerup_state = CURSOR_SPEED;
           primary_timer = millis();
           game.setupdate(0);
      }
      break;
        
    case CURSOR_SPEED:
      if (game_score >= 4) {
        tft.setTextColor(BACKGROUND);
        tft.setCursor(80,150,1);
        tft.println(last_powerup);
        game.game_cursor.setColor(TFT_GREEN);
        tft.setTextColor(TFT_GREEN);
        tft.setCursor(80,150,1);
        tft.println("CURSOR");
        strcpy(last_powerup, "CURSOR");
        if (left_button > 0) {
          game.game_cursor.setConst(2); //Doubling input speed
          powerup_state = NORMAL;
          primary_timer = millis();
          game.setupdate(-3);
        } else if (millis() - primary_timer >= 2000 || right_button > 1) {
            powerup_state = BLOCK_DELETE;
            primary_timer = millis();
            game.setupdate(0);
        }
      }else {
            powerup_state = BLOCK_DELETE;
            primary_timer = millis();
            game.setupdate(0);
      }
      break;

    case BLOCK_DELETE:
      if (game_score >= 7) 
      {
        tft.setTextColor(BACKGROUND);
        tft.setCursor(80,150,1);
        tft.println(last_powerup);
        game.game_cursor.setColor(TFT_MAGENTA);
        tft.setTextColor(TFT_MAGENTA);
        tft.setCursor(80,150,1);
        tft.println("DELETE");
        strcpy(last_powerup, "DELETE");
        if (left_button > 0) 
        {
          game.game_block.remove();
          powerup_state = NORMAL;
          primary_timer = millis();
          game.setupdate(-4);
        } 
        else if (millis() - primary_timer >= 2000 || right_button > 1) {
            powerup_state = IMU_BALL;
            primary_timer = millis();
            game.setupdate(0);
        }
      } 
      else {
            powerup_state = IMU_BALL;
            primary_timer = millis();
            game.setupdate(0);
      }
      break;
        
    case IMU_BALL:
        if (game_score >= 8) 
        {
          tft.setTextColor(BACKGROUND);
          tft.setCursor(80,150,1);
          tft.println(last_powerup);
          game.game_cursor.setColor(TFT_YELLOW);
          tft.setTextColor(TFT_YELLOW);
          tft.setCursor(80,150,1);
          tft.println("IMU BALL");
          strcpy(last_powerup, "IMU BALL");
          if (left_button > 0) 
          {
             imu_ball = true;
             imu_ball_timer = millis();
             powerup_state = NORMAL;
             primary_timer = millis(); 
             game.setupdate(-5); 
          } 
          else if (millis() - primary_timer >= 2000 || right_button > 1) 
          {
              powerup_state = NORMAL;
              primary_timer = millis();
              game.setupdate(0);
          }
        } else 
        {
            powerup_state = NORMAL;
            primary_timer = millis();
            game.setupdate(0);
        }
        break;
  }
}

void game_fsm(){
  //We Initialize The Session Best Score
  switch(display_state) 
  {
    
    case START:
      {
      imu.readAccelData(imu.accelCount); //We Read Our IMU'S Measurements
      float y = imu.accelCount[1] * imu.aRes;

      getuser.update(-y,right_button.update(),username);
      Intro(-y);
      if (left_button.update() > 0) 
      {
        tft.fillScreen(BACKGROUND);
        //Update Our Timers
        looptimer = millis();
        timestamp = millis();
        primary_timer = millis();
        
        game.setup();

        display_state = GAME;
        reset = 1;
      }
      break;
      }
    case GAME:
    
      imu.readAccelData(imu.accelCount); //We Read Our IMU'S Measurements
      
      //We Define The Horizontal And Vertical Components Of Our Forces
      float x = imu.accelCount[0] * imu.aRes;
      float y = imu.accelCount[1] * imu.aRes;

      //We Define A Variable That Checks The Bound For Ball Reset Purposes
      int boundcheck = 0;
    
      int status = game.getstatus(); //We Find Whether Our Game Session Has Ended
      updating = game.getupdate(); //We Find Whether We Made A Power-Up Decision

      //We Find The Portion Of The Score That Comes From Breaking Bricks
      float tempscore = 0;
      for (int i = 0; i < 60; i += 1) {
          if (strncmp(bricks+i, "0", 1) == 0) {
            tempscore += 1;
          }
      }
      //We Get The Portion Of The Game Score That Comes From Hits And Time Passage
      hitcounter = game.gethits();
      timestamp = (millis() - timestamp)/1000;

      //We Check If The Player Hit The Ball At All
      if (hitcounter >= 1) {
        game_score = (tempscore) + 12/(1 + hitcounter) + 12/(1+timestamp) + updating;
      }
      else {
        game_score = (tempscore) + 1/(1+timestamp) + updating;
      }

      //If Our Game Has Ended And We Haven't Exited Yet
      if (status == 1 && exited == 0) {
        char sbody[200];
        //We Update The Best Score Of The Session
        if (game_score > session_best) {
            session_best = game_score;

            Serial.print("uhhhh: ");
            Serial.println(session_best);

            //We Define Our POST Request
            memset(sbody, 0, sizeof (sbody) ); //for body
            memset(score_request, 0, sizeof(score_request) );
            memset(score_response, 0, sizeof(score_response) );
            sprintf(sbody, "user=%s&score=%f", username, session_best); //FOR ISAAK: I was thinking that you should return the player's highest score if the score parameter is set to -1.
            body_len = strlen(sbody); //calculate body length (for header reporting)
            sprintf(score_request,"POST http://608dev-2.net/sandbox/sc/team110/isaakWeek2/leaderboard.py HTTP/1.1\r\n");
            strcat(score_request,"Host: 608dev-2.net\r\n");
            strcat(score_request,"Content-Type: application/x-www-form-urlencoded\r\n");
            sprintf(score_request+strlen(score_request),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
            strcat(score_request,"\r\n"); //new line from header to body
            strcat(score_request,sbody); //body
            strcat(score_request,"\r\n"); //header

            //We Call On A Lab Defined Function For A POST Request
            do_http_request("608dev-2.net", score_request, score_response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
        }

        //We Define Our POST Request Meant For Getting Our Score
        memset(sbody, 0, sizeof (sbody) ); //for body
        memset(score_request, 0, sizeof(score_request) );
        memset(score_response, 0, sizeof(score_response) );
        sprintf(sbody, "user=%s&score=%d", username, -1); //FOR ISAAK: I was thinking that you should return the player's highest score if the score parameter is set to -1.
        body_len = strlen(sbody); //calculate body length (for header reporting)
        sprintf(score_request,"POST http://608dev-2.net/sandbox/sc/team110/isaakWeek2/leaderboard.py HTTP/1.1\r\n");
        strcat(score_request,"Host: 608dev-2.net\r\n");
        strcat(score_request,"Content-Type: application/x-www-form-urlencoded\r\n");
        sprintf(score_request+strlen(score_request),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
        strcat(score_request,"\r\n"); //new line from header to body
        strcat(score_request,sbody); //body
        strcat(score_request,"\r\n"); //header
  
        //We Call On A Lab Defined Function For A POST Request
        do_http_request("608dev-2.net", score_request, score_response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);

        //We Set Our Player's Best Score To The Response We Get From The Server
        player_best = atoi(score_response);
        
        tft.fillScreen(BACKGROUND);
        exited = 1;
      }
      //If Our Game Session Is Still Going On
      else if (status == 0) {
        
        //If The Ball Has To Reset
        if (game.getResetting() == 1) {
          reset = 1;
          start = 0;
        }
        //If The User Shoots The Ball
        else if (right_button.update() > 0) {
          reset = 0;
          start = 1;
        }
        //If The Ball Is Moving Normally
        else {
          reset = 0;
          start = 0;
        }

        if (imu_ball) {
          if (millis() - imu_ball_timer < 4000){
           BALL_Y_FORCE = -10000 * imu.accelCount[0] * imu.aRes;
           BALL_X_FORCE = -10000 * imu.accelCount[1] * imu.aRes;
          } else {
            BALL_Y_FORCE = 0;
            BALL_X_FORCE = 0;
            imu_ball = false;
          }
        }
        game.step(BALL_X_FORCE,BALL_Y_FORCE, -(y-0.04)*CONST, reset, start); //We Take A Step In Our Game Session
        if ((millis() - last_time) > GETTING_PERIOD){ // GETTING_PERIOD since last lookup? Look up again
          //formulate GET request...first line:
          sprintf(REQUEST_BUFFER,"GET http://608dev-2.net/sandbox/sc/team110/isaakWeek2/updateESP.py HTTP/1.1\r\n");
          strcat(REQUEST_BUFFER,"Host: 608dev-2.net\r\n"); //add more to the end
          strcat(REQUEST_BUFFER,"\r\n"); //add blank line!
          //submit to function that performs GET.  It will return output using response_buffer char array
          do_http_GET("608dev-2.net", REQUEST_BUFFER, RESPONSE_BUFFER, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,false);
          Serial.println(RESPONSE_BUFFER); //print to serial monitor
          last_time = millis();//remember when this happened so we perform next lookup in GETTING_PERIOD milliseconds
        }
        POWERUP_COLORS(left_button.update(), right_button.update(), game_score);
      }
      
      if (status == 1 && exited == 1) {
        tft.setCursor(0,0,1);
        tft.println("Game Over");
        tft.print("Score: ");
        tft.println(game_score);
        tft.println("Current Best Score: ");
        tft.println(session_best);
        tft.println("Your Total Best Score: ");
        tft.println(player_best);
        tft.print("Press RIGHT button");
        tft.print(" torestart!");

        if (right_button.update() > 0) {        
          tft.fillScreen(BACKGROUND);
          
          inforeset();
          game.setstatus(0);
          game.setboundcounter(0);
          
        }
      }
      //For The Purposes Of Debugging We Update Our Looptimer
      while (millis() - looptimer < LOOP_PERIOD); 
      looptimer = millis(); 
      xSemaphoreGive(semaphore); 
      break;
  }
}

//We Define How We Reset Our Relevant Information
void inforeset() 
{

  //We Set Our Relevant Match Values
  memset(bricks, 0, sizeof(bricks));
  boundcount = 0;
  exited = 0;
  start = 0;
  reset = 0;
  game.resethits();
  game.setupdate(0);
  memset(blocks, 0, sizeof(blocks));
  num_blocks = 0;

  //We Randomize Our Session
  randomSeed(analogRead(0)); 
  //We Update Our Timers
  looptimer = millis();
  timestamp = millis();

  //We Randomize Our Session
  randomSeed(analogRead(0)); 

  //Initialize display state
  display_state = START;
  powerup_state = NORMAL;

  //Resets Username Getting Protocol
  getuser.reset();
}


/*----------------------------------
 * char_append Function:
 * Arguments:
 *    char* buff: pointer to character array which we will append a
 *    char c: 
 *    uint16_t buff_size: size of buffer buff
 *    
 * Return value: 
 *    boolean: True if character appended, False if not appended (indicating buffer full)
 */
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}


/*----------------------------------
 * do_http_request Function:
 * Arguments:
 *    char* host: null-terminated char-array containing host to connect to
 *    char* request: null-terminated char-arry containing properly formatted HTTP request
 *    char* response: char-array used as output for function to contain response
 *    uint16_t response_size: size of response buffer (in bytes)
 *    uint16_t response_timeout: duration we'll wait (in ms) for a response from server
 *    uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
 * Return value:
 *    void (none)
 */
void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

void do_http_GET(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}
  
