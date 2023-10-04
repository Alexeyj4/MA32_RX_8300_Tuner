#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <AD9833.h>   // библиотека для работы с модулем AD9833
#include <Encoder.h>

//begin setup
const int FNC_PIN=10; // Fsync Required by the AD9833 Module
const int from_opa_pin=14; // sine from maup from opa 8300Hz (A0)
const int enc_a_pin=2; //encoder A pin
const int enc_b_pin=3; //encoder B pin
const int enc_btn_pin=7; //encoder Button pin
const int manual_switch_pin=4; // switch pin //switch for manual set 8300

const int SCREEN_WIDTH=128; // OLED display width, in pixels
const int SCREEN_HEIGHT=64; // OLED display height, in pixels
const int min_scan_freq=8100;//start freq to scam
const int max_scan_freq=8500;//stop freq to scan
const int freqinc=2;//freq increment for scan//def=2
const int nmeas=300; //iterations for amp measure//def=300
const int period=15624; //15624=1сек; 3125=0,2сек
const int centerfreq=8300; // central freq
const int ADCcoef=206; //U(V)=ADC indications (int)/ADCcoef(int)
const int lowamp=120; //low amp threshold //120=about 1v
const int minmeanthr=248; //mean threshold //248=about 1.2v //313=about 1.5v
const int maxmeanthr=372; //mean threshold //378=about 1.8v
const int min_scan_freqthr=8270; //freq threshold
const int max_scan_freqthr=8330; //freq threshold
const int res_freq_coef=8; //coef for resonance freq meas//+8 def
const int min_level_thr=30; //minimum level threshold (overload)//real minimum=0
const int max_level_thr=554; //maximum level threshold (overload)//real maximum=583
const int display_amp_coef=4; //=((real maximum=583)-(real minimum=0))/(SCREEN_WIDTH=128)
const int enc_polarity=-1;  //encoder polarity (A/B;+/-)// -1 or +1
const int first_string=20;  //first string on LCD
const int second_string=41;  //second string on LCD
const int red_led_pin=8; //NOT OK led //def=8
const int green_led_pin=9; //OK led //def=9
const int dc_coef=216;// dc=mean/dc_coef//216-def
//end setup

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Encoder myEnc(enc_b_pin, enc_a_pin); // avoid using pins with LEDs attached

int mean=0; //mean (DC)
int curfreq=centerfreq;
int Umeas;
int maxamp=0; //max ampliude for search resonance
int resfreq=0; //measured freq on max amp (resonance)
long i=0; //debug iterator
int Ulo; //min measured voltage
int Uhi; //max measured voltage
int amp; //counted amplitude
int overload; //overload flag (amp>3v)
int ok=0; //test result (red/green)
int enc_oldPosition  = 0; //encoder old position
int display_level_i=0; //display level bypass iterator (for automatic mode only). For faster resinance count
int display_dc_i=0; //display DC bypass iterator (for manual mode only). For faster level count
String old_mode=""; //old mode "auto" or "manual"


void display_level(){//show level bar
  
  display.fillRect(0, SCREEN_HEIGHT-10, SCREEN_WIDTH, 10, BLACK);
  display.fillRect(0, SCREEN_HEIGHT-10, amp/display_amp_coef, 10, WHITE);
  display.display(); 
}

void display_dc(){ //show 1.5v
    display.fillRect(0, first_string+1, SCREEN_WIDTH, second_string-first_string+1 , BLACK);
    display.setCursor(SCREEN_WIDTH/2,second_string);
    mean=(Uhi+Ulo)/2;
    display.print(float(mean)/dc_coef);
    display.print("vdc");//debug
    display.display(); 
}

void msg(String s) //pritn data on LCD and Serial //modes: "loading"-start screen, ""-default mode, "manual"
{
  if(s=="loading"){
    display.setFont(&FreeSerif9pt7b);
    display.clearDisplay();
    display.setTextSize(1);             
    display.setTextColor(WHITE);        
    display.setCursor(0,first_string);             
    display.println("Loading...");
    display.display();
    delay(1000);
    display.clearDisplay();

  }
  
  if(s==""){

    ok=1;
    mean=(Uhi+Ulo)/2;
    
    display.fillRect(0, 0, SCREEN_WIDTH, 54, BLACK); 
    
    Serial.print(resfreq);
    Serial.print(" amp=");
    Serial.print(maxamp);
    Serial.print(" mean=");
    Serial.print(mean);
    if(overload==1){
      Serial.print(" OVERLOAD! ");
      display.setCursor(0,second_string);
      display.print("overload");
      ok=0;
    }
    if(maxamp<lowamp){
      Serial.print(" LOW AMP! ");
      display.setCursor(0,second_string);
      display.print("low amp");
      ok=0;
    }
    if(mean<minmeanthr){
      Serial.print(" LOW 1.5V! ");
      display.setCursor(SCREEN_WIDTH/2+20,second_string);
      display.print("no1.5");
      ok=0;
    }
    if(mean>maxmeanthr){
      Serial.print(" HIGH 1.5V! ");
      display.setCursor(SCREEN_WIDTH/2+20,second_string);
      display.print("no1.5");
      ok=0;
    }
    if(resfreq<min_scan_freqthr){
      Serial.print(" LOW FREQ! ");
      display.setCursor(SCREEN_WIDTH/2,first_string);
      display.print("decr C");
      ok=0;
    }
    if(resfreq>max_scan_freqthr){
      Serial.print(" HIGH FREQ! ");
      display.setCursor(SCREEN_WIDTH/2,first_string);
      display.print("add C");
      ok=0;
    }
      
    if(ok==1){
      Serial.print(" GOOD!");
      digitalWrite(green_led_pin,1);
      digitalWrite(red_led_pin,0);
    }
    if(ok==0){
      Serial.print(" BAD!");
      digitalWrite(green_led_pin,0);
      digitalWrite(red_led_pin,1);
    }
    Serial.println("");    
    
    display.setCursor(0,first_string);
    display.print(resfreq);
    display.display();    
  }

  if(s=="manual"){
  
    display.fillRect(0, 0, SCREEN_WIDTH, first_string+1, BLACK);
    display.setCursor(0,first_string);
    display.print(curfreq);
    display.display();
  }
}

