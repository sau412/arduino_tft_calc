// TFT LCD support
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library

// Touch screen support
#include <TouchScreen.h>

// Touch screen variables
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

int ts_minx,ts_maxx,ts_miny,ts_maxy;

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// LCD variables

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
// optional
#define LCD_RESET A4

// Human-readable colors
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY    0x7BEF
#define LIGHTGRAY 0xBDF7
#define DARKBLUE  0x000F

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// Program variables
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define CHAR_HEIGHT 8
#define CHAR_WIDTH 6

#define INPUT_STRING_OFFSET (CHAR_HEIGHT*0)
#define INPUT_STRING_CHAR_SIZE 2
#define OUTPUT_STRING_OFFSET (CHAR_HEIGHT*3)
#define OUTPUT_STRING_CHAR_SIZE 2

#define NUM_BUTTONS_OFFSET 160
#define NUM_BUTTONS_HEIGHT 160
#define NUM_BUTTONS_X 5
#define NUM_BUTTONS_Y 4

#define FUNC_BUTTONS_OFFSET 48
#define FUNC_BUTTONS_HEIGHT 112
#define FUNC_BUTTONS_X 6
#define FUNC_BUTTONS_Y 4

char shift=0;
char alpha=0;
char store=0;
char hyper=0;

#define INPUT_BUFFER_LEN 128

//int buff_offset; // Offset on the screen
int buff_cursor; // Cursor offset
char cursor_visible;
unsigned char buff[INPUT_BUFFER_LEN]; // Input buffer

#define NUMBER_MODE_ORDINARY 1
#define NUMBER_MODE_SIMPLE_2 2 // d/c
#define NUMBER_MODE_SIMPLE_3 3 // a b/c
#define NUMBER_MODE_DEGREE 3

#define NUMBER_PRECISION 9
#define NUMBER_ITERATIONS (NUMBER_PRECISION)
#define NUMBER_ITERATIONS_SIN 10
#define NUMBER_ITERATIONS_EXP (NUMBER_ITERATIONS*2)
#define NUMBER_ITERATIONS_LN (NUMBER_ITERATIONS*4)

#define EVAL_NUMBER_BUFFER_LEN 10
#define EVAL_OPERATION_BUFFER_LEN 10

// Digit type
typedef unsigned char digit_t;
// Multiplication digit type (digit_t^2)
typedef signed int mult_digit_t;
// Exponent type
typedef signed int exponent_t;
// Sign type
typedef signed char sign_t;
// Counter cycles for digits, should support up to NUMBER_PRECISION*2
typedef signed char digit_counter_t;
// Counter for NUMBER_ITERATIONS
typedef signed char counter_interations_t;

struct number {
  digit_t digits[NUMBER_PRECISION]; // digits, each char is two decimal digits
  exponent_t exponent; // exponent
  sign_t sign;
} a,b,c,d,e,f,x,y,m,ans,screen;

// Choice (screen button code) to buffer value
const unsigned char PROGMEM choice_to_buff[]={
                                  // Number keys 0-19
                                  '7',  '8',  '9',  0,    0,
                                  '4',  '5',  '6',  'x',  '/',
                                  '1',  '2',  '3',  '+',  '-',
                                  '0',  '.',  'E',  165,  0,
                                  // Func keys are 20-43
                                  128,  129,  0,    0,    132,  133,
                                  135,  136,  137,  '^',  139,  141,
                                  '-',  166,  0,    143,  144,  145,
                                  0,    0,    '(',  ')',  ',',  167,
                                  // Func keys with shift 44-67
                                  '!',  130,  0,    0,    131,  134,
                                  135,  0,    0,    138,  140,  142,
                                  0,    0,    0,    149,  150,  151,
                                  0,    0,    0,    0,    0,    168,
                                  // Func keys with alpha 68-91
                                  0,    0,    0,    0,    0,    0,
                                  0,    0,    0,    0,    164,  'e',
                                  'A',  'B',  'C',  'D',  'E',  'F',
                                  0,    0,    0,    'X',  'Y',  'M',
                                  // Func keys with store
                                  0,    0,    0,    0,    0,    0,
                                  0,    0,    0,    0,    0,    0,
                                  155,  156,  157,  158,  159,  160,
                                  0,    0,    0,    161,  162,  163,
                                  // Func keys with hyper
                                  128,  129,  0,    0,    132,  133,
                                  135,  136,  137,  '^',  139,  141,
                                  '-',  166,  0,    146,  147,  148,
                                  0,    0,    '(',  ')',  ',',  167,
                                  // Func keys with shift+hyper
                                  '!',  130,  0,    0,    131,  134,
                                  135,  0,    0,    138,  140,  142,
                                  0,    0,    0,    152,  153,  154,
                                  0,    0,    0,    0,    0,    168
                                  };


#define BUTTON_SHIFT_CODE ((NUM_BUTTONS_X*NUM_BUTTONS_Y)+2)
#define BUTTON_ALPHA_CODE ((NUM_BUTTONS_X*NUM_BUTTONS_Y)+3)
#define BUTTON_HYPER_CODE ((NUM_BUTTONS_X*NUM_BUTTONS_Y)+14)
#define BUTTON_STORE_CODE ((NUM_BUTTONS_X*NUM_BUTTONS_Y)+18)


void setup(void) {
  //Serial.begin(9600);
  tft.reset();
  
  //Once again, need to hard code the code below
  uint16_t identifier = 0x9341;

  tft.begin(identifier);
  tft.setRotation(2);
  tft.fillScreen(BLACK);
  
  // Screen calibration
  ts_maxx=128;
  ts_minx=921;
  ts_maxy=898;
  ts_miny=72;

  /*char tmp[40];
  number_init_from_string(&a,"2.3025850929940456");
  number_init_from_string(&b,"2.3025850929940457");
  number_to_string(a,tmp);
  tft.setTextColor(WHITE,BLACK);
  tft.setCursor(0,CHAR_HEIGHT*5);
  tft.println(tmp);
  //Serial.println(tmp);
  tft.setCursor(0,CHAR_HEIGHT*7);
  number_to_string(b,tmp);
  tft.println(tmp);
  //Serial.println(tmp);
  
  // log10 = ln(argument)/ln(10)
  number_div(&ans,a,b);

  tft.setCursor(0,CHAR_HEIGHT*9);
  number_to_string(ans,tmp);
  tft.println(tmp);
  while(1) delay(1000);*/
  calc_ac();
}

