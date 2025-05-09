/* written by Alysia Carr, Paulane Tulop, Shira Rotem, William Chuter-Davies */

/* must manually configure include paths if compiling outside Arduino IDE */
#include <DHT.h>
#include <LiquidCrystal.h>
#include <RTClib.h>
#include <Stepper.h>

/* registers required for adc functionality */
volatile unsigned char* myADMUX = (unsigned char*) 0x7C;
volatile unsigned char* myADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* myADCSRA = (unsigned char*) 0x7A;
volatile unsigned char* myADCH = (unsigned char*) 0x79;
volatile unsigned char* myADCL = (unsigned char*) 0x78;

/* registers required for usart functionality */
volatile unsigned char* myUDR0 = (unsigned char*) 0xC6;
volatile unsigned char* myUBRR0H = (unsigned char*) 0xC5;
volatile unsigned char* myUBRR0L = (unsigned char*) 0xC4;
volatile unsigned char* myUCSR0C = (unsigned char*) 0xC2;
volatile unsigned char* myUCSR0B = (unsigned char*) 0xC1;
volatile unsigned char* myUCSR0A = (unsigned char*) 0xC0;

/* registers required for gpio functionality */
volatile unsigned char* myPORTD = (unsigned char*) 0x2B;
volatile unsigned char* myDDRD = (unsigned char*) 0x2A;
volatile unsigned char* myPORTH = (unsigned char*) 0x102;
volatile unsigned char* myDDRH = (unsigned char*) 0x101;
volatile unsigned char* myPORTL = (unsigned char*) 0x10B;
volatile unsigned char* myDDRL = (unsigned char*) 0x10A;

/* initialise global dht */
DHT dht11(43, DHT11);

/* initialise global lcd */
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/* initialise global rtc */
RTC_DS1307 rtc;

/* initialise global stepper */
Stepper stepper(200, 23, 25, 27, 29);

/* initialise program flags */
unsigned int stepper_displaced = 0;
unsigned int system_disabled = 0;
unsigned int system_state = 0;
unsigned int system_state_reported = 0;

void adc_init()
{       /* clear adc multiple selection register */
        *myADMUX = 0b00000000;
        /* set voltage reference to AVcc */
        *myADMUX |= 0b01000000;
        /* clear adc control and status register b */
        *myADCSRB = 0b00000000;
        /* clear adc control and status register a */
        *myADCSRA = 0b00000000;
        /* enable adc */
        *myADCSRA |= 0b10000000;
        /* set pre-scaler to 128 */
        *myADCSRA |= 0b00000111;
} /* adc_init */

unsigned int adc_read(unsigned char adc_input_channel)
{       /* clear input channel selection */
        *myADMUX &= 0b11100000;
        /* set input channel selection to 'adc_input_channel' */
        *myADMUX |= adc_input_channel & 0b00000111;
        /* start analog to digital conversion */
        *myADCSRA |= 0b01000000;
        /* wait until conversion is complete */
        while(*myADCSRA & 0b01000000);
        /* return conversion result */
        return *myADCL | (*myADCH << 8);
} /* adc_read */

void uint_to_str(unsigned int n, unsigned char* str)
{       /* initialise local buffer */
        unsigned char buf[10];
        /* initialise local index */
        unsigned int i = 0;
        /* store digits of 'n' in local buffer */
        do
        {       buf[i++] = n % 10 + '0';
                n /= 10;
        } while(n > 0);
        /* reverse 'buf' into 'str' */
        for(unsigned int j = 0; j < i; ++j)
        {       str[j] = buf[i - j - 1];
        }
        /* null-terminate 'str' */
        str[i] = '\0';
} /* uint_to_str */

void usart_init(unsigned int usart_baud_rate)
{       /* set baud rate hi byte to 'usart_baud_rate' */
        *myUBRR0H = (unsigned char)usart_baud_rate >> 8;
        /* set baud rate lo byte to 'usart_baud_rate' */
        *myUBRR0L = (unsigned char)usart_baud_rate;
        /* enable reciever and transmitter */
        *myUCSR0B = 0b00011000;
        /* set frame format to 8 data bits; 2 stop bits */
        *myUCSR0C = 0b00001110;
} /* usart_init */

unsigned char usart_rx(void)
{       /* wait until there are unread data in rx buffer */
        while(!(*myUCSR0A & 0b10000000));
        /* return data in rx buffer */
        return *myUDR0;
} /* usart_rx */

void usart_tx_char(unsigned char usart_tx_data)
{       /* wait until there are no data in tx buffer */
        while(!(*myUCSR0A & 0b00100000));
        /* set tx buffer to 'usart_tx_data' */
        *myUDR0 = usart_tx_data;
} /* usart_tx_char */

void usart_tx_str(unsigned char* usart_tx_data)
{       /* tx 'usart_tx_data' */
        while(*usart_tx_data)
        {       usart_tx_char(*usart_tx_data++);
        }
} /* usart_tx_str */

void usart_tx_uint(unsigned int usart_tx_data)
{       /* initialise local string */
        unsigned char str[11];
        /* convert 'usart_tx_data' into 'str' */
        uint_to_str(usart_tx_data, str);
        /* tx 'str' */
        usart_tx_str(str);
} /* usart_tx_uint */

