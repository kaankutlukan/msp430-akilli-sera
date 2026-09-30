#include <msp430g2553.h>

#define SCL_PIN   BIT1
#define SDA_PIN   BIT2
#define DHT_PIN   BIT4 
#define ROLE_PIN  BIT0
#define LED_PIN   BIT6
#define OLED_ADDR 0x3C

#define SOIL_DRY  710
#define SOIL_WET  300
#define MEASURE_INTERVAL 5000  
#define SAMPLES_PER_DAY  3    

#define POMPAYI_DURDUR()    (P2OUT &= ~ROLE_PIN)   
#define POMPAYI_CALISTIR()  (P2OUT |= ROLE_PIN)    

unsigned char day_scores[7]  = {0,0,0,0,0,0,0};
unsigned char day_count      = 0;
unsigned long score_sum      = 0;
unsigned char sample_in_day  = 0;


unsigned long system_uptime  = 0; 
unsigned int timer_tick      = 0;

void wait_ms(unsigned int ms) { unsigned int i; for(i = 0; i < ms; i++) __delay_cycles(1000); }

// I2C //
void scl_high(void) { P2DIR &= ~SCL_PIN; }
void scl_low(void)  { P2DIR |=  SCL_PIN; P2OUT &= ~SCL_PIN; }
void sda_high(void) { P2DIR &= ~SDA_PIN; }
void sda_low(void)  { P2DIR |=  SDA_PIN; P2OUT &= ~SDA_PIN; }
void i2c_start(void) { sda_high(); scl_high(); __delay_cycles(5); sda_low(); __delay_cycles(5); scl_low(); __delay_cycles(5); }
void i2c_stop(void)  { sda_low(); __delay_cycles(5); scl_high(); __delay_cycles(5); sda_high(); __delay_cycles(5); }
void i2c_write_byte(unsigned char byte) {
    unsigned char i;
    for(i = 0; i < 8; i++) {
        if(byte & 0x80) sda_high(); else sda_low();
        __delay_cycles(2); scl_high(); __delay_cycles(5); scl_low(); __delay_cycles(2);
        byte <<= 1;
    }
    sda_high(); scl_high(); __delay_cycles(5); scl_low(); __delay_cycles(5);
}

// OLED //
void oled_cmd(unsigned char c)  { i2c_start(); i2c_write_byte(OLED_ADDR<<1); i2c_write_byte(0x00); i2c_write_byte(c); i2c_stop(); }
void oled_data(unsigned char d) { i2c_start(); i2c_write_byte(OLED_ADDR<<1); i2c_write_byte(0x40); i2c_write_byte(d); i2c_stop(); }
void oled_init(void) { wait_ms(200); oled_cmd(0xAE); oled_cmd(0x8D); oled_cmd(0x14); oled_cmd(0xAF); }
void oled_clear(void) { unsigned char p, c; for(p = 0; p < 8; p++) { oled_cmd(0xB0|p); oled_cmd(0x00); oled_cmd(0x10); for(c = 0; c < 128; c++) oled_data(0x00); } }
void oled_clear_line(unsigned char page) { unsigned char c; oled_cmd(0xB0|page); oled_cmd(0x00); oled_cmd(0x10); for(c = 0; c < 128; c++) oled_data(0x00); }

// FONT //
const unsigned char FONT[][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    {0x7F,0x02,0x04,0x08,0x7F}, {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x06,0x08,0x06,0x7F}, {0x23,0x13,0x08,0x64,0x62},
    {0x3E,0x41,0x41,0x41,0x22}, {0x00,0x06,0x09,0x09,0x06},
    {0x00,0x41,0x7F,0x41,0x00}, {0x27,0x45,0x45,0x45,0x39},
    {0x7F,0x08,0x14,0x22,0x41}, {0x7F,0x09,0x19,0x29,0x46},
    {0x00,0x36,0x36,0x00,0x00}, {0x7F,0x40,0x40,0x40,0x40},
    {0x3E,0x41,0x41,0x41,0x3E}, {0x7C,0x12,0x11,0x12,0x7C},
    {0x3F,0x40,0x40,0x40,0x3F}, {0x01,0x01,0x7F,0x01,0x01},
    {0x1E,0x21,0x21,0x21,0x7E}, {0x7F,0x09,0x09,0x09,0x01},
    {0x00,0x00,0x00,0x00,0x00}
};

void oled_char(unsigned char page, unsigned char col, unsigned char idx) {
    unsigned char i; oled_cmd(0xB0|page); oled_cmd(col&0x0F); oled_cmd(0x10|(col>>4));
    for(i = 0; i < 5; i++) oled_data(FONT[idx][i]); oled_data(0x00);
}