void loop() {
  int choice;
  int choice_shifted;
  unsigned char operation;
  int i;
  
  choice=screen_get_touch();
  choice_shifted=choice;
  operation=0;
  
  if(choice>=(NUM_BUTTONS_X*NUM_BUTTONS_Y)) {
    // Shift
    if(shift && !alpha) choice_shifted+=(FUNC_BUTTONS_X*FUNC_BUTTONS_Y);
    // Alpha
    else if(!shift && alpha) choice_shifted+=(FUNC_BUTTONS_X*FUNC_BUTTONS_Y)*2;
    // Store
    else if(store) choice_shifted+=(FUNC_BUTTONS_X*FUNC_BUTTONS_Y)*3;
    // Hyper
    else if(hyper && !shift) choice_shifted+=(FUNC_BUTTONS_X*FUNC_BUTTONS_Y)*4;
    // Shift + hyper
    else if(hyper && shift) choice_shifted+=(FUNC_BUTTONS_X*FUNC_BUTTONS_Y)*5;
  }
  
  if(choice>=0) {
    operation=pgm_read_byte(choice_to_buff+choice_shifted);
    if(operation>0) {
        buff[buff_cursor++]=operation;
    }
  }
  
  switch(choice) {
    case 3: // Del
      for(i=buff_cursor;i!=INPUT_BUFFER_LEN;i++) {
        if(i==0) buff[0]=buff[1];
        else buff[i-1]=buff[i];
      }
      if(buff_cursor>0) buff_cursor--;
      break;
    case 4: // AC
      calc_ac();
      break;
    case BUTTON_SHIFT_CODE: // Shift
      if(shift) shift=0; else shift=1;
      alpha=store=0;
      func_redraw();
      break;
    case BUTTON_ALPHA_CODE: // Alpha
      if(alpha) alpha=0; else alpha=1;
      shift=hyper=store=0;
      func_redraw();
      break;
    case BUTTON_HYPER_CODE: // hyper
      if(!alpha && !store) {
        hyper=1;
        alpha=store=0;
      }
      func_redraw();
      break;
    case BUTTON_STORE_CODE: // STO
      if(store) store=0; else store=1;
      alpha=shift=hyper=0;
      func_redraw();
      break;
  }

  // Update screen
  if(choice!=-1) screen_input_update();
  
  // Run evaluation when =, M+, M-, store
  if(choice==19 || (operation>=155 && operation<=163) || operation==167 || operation==168) {
    calc_eval();
  }
}

#define BUTTON_DEL_COLOR RED
#define BUTTON_DEL_X 3
#define BUTTON_DEL_Y 0
#define BUTTON_AC_COLOR RED
#define BUTTON_AC_X 4
#define BUTTON_AC_Y 0

void screen_redraw() {
  int x,y;
  
  char *num_buttons[]={"7","8","9","Del","AC","4","5","6","x","/","1","2","3","+","-","0",".","EXP","Ans","="};
  
  int bg_color;
  int color;
  
  for(y=0;y!=NUM_BUTTONS_Y;y++) {
    for(x=0;x!=NUM_BUTTONS_X;x++) {
      if(x==BUTTON_DEL_X && y==BUTTON_DEL_Y) bg_color=BUTTON_DEL_COLOR;
      else if(x==BUTTON_AC_X && y==BUTTON_AC_Y) bg_color=BUTTON_AC_COLOR;
      else bg_color=GRAY;
      button(x*SCREEN_WIDTH/NUM_BUTTONS_X,NUM_BUTTONS_OFFSET+y*NUM_BUTTONS_HEIGHT/NUM_BUTTONS_Y,SCREEN_WIDTH/NUM_BUTTONS_X,NUM_BUTTONS_HEIGHT/NUM_BUTTONS_Y,BLACK,bg_color,WHITE,2,num_buttons[x+y*NUM_BUTTONS_X]);
    }
  }
  func_redraw();
}

void func_redraw() {
  int i,j;
  
  char *func_buttons[]=  {"x^-1", "nCr", "Shift", "Alpha", "Pol(", "x^3","a b/c", "\xFA",  "x\xFC", "^",     "log",  "ln",  "(-)",     "\xF7",    "hyp",     "sin",     "cos",     "tan",      "STO", "ENG", "(",  ")",       ",",       "M+"    };
  char *shift_buttons[]= {"x!",   "nPr", "Shift", "Alpha", "Rec(", "root3", "d/c"  , "",      "",      "x\xFA", "10^x", "e^x", "",        "",        "hyp",     "asin",    "acos",    "atan",     "STO", "<-",  "",   "",        "",        "M-"    };
  char *alpha_buttons[]= {"",     "",    "Shift", "Alpha", "",     "",      "",      "",      "",      "",      "\xE2", "e",   "A",       "B",       "C",       "D",       "E",       "F",        "STO",    "",    "",   "X",       "Y",       "M"     };
  char *store_buttons[]= {"",     "",    "Shift", "Alpha", "",     "",      "",      "",      "",      "",      "",     "",    "\x1A\x41","\x1A\x42","\x1A\x43","\x1A\x44","\x1A\x45","\x1A\x46", "STO", "",    "",   "\x1A\x58","\x1A\x59","\x1A\x4d" };
  char *hyper_buttons[]= {"",     "",    "Shift", "Alpha", "",     "",      "",      "",      "",      "",      "",     "",    "",        "",        "hyp",     "sinh",    "cosh",    "tanh",     "STO",    "",    "",   "",        "",        ""      };
  char *shyper_buttons[]={"",     "",    "Shift", "Alpha", "",     "",      "",      "",      "",      "",      "",     "",    "",        "",        "hyp",     "asinh",   "acosh",   "atanh",    "STO",    "",    "",   "",        "",        ""      };
  
  int bg_color;
  int color;
  for(j=0;j!=FUNC_BUTTONS_Y;j++) {
    for(i=0;i!=FUNC_BUTTONS_X;i++) {
      if(j==3 && i==0 && store) {
        bg_color=WHITE;
        color=DARKBLUE;
      } else if(j==0 && i==2) {
        if(shift) {
          bg_color=YELLOW;
          color=DARKBLUE;
        } else {
          bg_color=DARKBLUE;
          color=YELLOW;
        }
      } else if(j==0 && i==3) {
        if(alpha) {
          bg_color=RED;
          color=DARKBLUE;
        } else {
          bg_color=DARKBLUE;
          color=RED;
        }
      } else if(j==2 && i==2 && hyper) {
        bg_color=WHITE;
        color=DARKBLUE;
      } else {
        color=WHITE;
        bg_color=DARKBLUE;
      }
      if(store) {
        button(i*SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_OFFSET+j*FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,BLACK,bg_color,color,1,store_buttons[i+j*FUNC_BUTTONS_X]);
      } else if(shift) {
        if(color==WHITE) { color=YELLOW; bg_color=DARKBLUE; }
        if(hyper) button(i*SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_OFFSET+j*FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,BLACK,bg_color,color,1,shyper_buttons[i+j*FUNC_BUTTONS_X]);
        else button(i*SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_OFFSET+j*FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,BLACK,bg_color,color,1,shift_buttons[i+j*FUNC_BUTTONS_X]);
      } else if(alpha) {
        if(color==WHITE) color=RED;
        button(i*SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_OFFSET+j*FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,BLACK,bg_color,color,1,alpha_buttons[i+j*FUNC_BUTTONS_X]);
      } else if(hyper) {
        button(i*SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_OFFSET+j*FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,BLACK,bg_color,color,1,hyper_buttons[i+j*FUNC_BUTTONS_X]);
      } else {
        button(i*SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_OFFSET+j*FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,SCREEN_WIDTH/FUNC_BUTTONS_X,FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y,BLACK,bg_color,color,1,func_buttons[i+j*FUNC_BUTTONS_X]);
      }
    }
  }

}

void button(int x, int y, int w, int h, int fg_color, int bg_color, int font_color, int font_size, char *text) {
  tft.fillRect(x,y,w,h,bg_color);
  tft.drawRect(x,y,w,h,fg_color);
  tft.setCursor(x+w/2-CHAR_WIDTH*strlen(text)*font_size/2,y+h/2-CHAR_HEIGHT*font_size/2);
  tft.setTextColor(font_color);
  tft.setTextSize(font_size);
  tft.print(text);
}

