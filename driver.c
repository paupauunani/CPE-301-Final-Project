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
volatile unsigned char* myPORTD = (unsigned char*) 0x2B;
volatile unsigned char* myDDRD = (unsigned char*) 0x2A;
volatile unsigned char* myPORTL = (unsigned char*) 0x10B;
volatile unsigned char* myDDRL = (unsigned char*) 0x10A;
volatile unsigned char* myPINL = (unsigned char*) 0x109;

/* initialize global dht */
DHT dht11(40, DHT11);
float humidity;
float temperature;

/* initialize global lcd */
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

/* initialize global rtc */
RTC_DS1307 rtc;

const int temp_threshold = 79;
const int water_low = 20;

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

void start_isr(void){
    button_state = true;
}

void state_change(void){
    usart_tx_str("State is now ");

    switch(state){
        case DISABLED:
        usart_tx_str("DISABLED. \n");
            break;
        case IDLE:
            usart_tx_str("IDLE. \n");
            break;
        case ERROR:
            usart_tx_str("ERROR. \n");
            break;
        case RUNNING:
            usart_tx_str("RUNNING. \n");
            break;
        default:
            break;
    }
}

void dht_info(void){
    temperature = dht11.readTemperature();
    humidity = dht11.readHumidity();
}

void rtc_date(void){
    DateTime now = rtc.now();
    usart_tx_uint(now.year());
}

void setup(void)
{       usart_init(16000000 / 16 / 9600 - 1);
        adc_init();
        // dht11.begin();
        lcd.begin(16, 2);
        lcd.setCursor(0,0);
        /* stepper speed set to 60 rpm */
        stepper.setSpeed(60);
        rtc.begin();
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        
        *myPORTH = 0b00000000; //clear data
        *myDDRH = 0b00000000; //clear data
        *myDDRH |= 0b01110000; //sets LED pins as output

        *myDDRD = 0b00000000; //start button as input
        *myPORTD |= 0b00001000; //pullup resistor

        attachInterrupt(digitalPinToInterrupt(18), start_isr, CHANGE);
        state_change();
} /* setup */

void loop(void)
{
    unsigned long current_mil = millis();    
    switch(state){
            case DISABLED:
                //Yellow LED should be on
                set_color(YELLOW);
                //No monitoring of temperature or water should be performed
                //Start button should be monitored using ISR
                if(button_state){
                    button_state = false;
                    state = IDLE;
                    state_change();
                    rtc_date();
                }
                break;
            case IDLE:
                //Green LED should be on
                set_color(GREEN);
                //Exact time stamp should record transition times
                //Water level should be continuously monitored
                break;
            case ERROR:
                //Red LED should be on
                set_color(RED);
                //Motor should be off and not start regardless of temperature
                //Reset button should trigger a change to IDLE stage if water is above threshold
                //Error message displayed on LCD
                break;
            case RUNNING:
                //Blue LED should be on
                set_color(BLUE);
                //Fan motor should be on
                //System should transition to IDLE as soon as temp drops below threshold
                //System should transition to ERROR if water becomes too low
                break;
            default:
                break;
        }
} /* loop */