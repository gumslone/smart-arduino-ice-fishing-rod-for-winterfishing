#include <avr/pgmspace.h>

#define ENC_DECODER (1 << 2)
#define ENC_HALFSTEP

#include <ClickEncoder.h>
//#include <TimerOne.h>
#include <MsTimer2.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#include <Servo.h>

Servo myservo;  // create servo object to control a servo

byte prev_pos = 10;
byte pos = 10;    // variable to store the servo position

byte servo_min_pos = 4; //min possible value for servo
byte servo_max_pos = 170; //max possible value for servo

byte servo_start_pos = 90;
byte servo_end_pos = 120;
byte servo_speed = 1; //steps
bool servo_attached = false; //to prevent the servo jittering

byte custom_servo_start_pos = servo_start_pos;
byte custom_servo_end_pos = servo_start_pos+5;
byte custom_servo_speed = servo_speed;
unsigned long custom_servo_delay = 100;

volatile byte duty [28] = { //Default path array (saw shape)
 140, 160, 180, 200, 220, 240, 260,
 280, 260, 240, 220, 200, 180, 160,
 140, 120, 100, 80, 60, 40, 20,
 0, 20, 40, 60, 80, 100, 120
};

// texts for menus
const char  main_menu0[] PROGMEM = "Fishing Modes 1";
const char  main_menu1[] PROGMEM = "Fishing Modes 2";
const char  main_menu2[] PROGMEM = "Mixed Modes";
const char  main_menu3[] PROGMEM = "Custom Mode";
const char  main_menu4[] PROGMEM = "Servo Settings";

const char  fishing_mode_menu0[] PROGMEM = "Mode 1";
const char  fishing_mode_menu1[] PROGMEM = "Mode 2";
const char  fishing_mode_menu2[] PROGMEM = "Mode 3";
const char  fishing_mode_menu3[] PROGMEM = "Mode 4";
const char  fishing_mode_menu4[] PROGMEM = "Mode 5";
const char  fishing_mode_menu5[] PROGMEM = "Mode 6";
const char  fishing_mode_menu6[] PROGMEM = "Mode 7";
const char  fishing_mode_menu7[] PROGMEM = "Mode 8";
const char  fishing_mode_menu8[] PROGMEM = "Mode 9";
const char  fishing_mode_menu9[] PROGMEM = "Mode 10";

const char  mixed_mode_menu0[] PROGMEM = "Mix Mode 1";
const char  mixed_mode_menu1[] PROGMEM = "Mix Mode 2";
const char  mixed_mode_menu2[] PROGMEM = "Mix Mode 3";
const char  mixed_mode_menu3[] PROGMEM = "Mix Mode 4";
const char  mixed_mode_menu4[] PROGMEM = "Mix Mode 5";

const char  servo_setting_menu0[] PROGMEM = "Servo Min Pos";
const char  servo_setting_menu1[] PROGMEM = "Servo Max Pos";
const char  servo_setting_menu2[] PROGMEM = "Start Pos";
const char  servo_setting_menu3[] PROGMEM = "End Pos";
const char  servo_setting_menu4[] PROGMEM = "Speed";

const char  custom_mode_menu0[] PROGMEM = "Start Pos";
const char  custom_mode_menu1[] PROGMEM = "End Pos";
const char  custom_mode_menu2[] PROGMEM = "Speed";
const char  custom_mode_menu3[] PROGMEM = "Delay";
const char  custom_mode_menu4[] PROGMEM = "START";
////////////////////////////////////////////////////////////////
// menus - first item is menu title and it does not count toward cnt
const char* const main_menu[] PROGMEM = {
  main_menu0,
  main_menu1,main_menu2,main_menu3,main_menu4};

const char* const fishing_modes_menu1[] PROGMEM = {
  fishing_mode_menu0,
  fishing_mode_menu1,fishing_mode_menu2,fishing_mode_menu3,fishing_mode_menu4};

const char* const fishing_modes_menu2[] PROGMEM = {
  fishing_mode_menu5,
  fishing_mode_menu6,fishing_mode_menu7,fishing_mode_menu8,fishing_mode_menu9};

const char* const mixed_modes_menu[] PROGMEM = {
  mixed_mode_menu0,
  mixed_mode_menu1,mixed_mode_menu2,mixed_mode_menu3,mixed_mode_menu4};