void screen_output_update_message(char *str) {
  int i;
  tft.setCursor(0,OUTPUT_STRING_OFFSET);
  tft.setTextColor(WHITE,BLACK);
  tft.setTextSize(OUTPUT_STRING_CHAR_SIZE);
  if(strlen(str)<SCREEN_WIDTH/(CHAR_WIDTH*OUTPUT_STRING_CHAR_SIZE)) {
    tft.print(" ");
  }
  tft.print(str);
  if((strlen(str)+1)<SCREEN_WIDTH/(CHAR_WIDTH*OUTPUT_STRING_CHAR_SIZE)) {
    for(i=0;i!=SCREEN_WIDTH/(CHAR_WIDTH*OUTPUT_STRING_CHAR_SIZE)-strlen(str)-1;i++) tft.print(" ");
  }
}

void screen_output_update() {
  char int_buff[30];
  int exp_value;
  char i;
  long int_value;
  
  tft.setCursor(0,OUTPUT_STRING_OFFSET);
  tft.setTextColor(WHITE,BLACK);
  tft.setTextSize(OUTPUT_STRING_CHAR_SIZE);

  number_to_string(screen,int_buff);
  
  if(strlen(int_buff)<SCREEN_WIDTH/(CHAR_WIDTH*OUTPUT_STRING_CHAR_SIZE)) {
    for(i=0;i<=SCREEN_WIDTH/(CHAR_WIDTH*OUTPUT_STRING_CHAR_SIZE)-strlen(int_buff)-1;i++) {
      tft.print(" ");
    }
  }
  tft.print(int_buff);
}

#define VISIBLE_BUFF_EXTRA_RIGHT 10
#define VISIBLE_BUFF_SHIFT_LIMIT 5

void screen_input_update() {
  unsigned char visible_buff[SCREEN_WIDTH/(CHAR_WIDTH*INPUT_STRING_CHAR_SIZE)+VISIBLE_BUFF_EXTRA_RIGHT];
  char visible_buff_pos;
  char visible_cursor_pos;
  char symbol_len;
  int buff_offset;
  
  char i,j;
  char cursor_flag;
  char *func_names[]={"^-1", // 128
                      "C", // 129 nCr
                      "P", // 130 nPr
                      "Rec(", // 131 Rec(
                      "Pol(", // 132 Pol(
                      "^3", // 133
                      "3\xFA", // 134 3root
                      "\xD8", // 135 simple fraction
                      "\xFA", // 136 sqrt
                      "^2", // 137 ^2
                      "x\xFA", // 138 xroot
                      "log ", // 139 log
                      "10^", // 140
                      "ln ", // 141
                      "e^", // 142
                      "sin ", // 143
                      "cos ", // 144
                      "tan ", // 145
                      "sinh ", // 146
                      "cosh ", // 147
                      "tanh ", // 148
                      "asin ", // 149
                      "acos ", // 150
                      "atan ", // 151
                      "asinh ", // 152
                      "acosh ", // 153
                      "atanh ", // 154
                      "\x1A\x41", // 155 ->A
                      "\x1A\x42", // 156 ->B
                      "\x1A\x43", // 157 ->C
                      "\x1A\x44", // 158 ->D
                      "\x1A\x45", // 159 ->E
                      "\x1A\x46", // 160 ->F
                      "\x1A\x58", // 161 ->X
                      "\x1A\x59", // 162 ->Y
                      "\x1A\x4D", // 163 ->M
                      "\xE2", // 164 pi
                      "Ans", // 165 Ans
                      "\xF7", // 166 degree
                      "M+", // 167 M+
                      "M-" // 168 M-
                      };
  unsigned char symbol;
  
  visible_buff_pos=0;
  visible_cursor_pos=0;

  memset(visible_buff,0,SCREEN_WIDTH/(CHAR_WIDTH*INPUT_STRING_CHAR_SIZE)+VISIBLE_BUFF_EXTRA_RIGHT);

  buff_offset=0;
  for(i=0;i!=SCREEN_WIDTH/(CHAR_WIDTH*INPUT_STRING_CHAR_SIZE);i++) {
    if(buff_offset==buff_cursor) {
      visible_cursor_pos=visible_buff_pos;
    }
    symbol=buff[buff_offset];
    if(symbol==0) break;
    else if(symbol<128) {
      visible_buff[visible_buff_pos]=symbol;
      symbol_len=1;
    } else {
      strcpy(visible_buff+visible_buff_pos,func_names[symbol-128]);
      symbol_len=strlen(func_names[symbol-128]);
    }
    visible_buff_pos+=symbol_len;
    buff_offset++;
    
    // If visible buff moved too much right, shift it left
    if(visible_buff_pos>=(SCREEN_WIDTH/(CHAR_WIDTH*INPUT_STRING_CHAR_SIZE)-VISIBLE_BUFF_SHIFT_LIMIT)) {
      for(j=symbol_len;j!=SCREEN_WIDTH/(CHAR_WIDTH*INPUT_STRING_CHAR_SIZE)+VISIBLE_BUFF_EXTRA_RIGHT;j++) {
        visible_buff[j-symbol_len]=visible_buff[j];
      }
      visible_buff_pos-=symbol_len;
    }
  }
  
  tft.setCursor(0,INPUT_STRING_OFFSET);
  tft.setTextColor(WHITE,GRAY);
  tft.setTextSize(INPUT_STRING_CHAR_SIZE);
  
  for(i=0;i!=SCREEN_WIDTH/(CHAR_WIDTH*INPUT_STRING_CHAR_SIZE);i++) {
    //if(i==visible_cursor_pos && (millis()/500)%2) cursor_flag=1;
    if(i==visible_cursor_pos) cursor_flag=1;
    else cursor_flag=0;

    if(cursor_flag) {
      tft.print('_');
    } else {
      if(i<strlen(visible_buff)) {
        tft.print((char)visible_buff[i]);
      } else {
        tft.print(" ");
      }
    }
  }
}

// Button codes offset
#define NUM_BUTTON_TOUCH_OFFSET 0
#define FUNC_BUTTON_TOUCH_OFFSET (NUM_BUTTONS_X*NUM_BUTTONS_Y)
// Threshold before touch
#define NO_TOUCH_THRESHOLD_TOUCH 50
// Threshold after touch
#define NO_TOUCH_THRESHOLD_UNTOUCH 100