void rtc_tx_time(void)
{       /* initialise DateTime object */
        DateTime now = rtc.now();
        /* tx 'now' as "M / D / Y | H : M : S" */
        usart_tx_uint(now.month());
        usart_tx_str(" / ");
        usart_tx_uint(now.day());
        usart_tx_str(" / ");
        usart_tx_uint(now.year());
        usart_tx_str(" | ");
        usart_tx_uint(now.hour());
        usart_tx_str(" : ");
        usart_tx_uint(now.minute());
        usart_tx_str(" : ");
        usart_tx_uint(now.second());
} /* rtc_tx_time */

void stepper_step(void)
{       /* step backward one revolution if 'stepper_displaced' is true */
        if(stepper_displaced)
        {       stepper.step(-200);
                /* toggle 'stepper_displaced' */
                stepper_displaced = 0;
        }
        /* step forward one revolution if 'stepper_displaced' is false */
        else
        {       stepper.step(200);
                /* toggle 'stepper_displaced' */
                stepper_displaced = 1;
        }
} /* stepper_step */

void setup(void)
{       /* initialise usart */
        usart_init(16000000 / 16 / 9600 - 1);
        /* initialise adc */
        adc_init();
        /* initialise dht */
        dht11.begin();
        /* initialise lcd */
        lcd.begin(16, 2);
        /* set initial cursor position to 0,0 */
        lcd.setCursor(0,0);
        /* initialise rtc */
        rtc.begin();
        /* set initial rtc time to compile time */
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        /* set stepper speed to 60 rpm */
        stepper.setSpeed(60);
        /* set interrupt service routine one to 'system_power_isr' */
        attachInterrupt(digitalPinToInterrupt(18), system_power_isr, CHANGE);
        /* set interrupt service routine two to 'system_reset_isr' */
        attachInterrupt(digitalPinToInterrupt(19), system_reset_isr, CHANGE);
        /* clear port d data register */
        *myPORTD = 0b00000000;
        /* clear port d data direction register */
        *myDDRD = 0b00000000;
        /* set digital pin 18 to recieve input with pullup */
        *myPORTD |= 0b00001000;
        /* clear port h data register */
        *myPORTH = 0b00000000;
        /* clear port h data direction register */
        *myDDRH = 0b00000000;
        /* set digital pin 16 to recieve input with pullup */
        *myPORTH |= 0b00000010;
        /* set digital pins 7, 8, and 9 directions to out */
        *myDDRH |= 0b01110000;
        /* clear port l data register */
        *myPORTL = 0b00000000;
        /* clear port l data direction register */
        *myDDRL = 0b00000000;
        /* set digital pin 45 to recieve input with pullup */
        *myPORTL |= 0b00001000;
        /* set digital pin 42 direction to out */
        *myDDRL |= 0b10000000;
} /* setup */

void loop(void)
{       if(system_state != 0)
        {       lcd.clear();
                lcd.write("Tem: ");
                lcd.print((unsigned int)dht11.readTemperature());
                lcd.setCursor(0,1);
                lcd.write("Hum: ");
                lcd.print((unsigned int)dht11.readHumidity());
        }
        switch(system_state)
        {       /* state: disabled */
                case 0:
                        /* set led colour to yellow */
                        *myPORTH = 0b01100000;
                        *myPORTL &= 0b01111111;
                        if(!system_state_reported)
                        {       usart_tx_str("System DISABLED: ");
                                rtc_tx_time();
                                usart_tx_char('\n');
                                system_state_reported = 1;
        
                        }
                        if(system_disabled)
                        {       system_state = 1;
                                system_state_reported = 0;
                                Serial.println("button was pressed");
                        }
                        break;
                /* state: idle */
                case 1:
                        /* set led colour to green */
                        *myPORTH = 0b00100000;
                        if(!system_state_reported)
                        {       usart_tx_str("System IDLE: ");
                                rtc_tx_time();
                                usart_tx_char('\n');
                                system_state_reported = 1;
                        }
                        if(adc_read(0) < 20)
                        {       system_state = 2;
                                system_state_reported = 0;
                        }
                        break;
                /* state: error */
                case 2:
                        /* set led colour to red */
                        *myPORTH = 0b01000000;
                        *myPORTL &= 0b0111111;
                        lcd.clear();
                        lcd.print("ERROR: Water lvl");
                        lcd.setCursor(7,1);
                        lcd.print("is low");
                        if(!system_state_reported)
                        {       usart_tx_str("System ERROR: ");
                                rtc_tx_time();
                                usart_tx_char('\n');
                                system_state_reported = 1;
                        }
                        break;
                /* state: running */
                case 3:
                        /* set led colour to blue */
                        *myPORTH = 0b00010000;
                        *myPORTL |= 0b10000000;
                        if(dht11.readTemperature() < 75)
                        {       system_state = 1;
                                system_state_reported = 0;
                        }
                        if(adc_read(0) < 20)
                        {       system_state = 2;
                                system_state_reported = 0;
                        }
                        if(!system_state_reported)
                        {       usart_tx_str("System RUNNING: ");
                                rtc_tx_time();
                                usart_tx_char('\n');
                                system_state_reported = 1;
                        }
                        break;
                default:
                        break;
        }
        delay(1000);
} /* loop */

void system_power_isr(void)
{       /* toggle system disabled flag */
        system_disabled = !system_disabled; 
} /* system_power_isr */

void system_reset_isr(void)
{     
  if(system_state == 2 && !(adc_read(0) < 20))
        {       system_state = 1;
                system_state_reported = 0;
        }
} /* void system_reset_isr(void) */