const char* const custom_mode_menu[] PROGMEM = {
  custom_mode_menu0,
  custom_mode_menu1,custom_mode_menu2,custom_mode_menu3,custom_mode_menu4};

const char* const servo_settings_menu[] PROGMEM = {
  servo_setting_menu0,
  servo_setting_menu1,servo_setting_menu2,servo_setting_menu3,servo_setting_menu4};

char buffer[30];    // make sure this is large enough for the largest string it must hold

byte curr_menu_level = 0;
byte prev_menu[4]={0,0,0,0};
//int menu_values[5]={0,0,0,0,0};
//bool show_menu_values = false;
byte curr_menu = 0;
byte curr_menu_item = 0; //selected item
byte curr_menu_items = 6; // total items in current menu
byte menu_mode = 0; //0-change menu, 1 change value

unsigned long timer1_time, /*timer2_time,*/
timer1_last_time/*, timer2_last_time*/;



ClickEncoder *encoder;
int16_t encoder_last, encoder_value;

void timerIsr() {
  encoder->service();
}


void setup() {
  //Serial.begin(9600);
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  myservo.write(servo_start_pos);
  delay(50);
  //
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();
  myservo.detach();
   encoder = new ClickEncoder(A1, A0, A2, 4);
  //(rot,rot,btn,stepsPerNotchstepsPerNotch)
  
  //Timer1.initialize(1000);
  //Timer1.attachInterrupt(timerIsr); 
  MsTimer2::set(1, timerIsr); // 500ms period
  MsTimer2::start();
  timer1_time = micros();
  timer1_last_time = micros();
  //timer2_time = micros();
  //timer2_last_time = micros();

  
  encoder_last = -1;

  show_menu(main_menu);
}

void loop() {  
  
  
  encoder_value += encoder->getValue();
  
    if (encoder_value != encoder_last) {
      
      //Serial.print("Encoder Value: ");
      //Serial.println(encoder_value);

          if(menu_mode==0)
          {      
                  if(encoder_value <= 0)
                  {
                    curr_menu_item = 0;
                    encoder_value=0;
                  }
                  if(encoder_value >= curr_menu_items)
                  {
                    curr_menu_item = curr_menu_items-1;
                    encoder_value = curr_menu_items-1;
                  }
                  curr_menu_item = encoder_value;
                  display_menu();
          }
          encoder_last = encoder_value;
    }


    if(menu_mode==0)
    {
       // display_menu();  
    }
    else if(menu_mode == 1)
    {
        if(curr_menu == 4)//custom settings menu
        {
            if(curr_menu_item==0)//start pos
            {
                custom_set_servo_start_pos();
            }
            else if(curr_menu_item==1)//end pos
            {
                custom_set_servo_end_pos();
            }
            else if(curr_menu_item==2)//speed
            {
                custom_set_servo_speed();
            }
            else if(curr_menu_item==3)//speed
            {
                custom_set_servo_delay();
            }
        } 
        else if(curr_menu == 5)//servo settings menu
        {
            if(curr_menu_item==0)//servo min pos
            {
                set_servo_min_pos();
            }
            else if(curr_menu_item==1)//servo max pos
            {
                set_servo_max_pos();
            }
            else if(curr_menu_item==2)//start pos
            {
                set_servo_start_pos();
            }
            else if(curr_menu_item==3)//end pos
            {
                set_servo_end_pos();
            }
            else if(curr_menu_item==4)//speed
            {
                set_servo_speed();
            }
        } 
    }
    else if(menu_mode == 2)
    {
        if(curr_menu == 1)//fishing modes menu
        {
            pos = servo_start_pos;
            prev_pos = servo_start_pos-1; 
            if(curr_menu_item==0)//1
            {
                
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+1, 1, 1000);
            }
            else if(curr_menu_item==1)//2
            {
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+2, 1, 1000);
            }
            else if(curr_menu_item==2)//3
            {
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+3, 1, 10000);
            }
            else if(curr_menu_item==3)//4
            {
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+6, 2, 10000);
            }
            else if(curr_menu_item==4)//5
            {
               fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+10, 2, 10000); 
            }
        }
        else if(curr_menu == 2)//fishing modes menu 2
        {
            pos = servo_start_pos;
            prev_pos = servo_start_pos-1; 
            if(curr_menu_item==0)//1
            {
                
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+2, 1, 1000000);
            }
            else if(curr_menu_item==1)//2
            {
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+5, 2, 1000000);
            }
            else if(curr_menu_item==2)//3
            {
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+2, 1, 2000000);
            }
            else if(curr_menu_item==3)//4
            {
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+5, 2, 2000000);
            }
            else if(curr_menu_item==4)//5
            {
                fishing_mode(curr_menu_item+1, servo_start_pos, servo_start_pos+10, 1, 2000000);
            }
        }
        else if(curr_menu == 3)//fishing mixed menu 2
        {
            pos = servo_start_pos;
            prev_pos = servo_start_pos-1; 
            if(curr_menu_item==0)//1
            {
                
                fishing_mixed_mode(curr_menu_item+1, duty[28], 0, 1);
            }
            else if(curr_menu_item==1)//2
            {
                fishing_mixed_mode(curr_menu_item+1, duty[28], 1, 1);
            }
            else if(curr_menu_item==2)//3
            {
                fishing_mixed_mode(curr_menu_item+1, duty[28], 2, 1);
            }
            else if(curr_menu_item==3)//4
            {
                fishing_mixed_mode(curr_menu_item+1, duty[28], 3, 1);
            }
            else if(curr_menu_item==4)//5
            {
                fishing_mixed_mode(curr_menu_item+1, duty[28], 4, 1);
            }
        }

        else if(curr_menu == 4)//custom settings menu
        {
            pos = servo_start_pos;
            prev_pos = servo_start_pos-1; 
            if(curr_menu_item==4)//
            {
                custom_mode_start();
            }
        }