int screen_get_touch() {
  long prev_touch_millis;
  int x,y;
  TSPoint p;
  digitalWrite(13, HIGH);
  p = ts.getPoint();
  digitalWrite(13, LOW);

  if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    delay(NO_TOUCH_THRESHOLD_TOUCH);
    digitalWrite(13, HIGH);
    p = ts.getPoint();
    digitalWrite(13, LOW);
  }
  
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if(p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    
    
    x = map(p.x, ts_minx, ts_maxx, 0, tft.width() );
    y = map(p.y, ts_miny, ts_maxy, 0, tft.height() );
    
    do {
      digitalWrite(13, HIGH);
      p = ts.getPoint();
      digitalWrite(13, LOW);
      if(p.z > MINPRESSURE && p.z < MAXPRESSURE) prev_touch_millis=millis();
    } while((millis()-prev_touch_millis)<NO_TOUCH_THRESHOLD_UNTOUCH);
    
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    
    if(y>NUM_BUTTONS_OFFSET && y<NUM_BUTTONS_OFFSET+NUM_BUTTONS_HEIGHT) {
      return NUM_BUTTON_TOUCH_OFFSET+x/(SCREEN_WIDTH/NUM_BUTTONS_X)+(y-NUM_BUTTONS_OFFSET)/(NUM_BUTTONS_HEIGHT/NUM_BUTTONS_Y)*NUM_BUTTONS_X;
    } else if (y>FUNC_BUTTONS_OFFSET && y<FUNC_BUTTONS_OFFSET+FUNC_BUTTONS_HEIGHT) {
      return FUNC_BUTTON_TOUCH_OFFSET+x/(SCREEN_WIDTH/FUNC_BUTTONS_X)+(y-FUNC_BUTTONS_OFFSET)/(FUNC_BUTTONS_HEIGHT/FUNC_BUTTONS_Y)*FUNC_BUTTONS_X;
    }
  } else {
    return -1;
  }
}

// Reset calc
void calc_ac() {
  // Clear modes
  shift=0;
  alpha=0;
  store=0;
  hyper=0;

  // Clear input buffer
  memset(buff,0,INPUT_BUFFER_LEN);
  
  // Clear cursor pos
  buff_cursor=0;
  
  // Clear screen variable
  number_init(&screen);
  
  // Redraw screen
  screen_redraw();
  screen_output_update();
  screen_input_update();
}

// Evaluate operation
// Returns number_stack elements removed
char calc_eval_operation(struct number *number_stack_1,struct number *number_stack_2,unsigned char operation) {
  char stack_elements_removed;
  
  stack_elements_removed=0;
  
  switch(operation) {
    case '+':
      stack_elements_removed=1;
      number_add(number_stack_2,*number_stack_2,*number_stack_1);
      break;
    case '-':
      stack_elements_removed=1;
      number_sub(number_stack_2,*number_stack_2,*number_stack_1);
      break;
    case 'x':
      stack_elements_removed=1;
      number_mult(number_stack_2,*number_stack_2,*number_stack_1);
      break;
    case '/':
      stack_elements_removed=1;
      number_div(number_stack_2,*number_stack_2,*number_stack_1);
      break;
    case 139: // log
      number_log10(number_stack_1,*number_stack_1);
      break;
    case 141: // ln
      number_ln(number_stack_1,*number_stack_1);
      break;
    case 142: // e^
      number_exp(number_stack_1,*number_stack_1);
      break;
    case 143: // sin
      number_sin(number_stack_1,*number_stack_1);
      break;
    case 144: // cos
      number_cos(number_stack_1,*number_stack_1);
      break;
    case 155: // ->A
      number_copy(&a,number_stack_1);
      break;
    case 156: // ->B
      number_copy(&b,number_stack_1);
      break;
    case 157: // ->C
      number_copy(&c,number_stack_1);
      break;
    case 158: // ->D
      number_copy(&d,number_stack_1);
      break;
    case 159: // ->E
      number_copy(&e,number_stack_1);
      break;
    case 160: // ->F
      number_copy(&f,number_stack_1);
      break;
    case 161: // ->X
      number_copy(&x,number_stack_1);
      break;
    case 162: // ->Y
      number_copy(&y,number_stack_1);
      break;
    case 163: // ->M
      number_copy(&m,number_stack_1);
      break;
    case 167: // M+
      number_add(&m,m,*number_stack_1);
      break;
    case 168: // M-
      number_sub(&m,m,*number_stack_1);
      break;
  }
  return stack_elements_removed;
}

// Get operation priority
int get_operation_priority(unsigned char operation) {
  if(operation>=155 && operation<=163) return 0; // Store to A,B,C,D,E,F,X,Y,M
  if(operation>=167 && operation<=168) return 0; // M+ and M-
  if(operation=='+' || operation=='-') return 1; // add and sub
  if(operation=='x' || operation=='/') return 2; // mult and div
  if(operation=='^' || operation==138) return 3; // pow and xroot
  if(operation==129 || operation==130) return 4; // nCr and nPr
  return 10; // Unary operations
}

/*
void show_stack(char number_stack_index,struct number *number_stack,char operations_stack_index,unsigned char *operations_stack) {
  int i;
  char tmp[20];
  tft.setTextColor(GREEN,BLACK);
  tft.setTextSize(1);
  for(i=0;i!=number_stack_index;i++) {
    number_to_string(number_stack[i],tmp);
    tft.setCursor(CHAR_WIDTH*5,CHAR_HEIGHT*(i+10));
    tft.print(tmp);
    tft.print("  ");
  }
  tft.setCursor(CHAR_WIDTH*5,CHAR_HEIGHT*(number_stack_index+10));
  tft.print("^^^^");
  
  for(i=0;i!=operations_stack_index;i++) {
    tft.setCursor(0,CHAR_HEIGHT*(i+10));
    tft.print((char)operations_stack[i]);
    tft.print(" ");
  }
  tft.setCursor(0,CHAR_HEIGHT*(operations_stack_index+10));
  tft.print("^^^^");
}
*/

