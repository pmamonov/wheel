#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "../lcd/lcd.h"
#include "../time/time.h"

//eeprom
//#define EEPROM_LEN 1024

// buttons
#define BUT0 0xb
#define BUT1 0xd
#define BUT2 0x7
#define BUT3 0xe

// update signals

#define UCLK 0x1
#define UCNT 0x2
#define USCR 0x4
#define UKBD 0x8

// top modes
#define MCLK 1
#define MCNT 2
#define MSMB 3

// clock submodes
#define MCLK_SY 3
#define MCLK_SM 2
#define MCLK_SD 1
#define MCLK_Sh 4
#define MCLK_Sm 5
#define MCLK_Ss 6
#define MCLK_N MCLK_Ss

//counter submodes
#define MCNT_HST 1
#define MCNT_ST 2

#define MCNT_ST_T 1
#define MCNT_ST_Tm 2

#define SLEN 32

volatile time_t time; // epoch, uint32_t
volatile unsigned int ds; // tenths of second
volatile unsigned char kbd_delay; // delay user input
volatile unsigned char s_delay; // delay display shift
volatile unsigned char update; // update on external events
volatile unsigned long int cnt; // current counter value
volatile uint8_t trg;

struct exphead_t{
  unsigned int eeprom;
  time_t start;
  unsigned int T;
  unsigned int N;
};

void read_head(unsigned int p, struct exphead_t *exphead){
    exphead->eeprom = p;
    exphead->start = eeprom_read_dword(p);
    exphead->T = eeprom_read_word(p+4);
    exphead->N = eeprom_read_word(p+6);
}

ISR(INT0_vect){
  if (trg==1){
    cnt++;
    update |= UCNT;
  }
  trg = 0;
}

ISR(INT1_vect){
/*  
  if (trg==0) {
    cnt++;
    update |= UCNT;
  }
  */
  trg = 1;
}

ISR(TIMER1_OVF_vect){
  TCNT1 = TIM_START_VAL;
  ds++;
  if (ds >= 10){
    ds = 0;
    time++;
    update |=  UCLK;
  }
  if (kbd_delay) kbd_delay--;
  if (s_delay) s_delay--;
}