//custom_mode_start()
//fishing_mixed_mode(byte fishing_mode, volatile byte movement[28], byte move_speed, unsigned long delay_after)

        
      //fishing_mode(byte fishing_mode, byte start_pos, byte end_pos, byte move_speed, byte delay_after)



      
    }

  
  ClickEncoder::Button b = encoder->getButton();
  if (b != ClickEncoder::Open) {
    //Serial.print("Button: ");
    //#define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      //VERBOSECASE(ClickEncoder::Pressed);
      //VERBOSECASE(ClickEncoder::Held)
      //VERBOSECASE(ClickEncoder::Released)
      //VERBOSECASE(ClickEncoder::Clicked)
      case ClickEncoder::Clicked:
        
        if(menu_mode==0)
        {
          encoder_last = -1;
          menu_select();
        }
        else if(menu_mode==1||menu_mode==2)
        {
          encoder_last = -1;
          menu_mode = 0;
          curr_menu_level = curr_menu_level -1;
          curr_menu = prev_menu[curr_menu_level];
          myservo.detach();
          servo_attached = false;
        }
        break;
      case ClickEncoder::DoubleClicked:
          //Serial.println("ClickEncoder::DoubleClicked");
          //encoder->setAccelerationEnabled(!encoder->getAccelerationEnabled());
          //Serial.print("  Acceleration is ");
          //Serial.println((encoder->getAccelerationEnabled()) ? "enabled" : "disabled");
        break;
    }
  }    
}


void show_menu(char **menu)
{


   
 // text display tests
  display.setTextSize(1);
  display.clearDisplay();
  for(byte i=0;i<=4;i++)
  {
    display.setCursor(0,(i*10));
    if (i==curr_menu_item) 
      {
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      }
    else display.setTextColor(WHITE);
    
    strcpy_P(buffer, (char*)pgm_read_word(&(menu[i]))); // Necessary casts and dereferencing, just copy.
    display.println(buffer);
  }
  
  if(curr_menu > 0)
  {
    display.setCursor(115,50);
      if (5==curr_menu_item) 
      {
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      }
      else display.setTextColor(WHITE);
    display.println("<<");  
  }
  
  display.display();
  

}

