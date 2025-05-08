/* written by Alysia Carr, Paulane Tulop, Shira Rotem, William Chuter-Davies */

/* must manually configure include path if compiling outside Arduino IDE */
// #include <LiquidCrystal.h>
// #include <DHT11.h>

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

// const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
// LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// DHT11 dht(A1)

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
        /* set input channel selection to adc_input_channel */
        *myADMUX |= adc_input_channel & 0b00000111;
        /* start analog-digital conversion */
        *myADCSRA |= 0b01000000;
        /* wait until conversion is complete */
        while(*myADCSRA & 0b01000000);
        /* return conversion result */
        return *myADCL | (*myADCH << 8);
} /* adc_read */

void uint_to_str(unsigned int n, unsigned char* str)
{       unsigned char buf[10];
        unsigned int i = 0;
        do
        {       buf[i++] = n % 10 + '0';
                n /= 10;
        } while(n > 0);
        for(unsigned int j = 0; j < i; ++j)
        {       str[j] = buf[i - j - 1];
        }
        str[i] = '\0';
} /* uint_to_str */

void usart_init(unsigned int usart_baud_rate)
{       /* set baud rate hi byte */
        *myUBRR0H = (unsigned char)usart_baud_rate >> 8;
        /* set baud rate lo byte */
        *myUBRR0L = (unsigned char)usart_baud_rate;
        /* enable reciever and transmitter */
        *myUCSR0B = 0b00011000;
        /* set frame format: 8 data bits, 2 stop bits */
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
        /* set tx buffer to usart_tx_data */
        *myUDR0 = usart_tx_data;
} /* usart_tx_char */

void usart_tx_str(unsigned char* usart_tx_data)
{       while(*usart_tx_data)
        {       usart_tx_char(*usart_tx_data++);
        }
} /* usart_tx_str */

void usart_tx_uint(unsigned int usart_tx_data)
{       unsigned char str[10];
        uint_to_str(usart_tx_data, str);
        usart_tx_str(str);
} /* usart_tx_uint */

void setup(void)
{       usart_init(16000000 / 16 / 9600 - 1);
        adc_init();
        // lcd.begin(16, 2);
        // dht.setDelay(1000);
} /* setup */

void loop(void)
{       usart_tx_uint(adc_read(0));
        usart_tx_char('\n');
        //unsigned char t = read_temp();
        //unsigned char h = read_humid();
        //printLCD(0, 0, "Temp: ", t);
        //printLCD(0, 1, "Humid: ", h);
} /* loop */

/*unsigned char read_temp_humid(){
    int temp = dht.readTemperature();
    unsigned char temperature;
    if((temp != DHT11::ERROR_CHECKSUM) && (temp != DHT11::ERROR_TIMEOUT)){
        temperature = <integer to char conversion>
    } else {
        temperature = '0';
    }
    return temperature;
} read_temp */

/*unsigned char read_humid(){
    int humid = dht.readHumidity();
    unsigned char humidity;
    if((humid != DHT11::ERROR_CHECKSUM) && (humid != DHT11::ERROR_TIMEOUT)){
        humidity = <integer to char conversion>
    } else {
        humidity = '0';
    }
    return humidity;
} read_humid */

/*void printLCD(int row, int col, str metric, char num){
    lcd.setCursor(row, col);
    lcd.print(metric);
    lcd.print(num);
} printLCD */