void main(void) {
  char s[SLEN]; // buffer for LCD
  unsigned char sover=0;
  unsigned char sshift=0;
  uint32_t ssum=0;
  uint32_t ssumprev=0;

  int menu[] = {1,0,0,0}; // menu structure

  uint8_t kbd; // keyboard input
  char kbd_processed;

//  unsigned char clk=0;
  struct tm date; // human readable date
  unsigned int i;
  uint32_t x;

  unsigned int NExp=0;
  struct exphead_t exphead = {5,0,0,0};
  int T=1;
  char Tmult='\xc0'; // ч

  struct exphead_t hsthead = {5,0,0,0};
  int hstT;
  char hstU;

  uint32_t *mem, *mem0;
  unsigned int lmem=256;
  unsigned int gap;
  
  do {
    mem = malloc(lmem*4);
    lmem--;
  } while (!mem);
  for (i=0; i <= lmem; i++) mem[i] = 0xabadbabe;
  mem0 = mem;
  free(mem);

// read signature and number of exps
  x = eeprom_read_dword(0);
  if (x == 0xdeadbeef){
    NExp = eeprom_read_byte(4); // number of experiments
    for (i=0; i < NExp; i++){
        if (exphead.eeprom+8 < EEPROM_LEN){
          read_head(exphead.eeprom, &exphead);
          exphead.eeprom += 8+exphead.N*3;
        }
        else{
          NExp = i;
          break;
        }
    }
  }
  else {
    eeprom_write_dword(0, 0xdeadbeef);
    eeprom_write_byte(4, 0);
  }
  exphead.T=0;
  exphead.N=0;

  time = 0;
  update = 0xff;
  ds=0;
  cnt = 0;
  trg = 0;
  kbd_delay = 0;
  s_delay = 0;

  GICR |= (1 << INT0) | (1 << INT1); // enable int0, int1
  MCUCR |= 0xa; // xxxx1010: int0,1 on falling edge

  TIMSK |= (1 << TOIE1); // enable int on timer overflow
  TCNT1 = TIM_START_VAL;
  TCCR1B |= (1<<CS11)|(1<<CS10); // prescaler 1/64

// enable pull-up resistors
  PORTA = 0xff;
  PORTB = 0xff;
  PORTC = 0xff;
  PORTD = 0xff; // |= (1 << 2); //enable pull-up at PD2/INT0

// for monitoring
//  DDRB |= 0x08;

// initialize LCD
  lcd_Init();

  sei(); // enable ints

// MAIN LOOP
  while (1) {

/* // monitor working cycle
  if (PORTB & 0x08)
    PORTB &= 0xf7;
  else
    PORTB |= 0x08;*/

//save counter
  if (exphead.T){
    if (time >= exphead.start + exphead.T*60L){
// if there is enough room in eeprom?
      if (exphead.eeprom + 8 + exphead.N*3 + 2 < EEPROM_LEN){
// save counter value to eeprom
        for (i=0; i<3 ; i++){
          eeprom_write_byte( exphead.eeprom + 8 + exphead.N*3 + i, (uint8_t)(cnt >> (i*8)) );
        }
// reset counter
        cnt=0;
// increment number of points
        exphead.N++;
// save exp head to eeprom on first point acquisition
        if (exphead.N==1){
          NExp++;
          eeprom_write_byte(4, NExp);
          eeprom_write_dword(exphead.eeprom, exphead.start);
          eeprom_write_word(exphead.eeprom+4, exphead.T);
        }
// update exp head number of points in eeprom
        eeprom_write_word(exphead.eeprom+6, exphead.N);
// increment exphead.start
        exphead.start += exphead.T*60L;
//        exphead.start += exphead.T;
      }
      else {
        exphead.T=0;
        exphead.eeprom=EEPROM_LEN;
      }
    }
  }


// handle user input
    kbd = PINB & 0xf;
    if (!kbd_delay && kbd < 0xf){
      update |= UKBD;
      kbd_delay = 5;
      kbd_processed = 0;

// menu-specific keyboard processing
      switch (menu[0]){
// clock setup
        case MCLK:
          if (menu[2] && kbd == BUT1) kbd_processed=1; // 
          switch (menu[1]){
            case MCLK_Sm:
              if (kbd==BUT3) kbd_processed=1;
              break;
          }

          if (menu[2]){
            stamp2date(time, &date);
            switch (kbd){
              case BUT3:
                switch (menu[1]){
                  case MCLK_Sm:
                    date.min++;
                    break;
                  case MCLK_Sh:
                    date.hour++;
                    break;
                  case MCLK_SD:
                    date.day++;
                    break;
                  case MCLK_SM:
                    date.mon++;
                    break;
                  case MCLK_SY:
                    date.year++;
                    break;
                }
                date2stamp(&date, &time);
                update |= UCLK;
                kbd_processed = 1;
                break;
              case BUT2:
                switch (menu[1]){
                  case MCLK_Sm:
                    date.min--;
                    break;
                  case MCLK_Sh:
                    date.hour--;
                    break;
                  case MCLK_SD:
                    date.day--;
                    break;
                  case MCLK_SM:
                    date.mon--;
                    break;
                  case MCLK_SY:
                    date.year--;
                    break;
                }
                date2stamp(&date, &time);
                update |= UCLK;
                kbd_processed = 1;
                break;
            }
          }
          break;
// history & counter setup
        case MCNT:
          if (!menu[1] && kbd == BUT3){
            kbd_processed = 1;
            break;
          }
          switch (menu[1]){
            case MCNT_HST:
              if (menu[3] && menu[3]==hsthead.N && kbd==BUT3){                
                kbd_processed = 1;
                break;
              }
              if (menu[2] == (1+NExp)){
                switch (kbd){
                  case BUT3:
                    kbd_processed=1;
                    break;
                  case BUT1:
                  // clear stats
                    if (menu[3]){
                      NExp = 0;
                      exphead.T = 0;
                      exphead.eeprom=5;
                      eeprom_write_byte(4, 0);
                      menu[3]=0;
                      menu[2]=0;
                      kbd_processed = 1;
                    }
                    break;
                }
              }
              break;

            case MCNT_ST:
              if (kbd == BUT3 && (!menu[2] || menu[2]==MCNT_ST_Tm && !menu[3]) ) {
                kbd_processed=1;
                break;
              }

              // stop data collection
              if (exphead.T && kbd == BUT1 && !menu[2]){
                exphead.T=0;
                if (exphead.N){
                  exphead.eeprom += 8+exphead.N*3;
                }
                kbd_processed=1;
                break;
              }

              // start data collection on exit from submenu
              if (menu[2] && !menu[3] && kbd == BUT0 && exphead.eeprom+11 < EEPROM_LEN ){
                cnt=0;
                exphead.start=time;
                exphead.T=T;
                exphead.N=0;
                switch(Tmult){
                  case '\xc0':
                    exphead.T *= 60;
                    break;
                  case '\xe3':
                    exphead.T *= 60*24;
                    break;
                }
//                kbd_processed=1;
                break;
              }

              //data collection setup
              if (menu[3]){
                switch (menu[2]){
                  case MCNT_ST_T:
                  // period setup
                      switch (kbd){
                        case BUT3:
                          T++;
                          kbd_processed = 1;
                          break;
                        case BUT2:
                          if (T>1) T--;
                          kbd_processed = 1;
                          break;
                      }
                  break;
                  case MCNT_ST_Tm:
                  //period multiplier
                      switch (kbd){
                        case BUT3:
                          switch (Tmult){
                            case '\xbc':
                              Tmult = '\xc0';
                              break;
                            case '\xc0':
                              Tmult = '\xe3';
                              break;
                          }
                          kbd_processed = 1;
                          break;
                        case BUT2:
                          switch (Tmult){
                            case '\xc0':
                              Tmult = '\xbc';
                              break;
                            case '\xe3':
                              Tmult = '\xc0';
                              break;
                          }
                          kbd_processed = 1;
                          break;
                      }
                }
              }
              break;
            }
	    break;

        case MSMB:
          if (menu[1])
            switch (kbd){
              case BUT3:
                if (menu[1] < 0xff) break;
              case BUT1:
                kbd_processed=1;
                break;
            }

          break;

        case 5:
          if (kbd==BUT3) kbd_processed=1;
          break;
      }

// default kbd processing == menu navigation
      if (!kbd_processed) {
        switch (kbd){
// menu down
          case BUT1:
          for (i=0; menu[i] && i<3; i++);
          if (!menu[i]) menu[i]=1;
          break;

// menu up
          case BUT0:
          for (i=3; !menu[i] && i>1; i--);
          menu[i]=0;
          break;

// lateral navigation | special functions
          case BUT3:
          for (i=3; !menu[i] && i>0; i--);
          menu[i]++; // will correct it further
          break;

          case BUT2:
          for (i=3; !menu[i] && i>0; i--);
          menu[i] = (menu[i] > 1) ? menu[i]-1: menu[i];
          break;
        }
      }
    }

// update screen if needed
    if (update) {
      switch (menu[0]) {
        case MCLK:
          if (!(update & (UCLK | UKBD))) break;
          stamp2date(time, &date);
          switch (menu[1]){
/*            case MCLK_EP:
              snprintf(s, SLEN, "%16lu", time);
              break;*/
            case MCLK_SY:
            if (menu[2])
              snprintf(s, SLEN, "%02d.%02d.<%4d>", date.day, date.mon, date.year);
	    else
              snprintf(s, SLEN, "%02d.%02d.>%02d< %02d:%02d", date.day, date.mon, date.year%100, date.hour, date.min);
            break;
            case MCLK_SM:
	    if (menu[2])
              snprintf(s, SLEN, "%02d.<%02d>.%02d %02d:%02d", date.day, date.mon, date.year%100, date.hour, date.min);
	    else
              snprintf(s, SLEN, "%02d.>%02d<.%02d %02d:%02d", date.day, date.mon, date.year%100, date.hour, date.min);
            break;
            case MCLK_SD:
	    if (menu[2])
              snprintf(s, SLEN, "<%02d>.%02d.%02d %02d:%02d", date.day, date.mon, date.year%100, date.hour, date.min);
	    else
              snprintf(s, SLEN, ">%02d<.%02d.%02d %02d:%02d", date.day, date.mon, date.year%100, date.hour, date.min);
            break;
            case MCLK_Sh:
	    if (menu[2])
              snprintf(s, SLEN, "%02d.%02d.%02d <%02d>:%02d", date.day, date.mon, date.year%100, date.hour, date.min);
	    else
              snprintf(s, SLEN, "%02d.%02d.%02d >%02d<:%02d", date.day, date.mon, date.year%100, date.hour, date.min);
            break;
            case MCLK_Sm:
	    if (menu[2])
              snprintf(s, SLEN, "%02d.%02d.%02d %02d:<%02d>", date.day, date.mon, date.year%100, date.hour, date.min);
	    else
              snprintf(s, SLEN, "%02d.%02d.%02d %02d:>%02d<", date.day, date.mon, date.year%100, date.hour, date.min);
            break;
            case MCLK_Ss:
            snprintf(s, SLEN, "< %02d:%02d:%02d", date.hour, date.min, date.sec);
            menu[2]=0;
	    break;
            default:
            snprintf(s, SLEN, "%02d.%02d.%02d %02d:%02d >", date.day, date.mon, date.year%100, date.hour, date.min);
          }
          break;
        case MCNT:
/*          if (update & (UCNT | UKBD | UCLK)){
            update |= USCR;
          } 
          else{
            break;
          }*/
          switch (menu[1]){
            case MCNT_HST:
              if (menu[2] > NExp) {
                if (menu[3]){
                  snprintf(s, SLEN, "\xa9\x42\x45PEH\xae?" ); // УВЕРЕНЫ?
                }
                else{
                  snprintf(s, SLEN, "O\xab\xa5\x43T\xa5Tb") ; //ОЧИСТИТЬ
                }
                break;
              }
              if (menu[2]){
              // browse stats
              //read stats from eeprom
                hsthead.eeprom=5;
                i=0;
                while ( i < menu[2]){
                  read_head(hsthead.eeprom, &hsthead);
                  i++;
                  if (i < menu[2]) hsthead.eeprom += 8 + hsthead.N*3;
                };
                if (menu[3]){
                // print values
                   x = eeprom_read_dword(hsthead.eeprom+8+3*(menu[3]-1)) & 0x00ffffff;
                   snprintf(s, SLEN, "%3d/%3d:%8lu", menu[3], hsthead.N, x); // Values
                }
                else{
                // stat header
                  stamp2date(hsthead.start, &date);
                  hstT=hsthead.T;
                  hstU='\xbc'; // м
                  if (hstT/60){
                    hstT /= 60;
                    hstU = '\xc0'; // ч
                  }
                  if (hstT/1440){
                    hstT /= 1440;
                    hstU = '\xe3'; // д
                  }
                  snprintf(s, 33, "%2d %02d%02d%02d %02d:%02d %d%c/%d ", \
                                  menu[2], date.day, date.mon, date.year%100, date.hour, date.min, hstT,hstU, hsthead.N); // Exp. start
                }
              }
              else {
                snprintf(s, SLEN, "CTAT\xa5\x43T\xa5KA %3d >", NExp); // СТАТИСТИКА
              }
              break;
            case MCNT_ST:
              switch (menu[2]){
                case MCNT_ST_T:
                  if (menu[3]) {
                    snprintf(s, SLEN, "\xa8\x45P\xa5O\xe0:  <%3d> %c", T, Tmult); // ПЕРИОД
                  }
                  else{
                    snprintf(s, SLEN, "\xa8\x45P\xa5O\xe0:  >%3d< %c", T, Tmult); // ПЕРИОД                
                  }
                  break;
                case MCNT_ST_Tm:
                  if (menu[3]) {
                    snprintf(s, SLEN, "\xa8\x45P\xa5O\xe0:  %3d <%c>", T, Tmult);
                  }
                  else{
                    snprintf(s, SLEN, "\xa8\x45P\xa5O\xe0:  %3d >%c<", T, Tmult);
                  }
                  break;
                default:
                  if (exphead.T){
                    snprintf(s, SLEN, "<OCTAHOB\xa5Tb C\xa0OP"); // ОСТАНОВИТЬ СБОР
                  }
                  else{
                    snprintf(s, SLEN, "<HA\xab\x41Tb C\xa0OP %3d", \
                             exphead.eeprom+8 < EEPROM_LEN ? (EEPROM_LEN-exphead.eeprom-8)/3 : 0); // НАЧАТЬ СБОР
                  }
                }
              break;
            default:
	      if (exphead.T){
                snprintf(s, SLEN, "< %2d%c/%3d:%6lu", T, Tmult, exphead.N, cnt);
	      }
	      else{
                snprintf(s, SLEN, "< %14lu", cnt);
              }
              break;
          }
          break;

        case MSMB:
          if (update & UKBD){
            update |= USCR;
          }
          else {
            break;
          }
          if (menu[1])
            snprintf(s, SLEN, "%02X: %c", menu[1], (char)menu[1]);
          else
            snprintf(s, SLEN, "LCD CHARSET");
          break;

        case 4:
          stamp2date(exphead.start, &date);
          snprintf(s, SLEN, "%02d.%02d.%02d %02d:%02d", date.day, date.mon, date.year%100, date.hour, date.min);
          update |= USCR;
          break;

        case 5:
          gap=0;
          for (i=0; i <= lmem; i++)
            if (mem0[i] == 0xabadbabe) gap++;
          snprintf(s, SLEN, "GAP: %u/%u dw", lmem, gap);
          update |= USCR;
          break;

        default:
          break;
      }
// scroll screen if needed
      sover = strlen(s);
      sover = sover > 16 ? sover - 16 : 0;
      if (update & UKBD){
        sshift=0;
      }

      ssum = 0;
      for (i=0; i < SLEN/4; i++) ssum += ((uint32_t*)s)[i];
 
 // actual screen update     
      if ( (update & UKBD) || ssum!=ssumprev || sover && !s_delay ){
        lcd_Clear();
        lcd_Print(s + sshift);
        if (!s_delay && sover){ 
          sshift = sshift < sover ? sshift+1 : 0;
          s_delay=10;
        }
      }
      ssumprev=ssum;

// reset update flags
      update = 0x0;
    }
  }
}