// Evaluate expression
void calc_eval() {
  int buff_offset;
  int result;
  struct number number_stack[EVAL_NUMBER_BUFFER_LEN];
  unsigned char operations_stack[EVAL_OPERATION_BUFFER_LEN];
  char operations_stack_index;
  char number_stack_index;
  char error_flag;
  char bracket_flag;
  
  operations_stack_index=0;
  number_stack_index=0;
  buff_offset=0;
  error_flag=0;
  bracket_flag=0;
  
  screen_output_update_message("Calculating...");
  
  while(buff_offset<strlen(buff) && error_flag==0) {
    // If number then read number
    if((buff[buff_offset]>='0' && buff[buff_offset]<='9') || buff[buff_offset]=='.' || buff[buff_offset]=='E') {
      if(number_stack_index==EVAL_NUMBER_BUFFER_LEN) {
        //                            1234567890123456789
        screen_output_update_message("Num stack overflow");
        error_flag=1;
      } else {
        result=number_init_from_string(&number_stack[number_stack_index],buff+buff_offset);
      }
      if(result>0) {
        number_stack_index++;
        buff_offset+=result;
      } else if(result<0) {
        screen_output_update_message("Incorrect number");
        error_flag=1;
        break;
      }
    // Else read operation or special number
    } else {
      switch(buff[buff_offset]) {
        case 'A': // A
          number_copy(&number_stack[number_stack_index],&a);
          number_stack_index++;
          break;
        case 'B': // B
          number_copy(&number_stack[number_stack_index],&b);
          number_stack_index++;
          break;
        case 'C': // C
          number_copy(&number_stack[number_stack_index],&c);
          number_stack_index++;
          break;
        case 'D': // D
          number_copy(&number_stack[number_stack_index],&d);
          number_stack_index++;
          break;
        case 'E': // E
          number_copy(&number_stack[number_stack_index],&e);
          number_stack_index++;
          break;
        case 'F': // F
          number_copy(&number_stack[number_stack_index],&f);
          number_stack_index++;
          break;
        case 'X': // X
          number_copy(&number_stack[number_stack_index],&x);
          number_stack_index++;
          break;
        case 'Y': // Y
          number_copy(&number_stack[number_stack_index],&y);
          number_stack_index++;
          break;
        case 'M': // M
          number_copy(&number_stack[number_stack_index],&m);
          number_stack_index++;
          break;
        case 'e': // e
          number_init_e(&number_stack[number_stack_index]);
          number_stack_index++;
          break;
        case 164: // pi
          number_init_pi(&number_stack[number_stack_index]);
          number_stack_index++;
          break;
        case 165: // Ans
          number_copy(&number_stack[number_stack_index],&ans);
          number_stack_index++;
          break;
        default: // operation
          // If closing bracket
          if(buff[buff_offset]==')') bracket_flag=1;
          // If stack operations exists
          if(operations_stack_index>=1) {
            // Do stack operations until new operation priority is lower (or equal) than stack operations
            while(operations_stack_index>0 && number_stack_index>0 &&
                      ((operations_stack[operations_stack_index-1]!='(' 
                      && get_operation_priority(buff[buff_offset])<=get_operation_priority(operations_stack[operations_stack_index-1])) || bracket_flag!=0)) {
              
              operations_stack_index--;
              // If op is open bracket, bracket_flag=0, thus 
              if(operations_stack[operations_stack_index]=='(') {
                bracket_flag=0;
              }
              if(number_stack_index>=2) {
                result=calc_eval_operation(&number_stack[number_stack_index-1],&number_stack[number_stack_index-2],operations_stack[operations_stack_index]);
                number_stack_index-=result;
              } else if(number_stack_index==1) {
                result=calc_eval_operation(&number_stack[number_stack_index-1],&number_stack[number_stack_index-1],operations_stack[operations_stack_index]);
                number_stack_index-=result;
              } else {
                screen_output_update_message("Incorrect operations");
                error_flag=1;
                break;
              }
            }
          }
          // Add operation to stack
          if(!bracket_flag) {
            if(operations_stack_index==EVAL_OPERATION_BUFFER_LEN) {
              screen_output_update_message("Op stack overflow");
              error_flag=1;
            } else {
              operations_stack[operations_stack_index]=buff[buff_offset];
              operations_stack_index++;
            }
          }
          break;
      }
    buff_offset++;
    }
  //show_stack(number_stack_index,number_stack,operations_stack_index,operations_stack);
  //delay(5000);
  }
  
  while(operations_stack_index>=1 && error_flag==0) {
    //show_stack(number_stack_index,number_stack,operations_stack_index,operations_stack);
    //delay(5000);
    operations_stack_index--;

    if(number_stack_index>=2) {
      result=calc_eval_operation(&number_stack[number_stack_index-1],&number_stack[number_stack_index-2],operations_stack[operations_stack_index]);
    } else if(number_stack_index==1) {
      result=calc_eval_operation(&number_stack[number_stack_index-1],&number_stack[number_stack_index-1],operations_stack[operations_stack_index]);
    } else {
      screen_output_update_message("Incorrect operations");
      break;
    }
    number_stack_index-=result;
  }

  if(number_stack_index==1 && operations_stack_index==0) {
    number_stack_index--;
    number_copy(&ans,&number_stack[number_stack_index]);
    number_copy(&screen,&number_stack[number_stack_index]);
    screen_output_update();
  } else {
    // If error flag is not set, then error message is not shown
    if(error_flag==0) {
      screen_output_update_message("Incorrect operations");
    }
  }
}


// -------------------------------------------------------------------------------
// Number functions
// -------------------------------------------------------------------------------

// Init number with 0
void number_init(struct number *num) {
  digit_counter_t i;
  for(i=0;i!=NUMBER_PRECISION;i++) num->digits[i]=0;
  num->sign=0;
  num->exponent=0;
}

// Init number with 1
void number_init_one(struct number *num) {
  digit_counter_t i;
  num->digits[0]=1;
  for(i=1;i!=NUMBER_PRECISION;i++) num->digits[i]=0;
  num->sign=0;
  num->exponent=0;
}

// Init number with NaN
void number_init_nan(struct number *num) {
  digit_counter_t i;
  for(i=0;i!=NUMBER_PRECISION;i++) num->digits[i]=0;
  num->sign=-1;
  num->exponent=0;
}


// Read string into number, returns offset
// Returns positive offset on success
// Returns zero on zero string
// Returns negative on format error:
// -1 is multiple dots
// -2 too big exponent >=10000
int number_init_from_string(struct number *num,char *str) {
  int i;
  int string_index;
  digit_t digit;
  char dot_flag;
  char exp_sign;
  exponent_t exp_value;
  
  // Init variables
  number_init(num);
  string_index=0;
  dot_flag=0;
  
  // Read sign if present
  if(str[string_index]=='-') {
    num->sign=1;
    string_index++;
  } else if(str[string_index]=='+') {
    num->sign=0;
    string_index++;
  } else if(str[string_index]==0) {
    return 0;
  }
  
  // Read number digits until precision
  for(i=1;i!=NUMBER_PRECISION*2+1;i++) {
    if(str[string_index]=='.') {
      // Only one dot allowed
      if(dot_flag==1) {
        return -1;
      } else {
        dot_flag=1;
      }
      string_index++;
      continue;
    }
    
    // Break if no digits
    if(str[string_index]<'0' || str[string_index]>'9') {
      break;
    }
    digit=str[string_index]-'0';
    string_index++;
    if((i-dot_flag)%2) {
      num->digits[(i-dot_flag)/2]=num->digits[(i-dot_flag)/2]+digit;
    } else {
      num->digits[(i-dot_flag)/2]=num->digits[(i-dot_flag)/2]+digit*10;
    }
    if(!dot_flag && i!=1) num->exponent++;
  }
  
  // Skip next digits until exp or EoL
  while(str[string_index]!='E' && str[string_index]!=0) {
    if((str[string_index]>='0' && str[string_index]<='9') || str[string_index]=='.') {
      string_index++;
    } else {
      return string_index;
    }
  }
  
  // Read exponent
  if(str[string_index]=='E') {
    string_index++;
    
    // Read exponent sign
    if(str[string_index]=='-') {
      exp_sign=1;
      string_index++;
    } else if(str[string_index]=='+') {
      exp_sign=0;
      string_index++;
    } else if(str[string_index]<'0' || str[string_index]>'9') {
      return string_index;
    }
    
    // Read exponent value
    exp_value=0;
    while(str[string_index]>='0' && str[string_index]<='9') {
      if(exp_value<1000) {
        exp_value=exp_value*10+str[string_index]-'0';
        string_index++;
      } else {
        return -2;
      }
    }

    // Apply sign
    if(exp_sign==1) exp_value=-exp_value;

    // Apply exponent
    num->exponent+=exp_value;
  }

  number_normalize(num);
  
  return string_index;
}

// Init number with pi value
void number_init_pi(struct number *num) {
  int i;
  //                           0 1 2 3 4 5 6 7 8
  number_init_from_string(num,"3.1415926535897932");
}

// Init number with 2*pi value
void number_init_2pi(struct number *num) {
  int i;
  //                           0 1 2 3 4 5 6 7 8
  number_init_from_string(num,"6.2831853071795865");
}