void menu_select()
{

              if(curr_menu==0) //main menu
              {
                    if(curr_menu_item==0) //fishing modes 2
                    {
                      curr_menu_level = 1;
                      curr_menu = 1;
                      //show_menu_values = false;
                      prev_menu[curr_menu_level] = curr_menu;
                    }
                    else if(curr_menu_item==1) //fishing modes 2
                    {
                      curr_menu_level = 1;
                      curr_menu = 2;
                      //show_menu_values = false;
                      prev_menu[curr_menu_level] = curr_menu; 
                    }
                    else if(curr_menu_item==2) //custom modes
                    {
                      curr_menu_level = 1;
                      curr_menu = 3;
                      //show_menu_values = false;
                      prev_menu[curr_menu_level] = curr_menu; 
                    }
                    else if(curr_menu_item==3) //servo settings
                    {
                      curr_menu_level = 1;
                      curr_menu = 4;
                      //show_menu_values = true;
                      prev_menu[curr_menu_level] = curr_menu; 
                    }
                    else if(curr_menu_item==4) //about
                    {
                      curr_menu_level = 1;
                      curr_menu = 5;
                      //show_menu_values = false;
                      prev_menu[curr_menu_level] = curr_menu; 
                    }
                
              }
      
              else if(curr_menu==1) //fishing modes 1
              {
                    if(curr_menu_item==0) //fishing mode 1
                    {
                      curr_menu_level = 2;
                      //curr_menu = 1;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==1) //fishing mode2
                    {
                      curr_menu_level = 2;
                      //curr_menu = 2;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==2) //fishing mode3
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==3) //fishing mode4
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==4) //fishing mode5
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==5) //back button
                    {
                      curr_menu_level = curr_menu_level-1;
                      curr_menu = prev_menu[curr_menu_level];
                    }
                
              }
              else if(curr_menu==2) //fishing mode 2
              {
                    if(curr_menu_item==0) //fishing mode 6
                    {
                      curr_menu_level = 2;
                      //curr_menu = 1;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==1) //fishing mode7
                    {
                      curr_menu_level = 2;
                      //curr_menu = 2;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==2) //fishing mode8
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==3) //fishing mode9
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==4) //fishing mode10
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu;
                      menu_mode = 2; 
                    }
                    else if(curr_menu_item==5) //back button
                    {
                      curr_menu_level = curr_menu_level-1;
                      curr_menu = prev_menu[curr_menu_level];
                    }
                    
              }
              else if(curr_menu==3) //mixed fishing mode
              {
                    if(curr_menu_item==0) //mixed fishing mode 1
                    {
                      curr_menu_level = 2;
                      //curr_menu = 1;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==1) //mixed fishing mode 2
                    {
                      curr_menu_level = 2;
                      //curr_menu = 2;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==2) //mixed fishing mode 3
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==3) //mixed fishing mode 4
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                    }
                    else if(curr_menu_item==4) //mixed fishing mode 5
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu;
                      menu_mode = 2; 
                    }
                    else if(curr_menu_item==5) //back button
                    {
                      curr_menu_level = curr_menu_level-1;
                      curr_menu = prev_menu[curr_menu_level];
                    }
                    
              }
              else if(curr_menu==4) //custom mode
              {
                    if(curr_menu_item==0) //start pos
                    {
                      curr_menu_level = 2;
                      //curr_menu = 1;
                      prev_menu[curr_menu_level] = curr_menu;
                      menu_mode = 1; 
                      encoder_value = custom_servo_start_pos;
                    }
                    else if(curr_menu_item==1) //end pos
                    {
                      curr_menu_level = 2;
                      //curr_menu = 2;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 1;
                      encoder_value = custom_servo_end_pos;
                    }
                    else if(curr_menu_item==2) //speed
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 1;
                      encoder_value = custom_servo_speed;
                    }
                    else if(curr_menu_item==3) //delay
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 1;
                      encoder_value = custom_servo_delay/10;
                    }
                    else if(curr_menu_item==4) //start
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 2;
                      encoder_value = 0;
                      
                    }
                    else if(curr_menu_item==5) //back button
                    {
                      curr_menu_level = curr_menu_level-1;
                      curr_menu = prev_menu[curr_menu_level];
                    }
                
              }
              else if(curr_menu==5) //settings mode
              {
                    if(curr_menu_item==0) //servo min pos
                    {
                      curr_menu_level = 2;
                      //curr_menu = 1;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 1;
                      encoder_value = servo_min_pos;
                    }
                    else if(curr_menu_item==1) //servo max pos
                    {
                      curr_menu_level = 2;
                      //curr_menu = 2;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 1;
                      encoder_value = servo_max_pos;
                    }
                    else if(curr_menu_item==2) //start pos
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 1;
                      encoder_value = servo_start_pos;
                    }
                    else if(curr_menu_item==3) //end pos
                    {
                      curr_menu_level = 2;
                      //curr_menu = 3;
                      prev_menu[curr_menu_level] = curr_menu;
                      menu_mode = 1;
                      encoder_value = servo_end_pos; 
                    }
                    else if(curr_menu_item==4) //speed
                    {
                      curr_menu_level = 2;
                      //curr_menu = 1;
                      prev_menu[curr_menu_level] = curr_menu; 
                      menu_mode = 1;
                      encoder_value = servo_speed;
                    }
                    else if(curr_menu_item==5) //back button
                    {
                      curr_menu_level = curr_menu_level-1;
                      curr_menu = prev_menu[curr_menu_level];
                    }
                
              }

} //end of menu select

