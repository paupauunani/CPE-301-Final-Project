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

/* registers required for timer functionality */
volatile unsigned char* myTCCR1A = (unsigned char*) 0x80;
volatile unsigned char* myTCCR1B = (unsigned char*) 0x81;
volatile unsigned char* myTCCR1C = (unsigned char*) 0x82;
volatile unsigned char* myTIMSK1 = (unsigned char*) 0x6F;
volatile unsigned char* myTIFR1 = (unsigned char*) 0x36;
volatile unsigned int* myTCNT1 = (unsigned int*) 0x84;

/* registers required for gpio functionality */
volatile unsigned char* myPORTH = (unsigned char*) 0x102;
volatile unsigned char* myDDRH = (unsigned char*) 0x101;
volatile unsigned char* myPORTL = (unsigned char*) 0x10B;
volatile unsigned char* myDDRL = (unsigned char*) 0x10A;
volatile unsigned char* myPINL = (unsigned char*) 0x109;

/* initialize global dht */
DHT dht11(40, DHT11);

/* initialize global lcd */
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/* initialize global rtc */
RTC_DS1307 rtc;

/* initialize global stepper */
Stepper stepper(200, 22, 24, 26, 28);
bool stepper_flag = 0;

/* initialize interrupt states*/
enum State{DISABLED, IDLE, ERROR, RUNNING};
State state = DISABLED;
volatile bool button_state = false;

/* define color states of the LED */
enum Color{YELLOW, RED, BLUE, GREEN};
Color color = YELLOW;

const int red_pin = 9;
const int green_pin = 8;
const int blue_pin = 7;

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
        /* set frame format to 8 data bits, 2 stop bits */
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
        /* tx local string */
        usart_tx_str(str);
} /* usart_tx_uint */

void stepper_step(void)
{       /* step backward one revolution if 'stepper_flag' is true */
        if(stepper_flag)
        {       stepper.step(-200);
        }
         /* step forward one revolution if 'stepper_flag' is false */
        else
        {       stepper.step(200);
        }
        /* toggle 'stepper_flag' */
        stepper_flag = !stepper_flag;
} /* stepper_step */

void set_color(Color c){
    *myPORTH = 0b00000000;

    switch(c){
        case RED:
            *myPORTH |= 0b01000000;
            break;
        case GREEN:
            *myPORTH |= 0b00100000;
            break;
        case BLUE:
            *myPORTH |= 0b00010000;
            break;
        case YELLOW:
            *myPORTH |= 0b01100000;
            break;
        default:
            break;
    }
} /* set_color */

void setup(void)
{       usart_init(16000000 / 16 / 9600 - 1);
        adc_init();
        // dht11.begin();
        lcd.begin(16, 2);
        lcd.setCursor(0,0);
        lcd.print("hello, world!");
        /* stepper speed set to 60 rpm */
        stepper.setSpeed(60);
        // rtc.begin();
        // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        /* clear port l data register */
        *myPORTH = 0b00000000;
        /* clear port l data direction register */
        *myDDRH = 0b00000000;
        /* set digital pin 40 direction to out */
        *myDDRH |= 0b01110000;
} /* setup */

void loop(void)
{       // DateTime now = rtc.now();
        switch(state){
            case DISABLED:
                //Yellow LED should be on
                set_color(YELLOW);
                break;
            case IDLE:
                //Green LED should be on
                set_color(GREEN);
                break;
            case ERROR:
                //Red LED should be on
                set_color(RED);
                break;
            case RUNNING:
                //Blue LED should be on
                set_color(BLUE);
                break;
            default:
                break;
        }
} /* loop */