// Init number with pi/2 value
void number_init_pi_2(struct number *num) {
  int i;
  //                           0 1 2 3 4 5 6 7 8
  number_init_from_string(num,"1.5707963267948966");
}

// Init number with e value
void number_init_e(struct number *num) {
  int i;
  //                           0 1 2 3 4 5 6 7 8
  number_init_from_string(num,"2.7182818284590452");
}

// Init number with sqrt(e) value
void number_init_sqrt_e(struct number *num) {
  int i;
  //                           0 1 2 3 4 5 6 7 8
  number_init_from_string(num,"1.6487212707001281");
}

// Init number with ln(10) value
void number_init_ln10(struct number *num) {
  int i;
  //                             0 1 2 3 4 5 6 7 8
  //number_init_from_string(num,"2.3025850929940456");
  // Right value
  //                           0 1 2 3 4 5 6 7 8
  number_init_from_string(num,"2.3025850929940457");
}

// Copy number to number
void number_copy(struct number *to,struct number *from) {
  int i;
  for(i=0;i!=NUMBER_PRECISION;i++) to->digits[i]=from->digits[i];
  to->sign = from->sign;
  to->exponent = from->exponent;
}

// Number to string human-readable
void number_to_string(struct number num,char *str) {
  digit_counter_t string_index;
  digit_counter_t i;
  digit_counter_t last_valuable;

  if(number_is_nan(num)) {
    strcpy(str,"NaN");
    return;
  }

  number_round_last_digit(&num);
  
  if(num.exponent<-3 || num.exponent>14) {
    number_to_string_sci(num,str);
    return;
  }
  
  string_index=0;
  if(num.sign==1) {
    str[string_index]='-';
    string_index++;
    str[string_index]=0;
  }

  // Find last valuable digit
  last_valuable=0;
  for(i=1;i!=NUMBER_PRECISION*2;i++) {
    if(i%2) {
      if(((digit_t)(num.digits[i/2])%10)!=0) last_valuable=i;
    } else {
      if(((digit_t)(num.digits[i/2]/10))!=0) last_valuable=i;
    }
  }
  
  if(num.exponent<0) {
    str[string_index]='0';
    string_index++;
    str[string_index]='.';
    string_index++;
    if(num.exponent<-1) {
      for(i=num.exponent+1;i!=0;i++) {
        str[string_index]='0';
        string_index++;
      }
    }
    str[string_index]=0;
  }
  
  // Valuable digits
  for(i=1;i!=NUMBER_PRECISION*2;i++) {
    if(i%2) {
      str[string_index]='0'+num.digits[i/2]%10; // second digit
      string_index++;
    } else {
      str[string_index]='0'+num.digits[i/2]/10; // first digit
      string_index++;
    }
    if(i>=(num.exponent+1) && i>=last_valuable) break;
    if(i==(num.exponent+1)) {
      str[string_index]='.';
      string_index++;
    }
  }
  str[string_index]=0;
}

// Number to string in scientific format
void number_to_string_sci(struct number num,char *str) {
  int i;
  int string_index;
  int last_digit;
  char buff[6];
  
  last_digit=0;
  
  string_index=0;
  
  // Sign
  if(num.sign==1) {
    str[string_index]='-';
    string_index++;
    str[string_index]=0;
  }
  
  // Valuable digits
  for(i=1;i!=NUMBER_PRECISION*2;i++) {
    if(i%2) {
      str[string_index]='0'+num.digits[i/2]%10; // second digit
      string_index++;
      str[string_index]=0;
    } else {
      str[string_index]='0'+num.digits[i/2]/10; // first digit
      string_index++;
      str[string_index]=0;
    }
    if(i==1) {
      str[string_index]='.';
      string_index++;
      str[string_index]=0;
    }
  }

  // Exponent
  sprintf(buff,"E%d",(int)num.exponent);
  strcat(str,buff);
}

// Shift number right
void number_shift_right(struct number *num) {
  int i;
  char digit_shifted;
  
  for(i=NUMBER_PRECISION-1;i>=0;i--) {
    digit_shifted=0;
    digit_shifted=num->digits[i]/10;
    if(i>0) digit_shifted+=(num->digits[i-1]%10)*10;
    num->digits[i]=digit_shifted;
  }
}

// Shift number left
void number_shift_left(struct number *num) {
  int i;
  char digit_shifted;
  
  for(i=0;i!=NUMBER_PRECISION;i++) {
    digit_shifted=0;
    digit_shifted=(num->digits[i]%10)*10;
    if((i+1)<NUMBER_PRECISION) digit_shifted+=(num->digits[i+1]/10);
    num->digits[i]=digit_shifted;
  }
}

// Normalize form
void number_normalize(struct number *num) {
  int i;
  
  // Shift right if needed
  if(num->digits[0]>=10) {
    number_shift_right(num);
    num->exponent++;
  }
  
  // Shift left if needed
  i=NUMBER_PRECISION*2;
  while(num->digits[0]==0) {
    number_shift_left(num);
    num->exponent--;
    i--;
    // Precision exceeded
    if(i==0) {
      num->exponent=0;
      break;
    }
  }
}

// Round last two digits
void number_round_last_digit(struct number *num) {
  digit_counter_t i;

  for(i=NUMBER_PRECISION-1;i>=0;i--) {
    if(i==NUMBER_PRECISION-1) {
      if(num->digits[i]<50) {
        num->digits[i]=0;
        break;
      } else {
        num->digits[i]=0;
      }
    } else {
      if(num->digits[i]<99) {
        num->digits[i]++;
        break;
      } else {
        num->digits[i]=0;
      }
    }
  }
  number_normalize(num);
}

// Add two numbers
void number_add(struct number *result,struct number left_op,struct number right_op) {
  digit_counter_t i;
  exponent_t max_exp;
  char carry;
  mult_digit_t digit;
  char left_less_flag;
  
  // Check for NaN
  if(number_is_nan(left_op) || number_is_nan(right_op)) {
    number_init_nan(result);
    return;
  } else {
    // Init result
    number_init(result);
  }
  
  // If left op is zero
  if(number_is_zero(left_op)) {
    number_copy(result,&right_op);
    return;
  }
  
  // If right op is zero
  if(number_is_zero(right_op)) {
    number_copy(result,&left_op);
    return;
  }

  // Result sign is 
  if(left_op.sign==right_op.sign) {
    result->sign = left_op.sign;
  } else {
    if(left_op.sign==0) {
      right_op.sign=0;
      left_less_flag=number_le(left_op,right_op);
      if(left_less_flag) result->sign=1;
      else result->sign=0;
      right_op.sign=1;
    } else {
      left_op.sign=0;
      left_less_flag=number_le(left_op,right_op);
      if(left_less_flag) result->sign=0;
      else result->sign=1;
      left_op.sign=1;
    }
  }
  
  // Shifting numbers until exponents are matched
  max_exp=max(left_op.exponent,right_op.exponent);
  result->exponent=max_exp;
  
  while(left_op.exponent!=right_op.exponent) {
    if(left_op.exponent!=max_exp) {
      number_shift_right(&left_op);
      left_op.exponent++;
    }
    if(right_op.exponent!=max_exp) {
      number_shift_right(&right_op);
      right_op.exponent++;
    }
  }

  // Adding
  carry=0;
  for(i=NUMBER_PRECISION-1;i>=0;i--) {
    if(left_op.sign==right_op.sign) {
      digit=(int)left_op.digits[i]+(int)right_op.digits[i]+(int)carry;
      result->digits[i] = digit%100;
      carry = digit/100;
    } else {
      if(left_less_flag) digit=(int)right_op.digits[i]-(int)left_op.digits[i]-(int)carry;
      else digit=(int)left_op.digits[i]-(int)right_op.digits[i]-(int)carry;
      if(digit<0) {
        carry=1;
        digit+=100;
      } else {
        carry=0;
      }
      result->digits[i] = digit%100;
    }
  }

  if(carry!=0) {
    if(result->sign==0) result->sign=1;
    else result->sign=0;
  }

  // Normalize
  number_normalize(result);
}