void display_menu()
{
          if(curr_menu==0)
          {
            show_menu(main_menu);
          }
          else if(curr_menu==1)
          {
            show_menu(fishing_modes_menu1);
          }
          else if(curr_menu==2)
          {
            show_menu(fishing_modes_menu2);
          }
          else if(curr_menu==3)
          {
            show_menu(mixed_modes_menu);
          }
          else if(curr_menu==4)
          {
            show_menu(custom_mode_menu);
          }
          else if(curr_menu==5)
          {
            show_menu(servo_settings_menu);
          }
}

void set_servo_min_pos()
{
    display.clearDisplay();
  // text display tests
  display.setTextSize(1);
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Servo start position");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      servo_min_pos = encoder_value*-1;
  }
  else
  {
      servo_min_pos = encoder_value;
  }

  if(servo_min_pos<0)
  {
    servo_min_pos = 0;
    encoder_value = 0;
  }
  else if(servo_min_pos>=servo_max_pos)
  {
    servo_min_pos = servo_max_pos-1;
    encoder_value = servo_max_pos-1;
  }
  myservo.attach(9);
  myservo.write(servo_min_pos);
  delay(30);
  myservo.detach();
  display.println(servo_min_pos);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}
void set_servo_max_pos()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Servo max position");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      servo_max_pos = encoder_value*-1;
  }
  else
  {
      servo_max_pos = encoder_value;
  }

  if(servo_max_pos<=servo_min_pos)
  {
    servo_max_pos = servo_min_pos+1;
    encoder_value = servo_min_pos+1;
  }
  else if(servo_max_pos>180)
  {
    servo_max_pos = 180;
    encoder_value = 180;
  }
  myservo.attach(9);
  myservo.write(servo_max_pos);
  delay(30);
  myservo.detach();
  display.println(servo_max_pos);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}

void set_servo_start_pos()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Servo start position");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      servo_start_pos = encoder_value*-1;
  }
  else
  {
      servo_start_pos = encoder_value;
  }

  if(servo_start_pos<servo_min_pos)
  {
    servo_start_pos = servo_min_pos;
    encoder_value = servo_min_pos;
  }
  else if(servo_start_pos>servo_max_pos)
  {
    servo_start_pos = servo_max_pos;
    encoder_value = servo_max_pos;
  }
  myservo.attach(9);
  myservo.write(servo_start_pos);
  delay(30);
  myservo.detach();
  display.println(servo_start_pos);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}

void set_servo_end_pos()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Servo end position");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      servo_end_pos = encoder_value*-1;
  }
  else
  {
      servo_end_pos = encoder_value;
  }

  if(servo_end_pos<servo_min_pos)
  {
    servo_end_pos = servo_min_pos;
    encoder_value = servo_min_pos;
  }
  else if(servo_end_pos>servo_max_pos)
  {
    servo_end_pos = servo_max_pos;
    encoder_value = servo_max_pos;
  }
  myservo.attach(9);
  myservo.write(servo_end_pos);
  delay(30);
  myservo.detach();
  
  
  display.println(servo_end_pos);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}

void set_servo_speed()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Servo speed");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      servo_speed = encoder_value*-1;
  }
  else
  {
      servo_speed = encoder_value;
  }

  if(servo_speed<0)
  {
    servo_speed = 0;
    encoder_value = 0;
  }
  else if(servo_speed>50)
  {
    servo_speed = 50;
    encoder_value = 50;
  }
  display.println(servo_speed);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}