void draw_graph(void) {
    unsigned char page, col, bar_idx, b;
    unsigned char col_data;
    unsigned char height_in_pixels;

    oled_clear(); 
    for(page = 0; page < 8; page++) {
        oled_cmd(0xB0 | page); oled_cmd(0x00); oled_cmd(0x10);
        for(col = 0; col < 128; col++) {
            bar_idx = col / 18; 
            if(bar_idx > 6 || (col % 18) < 2) { 
                oled_data(0x00); 
            } else {
                height_in_pixels = ((unsigned int)day_scores[bar_idx] * 64) / 100;
                col_data = 0;
                for(b = 0; b < 8; b++) {
                    if((page * 8 + b) >= (64 - height_in_pixels)) {
                        col_data |= (1 << b);
                    }
                }
                oled_data(col_data);
            }
        }
    }
    wait_ms(10000); 
    oled_clear(); 
}

// UART //
void uart_init(void) { 
    P1SEL |= BIT1 | BIT2; 
    P1SEL2 |= BIT1 | BIT2; 
    UCA0CTL1 |= UCSSEL_2; 
    UCA0BR0 = 104; 
    UCA0BR1 = 0; 
    UCA0MCTL = UCBRS0; 
    UCA0CTL1 &= ~UCSWRST; 
}
void uart_putc(unsigned char c) { while(!(IFG2 & UCA0TXIFG)); UCA0TXBUF = c; }
void uart_puts(const char *s) { while(*s) uart_putc(*s++); }
void uart_putnum(unsigned int v) { if(v >= 10) uart_putnum(v/10); uart_putc('0' + v%10); }

// PWM //
void pwm_init(void) {
    P1DIR |= LED_PIN; P1SEL |= LED_PIN;
    TA0CCR0 = 1000 - 1; TA0CCTL1 = OUTMOD_7; TA0CCR1 = 0;
    TA0CTL = TASSEL_2 | MC_1;
}
void pwm_set_brightness(unsigned int percent) {
    if (percent > 99) percent = 99;
    TA0CCR1 = percent * 10;
}


void timer1_init(void) {
    TA1CCR0 = 12500;                 
    TA1CCTL0 = CCIE;                 
    TA1CTL = TASSEL_2 | ID_3 | MC_1; 
}

// DHT22 SENSÖRÜ //
unsigned char dht_data[5];
int read_dht(void) {
    unsigned char i, j; unsigned int cnt;
    
    P2DIR |= DHT_PIN;          
    P2OUT &= ~DHT_PIN;         
    wait_ms(20);               
    
    P2DIR &= ~DHT_PIN;         
    
    cnt=0; while( P2IN & DHT_PIN)  { if(++cnt>5000) return -1; }
    cnt=0; while(!(P2IN & DHT_PIN)){ if(++cnt>5000) return -2; }
    cnt=0; while( P2IN & DHT_PIN)  { if(++cnt>5000) return -3; }
    
    for(i=0; i<5; i++){
        dht_data[i]=0;
        for(j=0; j<8; j++){
            cnt=0; while(!(P2IN & DHT_PIN)){ if(++cnt>5000) return -4; }
            cnt=0; while( P2IN & DHT_PIN)  { cnt++; __delay_cycles(2); if(cnt>500) return -5; }
            
            if(cnt > 3) dht_data[i] |= (1<<(7-j));
        }
    }
    
    if((unsigned char)(dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) != dht_data[4]) {
        return -6;
    }
    return 0; 
}

unsigned int adc_read(unsigned char inch) {
    ADC10CTL0 &= ~ENC; ADC10CTL1 = (unsigned int)inch << 12; ADC10AE0 = BIT3 | BIT4;
    ADC10CTL0 = SREF_0 | ADC10SHT_2 | ADC10ON | ENC | ADC10SC;
    while(ADC10CTL1 & ADC10BUSY); return ADC10MEM;
}