// Substraction
void number_sub(struct number *result,struct number left_op,struct number right_op) {
  number_inverse_sign(&right_op);
  number_add(result,left_op,right_op);
}


// Multiple number by digit
void number_mult_single(struct number *result,struct number left_op,digit_t mult_digit) {
  char carry;
  mult_digit_t digit;
  digit_counter_t i;

  // Init result
  number_init(result);

  // If multiply by 0, then we done
  if(mult_digit==0) return;

  carry=0;
  for(i=NUMBER_PRECISION-1;i>=0;i--) {
    digit=(int)carry+((int)left_op.digits[i])*((int)mult_digit);
    result->digits[i] = digit%100;
    carry = digit/100;
  }

  // If carry != 0
  if(carry!=0) {
    number_shift_right(result);
    number_shift_right(result);
    result->digits[0]=carry;
    result->exponent+=2;
  }

  // Normalize
  number_normalize(result);
}

// Multiply two numbers
void number_mult(struct number *result,struct number left_op,struct number right_op) {
  struct number tmp;
  digit_counter_t i;
  digit_t digit;
  char tmp_buff[40];
  
  // Check for NaN
  if(number_is_nan(left_op) || number_is_nan(right_op)) {
    number_init_nan(result);
    return;
  }
  
  // Init result
  number_init(result);
  
  // If left op or right op is zero, then result is zero
  if(number_is_zero(left_op) || number_is_zero(right_op)) {
    return;
  }
  
  // If left op is one, result is right_op
  if(number_is_one(left_op)) {
    number_copy(result,&right_op);
    return;
  }
  
  // If right op is one, result is left_op
  if(number_is_one(right_op)) {
    number_copy(result,&left_op);
    return;
  }

  for(i=NUMBER_PRECISION-1;i>=0;i--) {

    digit=right_op.digits[i];

    number_mult_single(&tmp,left_op,digit);
    tmp.exponent-=i*2;
    
    number_add(result,*result,tmp);
  }

  // Calculating exponent
  result->exponent+=right_op.exponent+left_op.exponent;

  if(left_op.sign==right_op.sign) result->sign=0;
  else result->sign=1;

  // Normalize
  number_normalize(result);
}

// Power (only integer)
void number_pow_digit(struct number *result,struct number left_op,digit_t digit) {
  digit_t i;
  
  // Check for NaN
  if(number_is_nan(left_op)) {
    number_init_nan(result);
    return;
  }
  
  number_init_one(result);
  if(digit==0) return;
  
  for(i=0;i!=digit;i++) {
    number_mult(result,*result,left_op);
  }
}

// Factorial
void number_factor(struct number *result,digit_t digit) {
  struct number number; // Ordinary number
  struct number one; // 1
  digit_counter_t i;

  // Check for NaN
  if(number_is_nan(*result)) {
    number_init_nan(result);
    return;
  }
  
  number_init_one(result);
  if(digit==0 || digit==1) return;

  number_init_one(&number);
  number_init_one(&one);
  
  for(i=1;i!=digit;i++) {
    //Serial.print("+");
    number_add(&number,number,one);
    //Serial.print("*");
    number_mult(result,*result,number);
  }
  //Serial.println("done");
}

// Compare two numbers, less or equal
char number_le(struct number left_op,struct number right_op) {
  digit_counter_t i;

  // Check for NaN
  if(number_is_nan(left_op) || number_is_nan(right_op)) {
    return -1;
  }
  
  // Comparing positive and negative
  if(left_op.sign==1 && right_op.sign==0) return 1;
  else if(left_op.sign==0 && right_op.sign==1) return 0;

  // Positive numbers
  if(left_op.sign==0) {
    // Zero is lower of positives
    if(number_is_zero(left_op)) return 1;
    else if(number_is_zero(right_op)) return 0;
    else if(left_op.exponent<right_op.exponent) return 1;
    else if(left_op.exponent>right_op.exponent) return 0;
  // Negative numbers
  } else {
    // Negative zero is bigger of all negatives
    if(number_is_zero(left_op)) return 0;
    else if(number_is_zero(right_op)) return 1;
    else if(left_op.exponent<right_op.exponent) return 0;
    else if(left_op.exponent>right_op.exponent) return 1;
  }
  
  for(i=0;i!=NUMBER_PRECISION;i++) {
    // Positive numbers
    if(left_op.sign==0) {
      if(left_op.digits[i]<right_op.digits[i]) return 1;
      else if(left_op.digits[i]>right_op.digits[i]) return 0;
    } else {
      // Negative numbers
      if(left_op.digits[i]<right_op.digits[i]) return 0;
      else if(left_op.digits[i]>right_op.digits[i]) return 1;
    }
  }

  // Equial
  return 1;
}

// Compare to zero
char number_is_zero(struct number num) {
  digit_counter_t i;

  // Check for NaN
  if(number_is_nan(num)) {
    return -1;
  }
  
  // If any digit is not zero, then all number is not zero
  for(i=0;i!=NUMBER_PRECISION;i++) {
    if(num.digits[i]!=0) return 0;
  }
  return 1;
}

// Compare to 1
char number_is_one(struct number num) {
  digit_counter_t i;

  // Check for NaN
  if(number_is_nan(num)) {
    return -1;
  }
  
  number_normalize(&num);
  if(num.exponent!=0) return 0;
  if(num.digits[0]!=1) return 0;
  for(i=1;i!=NUMBER_PRECISION;i++) {
    if(num.digits[i]!=0) return 0;
  }
  return 1;
}

// Inverse sign
void number_inverse_sign(struct number *num) {
  if(num->sign==0) {
    num->sign=1;
  } else if(num->sign==1) {
    num->sign=0;
  }
}

// Check for NaN flag
char number_is_nan(struct number num) {
  if(num.sign==-1) {
    return 1;
  } else {
    return 0;
  }
}