void resetU()
{
  Ulo=1024;
  Uhi=0;
}

void resetres()
{
  resfreq=0;
  maxamp=0;   
}

void measure_level()
{  
  resetU();
  for(int i=0;i<=nmeas;i++)
  { 
    Umeas=analogRead(from_opa_pin);
    if(Umeas>Uhi)Uhi=Umeas;
    if(Umeas<Ulo)Ulo=Umeas;
  }
  
  amp=Uhi-Ulo;
  
  if(Ulo<min_level_thr or Uhi>max_level_thr)overload=1;
    
}


AD9833 gen(FNC_PIN);

void setup() {

  Serial.begin(115200);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  msg("loading");
  
  ADCSRA |= (1 << ADPS2);                     //Биту ADPS2 присваиваем единицу - коэффициент деления 16 //ADC speed up accuracy down
  ADCSRA &= ~ ((1 << ADPS1) | (1 << ADPS0));  //Битам ADPS1 и ADPS0 присваиваем нули  //ADC speed up accuracy down

  resetU();  

  pinMode(from_opa_pin, INPUT);
  pinMode(manual_switch_pin, INPUT_PULLUP);//debug
  pinMode(enc_a_pin, INPUT_PULLUP);//A
  pinMode(enc_b_pin, INPUT_PULLUP);//B
  pinMode(enc_btn_pin, INPUT_PULLUP);//button
  pinMode(red_led_pin, OUTPUT);
  pinMode(green_led_pin, OUTPUT);
  
  delay(10);
  analogRead(from_opa_pin);//first read for reduce error
  delay(10);
  
  gen.Begin(); // это команда должна быть первой после того как вы объявили объект AD9833
  gen.ApplySignal(SINE_WAVE,REG0,min_scan_freq);
  gen.EnableOutput(true);
  delay(10); // задержка 1 сек.
}



void loop()
{

if(digitalRead(manual_switch_pin)==0){ //manual mode on //def=0

    if(old_mode!="manual"){
      curfreq=centerfreq; 
      gen.ApplySignal(SINE_WAVE,REG0,curfreq); 
      old_mode="manual";
      digitalWrite(green_led_pin,0);
      digitalWrite(red_led_pin,0); 
      myEnc.write(0); 
      msg("manual");
    }
    
    measure_level();
    display_level();
    if(display_dc_i>=20){
      display_dc(); 
      display_dc_i=0;
    } else display_dc_i++;  
        
    int enc_newPosition = myEnc.read();
    
    if ((enc_newPosition - enc_oldPosition)>=4) {
      if(digitalRead(enc_btn_pin)==0)curfreq=curfreq+enc_polarity; else curfreq=curfreq+(enc_polarity*10);          
          
      gen.ApplySignal(SINE_WAVE,REG0,curfreq);
      Serial.println(curfreq);
      msg("manual");
            
      enc_oldPosition = 0;      
      myEnc.write(0);   
    }
      
    if ((enc_newPosition - enc_oldPosition)<=-4) {
      if(digitalRead(enc_btn_pin)==0)curfreq=curfreq-enc_polarity; else curfreq=curfreq-(enc_polarity*10);
        
      gen.ApplySignal(SINE_WAVE,REG0,curfreq);
      Serial.println(curfreq);    
      msg("manual");
      
      enc_oldPosition = 0;      
      myEnc.write(0);   
        
    }
            

} else { //automatic mode

    old_mode="auto";
  
    gen.ApplySignal(SINE_WAVE,REG0,curfreq);
    
    measure_level();
    if(display_level_i>=20){
      display_level();
      display_level_i=0;
    } else display_level_i++;
    
    if(amp>maxamp)
    {
      maxamp=amp;
      resfreq=curfreq-res_freq_coef;      
    }
    curfreq+=freqinc;
    if(curfreq>=max_scan_freq)
    {
      curfreq=min_scan_freq;
      msg("");    
      resetres(); //reset resonance data
      overload=0; //reset overload data
    }
  }
  
  
}