void custom_set_servo_start_pos()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Custom servo start position");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      custom_servo_start_pos = encoder_value*-1;
  }
  else
  {
      custom_servo_start_pos = encoder_value;
  }

  if(custom_servo_start_pos<servo_min_pos)
  {
    custom_servo_start_pos = servo_min_pos;
    encoder_value = servo_min_pos;
  }
  else if(custom_servo_start_pos>servo_max_pos)
  {
    custom_servo_start_pos = servo_max_pos;
    encoder_value = servo_max_pos;
  }
  myservo.attach(9);
  myservo.write(custom_servo_start_pos);
  delay(30);
  myservo.detach();
  display.println(custom_servo_start_pos);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}

void custom_set_servo_end_pos()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Custom servo end position");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      custom_servo_end_pos = encoder_value*-1;
  }
  else
  {
      custom_servo_end_pos = encoder_value;
  }

  if(custom_servo_end_pos<servo_min_pos)
  {
    custom_servo_end_pos = servo_min_pos;
    encoder_value = servo_min_pos;
  }
  else if(custom_servo_end_pos>servo_max_pos)
  {
    custom_servo_end_pos = servo_max_pos;
    encoder_value = servo_max_pos;
  }
  myservo.attach(9);
  myservo.write(custom_servo_end_pos);
  delay(30);
  myservo.detach();
  
  
  display.println(custom_servo_end_pos);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}

void custom_set_servo_speed()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Custom servo speed");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      custom_servo_speed = encoder_value*-1;
  }
  else
  {
      custom_servo_speed = encoder_value;
  }

  if(custom_servo_speed<0)
  {
    custom_servo_speed = 0;
    encoder_value = 0;
  }
  else if(custom_servo_speed>50)
  {
    custom_servo_speed = 50;
    encoder_value = 50;
  }
  display.println(custom_servo_speed);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}

void custom_set_servo_delay()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Custom servo delay in millis");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  if(encoder_value < 0)
  {
      custom_servo_delay = encoder_value*-10;
  }
  else
  {
      custom_servo_delay = encoder_value*10;
  }

  if(custom_servo_delay<0)
  {
    custom_servo_delay = 0;
    encoder_value = 0;
  }
  else if(custom_servo_delay>10000)
  {
    custom_servo_delay = 10000;
    encoder_value = 1000;
  }
  display.println(custom_servo_delay);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();
}


void fishing_mode(byte fishing_mode, byte start_pos, byte end_pos, byte move_speed, unsigned long delay_after)
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Fishing Mode:");
  
  display.setTextSize(1);
  display.setCursor(0,10);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  display.println(fishing_mode);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();   

  
  timer1_time = micros();
  if((timer1_time-timer1_last_time) >= delay_after)
  {
  
          if(end_pos>servo_max_pos)
          {
            end_pos = servo_max_pos;
          }
          else if(start_pos<servo_min_pos)
          {
            start_pos = servo_min_pos;
          }
        
          if(prev_pos > pos)
          {
            pos = pos - move_speed;
          }
          else
          {
            pos = pos + move_speed;
          }
        
          
          if(pos > end_pos)
          {
            pos = end_pos;
          }
          else if(pos<start_pos)
          {
            pos = start_pos;
          }
          if(servo_attached==false)
          {
            myservo.attach(9);
            servo_attached = true;
          }
          for (pos = start_pos; pos <= end_pos; pos += move_speed) { // goes from 0 degrees to 180 degrees
            // in steps of 1 degree
            if(pos>end_pos)
            {
              pos = end_pos;
            }
            myservo.write(pos);              // tell servo to go to position in variable 'pos'
            delay(15);                       // waits 15ms for the servo to reach the position
          }
          for (pos = end_pos; pos >= start_pos; pos -= move_speed) { // goes from 180 degrees to 0 degrees
            if(pos<start_pos)
            {
              pos=start_pos;
            }
            myservo.write(pos);              // tell servo to go to position in variable 'pos'
            delay(15);                       // waits 15ms for the servo to reach the position
          }
          if(delay_after>300000) //detach only for long delay
          {
            myservo.detach();
            servo_attached = false;
          }
          prev_pos = pos;
  
    timer1_last_time = micros();
  }
  

}