// Divide two numbers
void number_div(struct number *result,struct number left_op,struct number right_op) {
  struct number tmp;
  digit_counter_t i;
  int max_exp;
  char carry;
  int digit;
  //char buff[80];
  
  // Check for NaN
  if(number_is_nan(left_op) || number_is_nan(right_op) || number_is_zero(right_op)) {
    number_init_nan(result);
    return;
  }
  
  // Init result
  number_init(result);

  // If left op is zero, then result is zero
  if(number_is_zero(left_op)) {
    return;
  }
  
  // If right op is one, result is left_op
  if(number_is_one(right_op)) {
    number_copy(result,&left_op);
    return;
  }
  
  // Calculate result exponent
  result->exponent = left_op.exponent-right_op.exponent;
  
  left_op.exponent=0;
  right_op.exponent=0;

  // Calculate result sign
  if(left_op.sign==right_op.sign) result->sign=0;
  else result->sign=1;

  left_op.sign=0;
  right_op.sign=0;
  
  for(i=0;i!=NUMBER_PRECISION;i++) {
    digit=0;
    number_normalize(&left_op);
    while(number_le(right_op,left_op)) {
      digit++;
      /*if(digit==101) {
        tft.setCursor(0,0);
        tft.print("DIV OVERFLOW");
        delay(10000);
      }*/
      number_sub(&left_op,left_op,right_op);
      /*number_to_string(left_op,buff);
      tft.setCursor(0,0);
      tft.print(i);
      tft.print(" ");
      tft.print(digit);
      tft.print(" ");
      tft.println(buff);
      tft.print(" ");
      delay(100);*/
    }
    if(digit>=100) {
      digit=digit%100;
      if(i>0) result->digits[i-1]++;
    }
    result->digits[i]=digit;
    left_op.exponent+=2;
    //delay(10000);
  }

  number_normalize(result);
}

// Integer part
void number_integer(struct number *result,struct number argument) {
  digit_counter_t i;
  
  number_copy(result,&argument);
  
  for(i=1;i!=NUMBER_PRECISION*2;i++) {
    if(i%2) {
      if((i-1)>argument.exponent) {
        result->digits[i/2]-=result->digits[i/2]%10;
      }
    } else {
      if((i-1)>argument.exponent) {
        result->digits[i/2]=0;
      }
    }
  }

  number_normalize(result);
}

// Sinus of any argument
void number_sin(struct number *result,struct number argument) {
  struct number upper;
  struct number lower;
  counter_interations_t i;
  char inverse_result_sign;

  // Check for NaN
  if(number_is_nan(argument)) {
    number_init_nan(result);
    return;
  }
  
  inverse_result_sign=0;
  number_init(result);

  // Removing period from argument
  // Upper is 2*pi
  number_init_2pi(&upper);
  
  // Divide argument by 2*pi
  number_div(&lower,argument,upper); // lower = argument / 2*pi
  // Get integer part
  number_integer(&lower,lower); // lower = int(lower)
  // If non-zero
  if(number_is_zero(lower)==0) {
    // Mult integer part to pi
    number_mult(&lower,upper,lower); // lower = 2*pi * lower
    // Substract period from argument
    number_sub(&argument,argument,lower); // argument = argument - lower
  }
  
  // If argument negative, add 2pi
  number_init(&lower); // lower = 0
  if(number_le(argument,lower)) { // if argument <= 0
    // Upper is still 2*pi
    number_add(&argument,argument,upper); // argument = argument + 2*pi
  }
  
  // If argument from pi to 2pi, then substract pi and invert sign of result
  number_init_pi(&upper);
  if(number_le(upper,argument)) { // pi <= argument
    inverse_result_sign=1; // inverse sign flag
    number_sub(&argument,argument,upper); // argument = argument - pi
  }
  
  // If argument from pi/2 to pi, then substract angle from pi/2
  number_init_pi_2(&upper);
  if(number_le(upper, argument)) { // pi/2 <= agrument
    number_sub(&argument, upper, argument); // argument = pi/2 - argument
  }
  
  // Calculating summ
  for(i=NUMBER_ITERATIONS_SIN-1;i>=0;i--) {
    number_pow_digit(&upper,argument,i*2+1);
    number_factor(&lower,i*2+1);
    number_div(&upper,upper,lower);
    if(i%2) upper.sign=1;
    else upper.sign=0;
    number_add(result,*result,upper);
  }

  // Inverse sign of result if needed
  if(inverse_result_sign) {
    number_inverse_sign(result);
  }
}

// Cosinus of any argument
void number_cos(struct number *result,struct number argument) {
  struct number upper;
  struct number lower;

  // Get pi/2
  number_init_pi_2(&upper);
  
  // pi/2 - argument
  number_sub(&argument,upper,argument);

  // Calculate as sinus
  number_sin(result,argument);
}

// Exponent
void number_exp(struct number *result,struct number argument) {
  struct number upper;
  struct number lower;
  counter_interations_t i;
  
  // Check for NaN
  if(number_is_nan(argument)) {
    number_init_nan(result);
    return;
  }
  
  number_init(result);
  
  for(i=NUMBER_ITERATIONS_EXP-1;i>=0;i--) {
    number_pow_digit(&upper,argument,i);
    number_factor(&lower,i);
    number_div(&upper,upper,lower);
    
    number_add(result,*result,upper);
  }
}

// Natural logarithm
void number_ln(struct number *result,struct number argument) {
  struct number upper;
  struct number lower;
  struct number integer_part;
  struct number one;
  char tmp[5];
  counter_interations_t i;
  char inverse_result_sign;

  // Check for NaN
  if(number_is_nan(argument)) {
    number_init_nan(result);
    return;
  }
  
  // Init variables
  inverse_result_sign=0;
  // result = 0
  number_init(result);

  // if argument<=0, set error flag
  if(number_le(argument,*result)) {
    number_init_nan(result);
    return;
  }

  // Best argument values are between 1/sqrt(e) and sqrt(e)
  
  // while argument > sqrt(e), then divide to e until sqrt(e) reached
  number_init_sqrt_e(&upper);
  number_init_e(&lower);
  number_init(&integer_part);
  number_init_one(&one);
  
  // While argument > sqrt(e)
  while(!number_le(argument,upper)) {
    number_div(&argument,argument,lower);
    number_add(&integer_part,integer_part,one);
  }
  
  // upper = 1 / sqrt(e)
  number_div(&upper,one,upper);
  // While argument < 1 / sqrt(e)
  while(!number_le(upper,argument)) {
    number_mult(&argument,argument,lower);
    number_sub(&integer_part,integer_part,one);
  }
  
  // argument = argument - 1
  number_sub(&argument,argument,one);
  
  for(i=NUMBER_ITERATIONS_LN;i>0;i--) {
    // upper = argument ^ i
    number_pow_digit(&upper,argument,i);
    
    // lower = i
    sprintf(tmp,"%d",(int)i);
    number_init_from_string(&lower,tmp);

    // upper = upper / lower
    number_div(&upper,upper,lower);

    // Change sign for even values of i
    if(i%2==0) {
      number_inverse_sign(&upper);
    }

    // Add
    number_add(result,*result,upper);
  }

  // Add integer part
  number_add(result,*result,integer_part);
}

// Logarithm base 10
void number_log10(struct number *result,struct number argument) {
  // result = 10
  number_init_ln10(result);
  // argument = ln(argument)
  number_ln(&argument,argument);
  char tmp[40];
  /*number_to_string(argument,tmp);
  tft.setCursor(0,CHAR_HEIGHT*5);
  tft.println(tmp);*/
  //Serial.println(tmp);
  /*tft.setCursor(0,CHAR_HEIGHT*7);
  number_to_string(*result,tmp);
  tft.println(tmp);*/
  //Serial.println(tmp);
  
  // log10 = ln(argument)/ln(10)
  number_div(result,argument,*result);
}