// MAIN //
int main(void) {
    unsigned int raw_soil = 0, soil_p = 0, raw_light = 0, light_p = 0, score = 0;
    unsigned int temp_v = 24; 
    
    unsigned char has_warning = 0;
    int dht_status = 0; 

    WDTCTL = WDTPW | WDTHOLD;
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    P2SEL  &= ~ROLE_PIN;
    P2SEL2 &= ~ROLE_PIN;
    P2DIR  |=  ROLE_PIN;
    POMPAYI_DURDUR();

    P2SEL = 0; P2SEL2 = 0;
    uart_init(); 
    pwm_init();
    timer1_init(); 
    wait_ms(1000);

    oled_init();
    oled_clear();

    oled_char(0,0,22); oled_char(0,7,10); oled_char(0,14,11); oled_char(0,21,19);
    oled_char(0,28,16); oled_char(0,35,21); oled_char(0,42,11); oled_char(0,49,19);

    __enable_interrupt(); 

    for(;;) {
        raw_soil = adc_read(4);

        if(raw_soil >= SOIL_DRY)       soil_p = 0;
        else if(raw_soil <= SOIL_WET)  soil_p = 99;
        else soil_p = (unsigned int)((unsigned long)(SOIL_DRY - raw_soil) * 99UL / (SOIL_DRY - SOIL_WET));

        raw_light = adc_read(3);
        if(raw_light > 1000) light_p = 0;
        else light_p = (unsigned int)((1023UL - raw_light) * 99UL / 1023UL);

        if(light_p < 30)      pwm_set_brightness(85);
        else if(light_p < 60) pwm_set_brightness(40);
        else                  pwm_set_brightness(0);

        if(soil_p < 30) {
            POMPAYI_CALISTIR();
        } else {
            POMPAYI_DURDUR();
        }

        wait_ms(2000); 
        dht_status = read_dht();
        if(dht_status == 0) {
            temp_v = (((unsigned int)(dht_data[2]&0x7F))<<8 | dht_data[3]) / 10;
        }

        score = (soil_p * 40 + light_p * 30 + (temp_v > 20 && temp_v < 28 ? 3000 : 1000)) / 100;
        if(score > 99) score = 99;
        
        score_sum += score; 
        sample_in_day++;
        
        if(sample_in_day >= SAMPLES_PER_DAY) {
            day_scores[day_count] = (unsigned char)(score_sum / SAMPLES_PER_DAY);
            day_count++; 
            score_sum = 0; 
            sample_in_day = 0;
            
            if(day_count >= 7) {
                unsigned int total_score = 0;
                unsigned int avg_score = 0;
                unsigned char i;
                
                uart_puts("\r\n========================\r\n");
                uart_puts("   7 GUNLUK RAPOR\r\n");
                uart_puts("========================\r\n");
                for(i=0; i<7; i++) {
                    uart_puts(""); uart_putnum(i+1); uart_puts(". GUN SKORU: ");
                    uart_putnum(day_scores[i]); uart_puts("\r\n");
                    total_score += day_scores[i];
                }
                avg_score = total_score / 7;
                uart_puts("------------------------\r\n");
                uart_puts("ORTALAMA SKOR: "); uart_putnum(avg_score); uart_puts("\r\n");
                uart_puts("========================\r\n\r\n");

                POMPAYI_DURDUR(); 
                draw_graph();
                
                day_count = 0; 
                oled_clear();
                oled_char(0,0,22); oled_char(0,7,10); oled_char(0,14,11); oled_char(0,21,19);
                oled_char(0,28,16); oled_char(0,35,21); oled_char(0,42,11); oled_char(0,49,19);
            }
        }

        //  BLUETOOTH EKRANI //
        uart_puts("--- YENI OLCUM ---\r\n");
        
        
        uart_puts("SISTEM UPTIME: "); uart_putnum(system_uptime); uart_puts(" Saniye\r\n");
        
        uart_puts("ISIK: %"); uart_putnum(light_p); uart_puts("\r\n");
        uart_puts("TOPRAK NEM ORANI: %"); uart_putnum(soil_p); uart_puts("\r\n");
        
        if(dht_status == 0) {
            uart_puts("SICAKLIK: "); uart_putnum(temp_v); uart_puts(" C\r\n");
        } else {
            uart_puts("SICAKLIK: OKUMA HATASI! (Sensore Guc Verin)\r\n");
        }
        
        uart_puts("BUYUME SKORU: "); uart_putnum(score); uart_puts("/99\r\n");
        
        uart_puts("ONERI: ");
        if(soil_p < 30) {
            uart_puts("SULAMA GEREKLI (Pompa Acik)\r\n");
        } else if(light_p < 30) {
            uart_puts("ISIK GEREKLI (LED Acik)\r\n");
        } else if(temp_v < 20) {
            uart_puts("ISITMA GEREKLI (Sicaklik Cok Dusuk)\r\n"); 
        } else if(temp_v > 28) {
            uart_puts("SOGUTMA GEREKLI (Sicaklik Cok Yuksek)\r\n"); 
        } else {
            uart_puts("ORTAM TAM (Sistem Ideal)\r\n");
        }
        uart_puts("\r\n"); 

        oled_clear_line(2);
        oled_clear_line(4);
        has_warning = 0;

        if(soil_p < 30) {
            oled_char(2,0,17); oled_char(2,7,24); oled_char(2,14,21); oled_char(2,21,23); oled_char(2,28,12); oled_char(2,35,23);
            oled_char(2,49,26); oled_char(2,56,11); oled_char(2,63,19); oled_char(2,70,11); oled_char(2,77,18); oled_char(2,84,21); oled_char(2,91,16);
            has_warning = 1;
        }

        if(light_p < 30) {
            unsigned char line = (has_warning == 1) ? 4 : 2;
            oled_char(line,0,16); oled_char(line,7,17); oled_char(line,14,16); oled_char(line,21,18);
            oled_char(line,35,26); oled_char(line,42,11); oled_char(line,49,19); oled_char(line,56,11); oled_char(line,63,18); oled_char(line,70,21); oled_char(line,77,16);
            has_warning = 1;
        }

        if(has_warning == 0) {
            oled_char(2,0,22); oled_char(2,7,19); oled_char(2,14,25); oled_char(2,21,23); oled_char(2,28,12);
            oled_char(2,42,25); oled_char(2,49,23); oled_char(2,56,12);
        }

        wait_ms(MEASURE_INTERVAL);
    }
}


#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0_ISR(void) {
    timer_tick++;           
    if(timer_tick >= 10) {  
        system_uptime++;    
        timer_tick = 0;     
    }
}