void fishing_mixed_mode(byte fishing_mode, volatile byte movement[28], byte move_speed, unsigned long delay_after)
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Fishing Mixed Mode:");
  
  display.setTextSize(1);
  display.setCursor(0,10);
  display.setTextColor(BLACK,WHITE); // 'inverted' text

  display.println(fishing_mode);

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();   

  
  timer1_time = micros();
  if((timer1_time-timer1_last_time) >= delay_after)
  {
  
          /*if(end_pos>servo_max_pos)
          {
            end_pos = servo_max_pos;
          }
          else if(start_pos<servo_min_pos)
          {
            start_pos = servo_min_pos;
          }*/
        
          if(prev_pos > pos)
          {
            pos = pos - move_speed;
          }
          else
          {
            pos = pos + move_speed;
          }
        
          
          /*if(pos > end_pos)
          {
            pos = end_pos;
          }
          else if(pos<start_pos)
          {
            pos = start_pos;
          }*/
          if(servo_attached==false)
          {
            myservo.attach(9);
            servo_attached = true;
          }
          
          for (byte i = 0; pos < 28; i++) { // goes from 0 degrees to 180 degrees
            // in steps of 1 degree
            //if(pos>end_pos)
            //{
            //  pos = end_pos;
            //}
            pos = 1500 + movement[i] + (move_speed*10);
            myservo.write(pos);              // tell servo to go to position in variable 'pos'
            //delay(15);                       // waits 15ms for the servo to reach the position
          }

          
          if(delay_after>300000) //detach only for long delay
          {
            myservo.detach();
            servo_attached = false;
          }
          prev_pos = pos;
  
    timer1_last_time = micros();
  }
  

}



void custom_mode_start()
{
  display.clearDisplay();
  // text display tests
  display.setTextSize(1);

  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.println("Custom Mode Start, encoder:");
  
  display.setTextSize(4);
  display.setCursor(0,20);
  display.setTextColor(BLACK,WHITE); // 'inverted' text
  display.println(encoder_value); 

  display.setTextSize(1);
  //if(curr_menu > 0)
  //{
    display.setCursor(115,50);
      //if (5==curr_menu_item) 
      //{
        display.setTextColor(BLACK,WHITE); // 'inverted' text
      //}
      //else display.setTextColor(WHITE);
    display.println("<<");  
  //}
  
  display.display();   

  
  timer1_time = micros();
  if((timer1_time-timer1_last_time) >= (custom_servo_delay * 1000))
  {
  
          /*if(end_pos>servo_max_pos)
          {
            end_pos = servo_max_pos;
          }
          else if(start_pos<servo_min_pos)
          {
            start_pos = servo_min_pos;
          }*/
        
          if(prev_pos > pos)
          {
            pos = pos - custom_servo_speed;
          }
          else
          {
            pos = pos + custom_servo_speed;
          }
        
          
          /*if(pos > end_pos)
          {
            pos = end_pos;
          }
          else if(pos<start_pos)
          {
            pos = start_pos;
          }*/
          if(servo_attached==false)
          {
            myservo.attach(9);
            servo_attached = true;
          }

          //custom_servo_speed = custom_servo_speed+encoder_value;
          
          for (pos = custom_servo_start_pos; pos <= custom_servo_end_pos; pos += custom_servo_speed) { // goes from 0 degrees to 180 degrees
            // in steps of 1 degree
            if(pos>custom_servo_end_pos)
            {
              pos = custom_servo_end_pos;
            }
            myservo.write(pos);              // tell servo to go to position in variable 'pos'
            delay(15);                       // waits 15ms for the servo to reach the position
          }
          for (pos = custom_servo_end_pos; pos >= custom_servo_start_pos; pos -= custom_servo_speed) { // goes from 180 degrees to 0 degrees
            if(pos<custom_servo_start_pos)
            {
              pos=custom_servo_start_pos;
            }
            myservo.write(pos);              // tell servo to go to position in variable 'pos'
            delay(15);                       // waits 15ms for the servo to reach the position
          }
          
          if(custom_servo_delay>300000) //detach only for long delay
          {
            myservo.detach();
            servo_attached = false;
          }
          prev_pos = pos;
  
    timer1_last_time = micros();
  }
  

}


