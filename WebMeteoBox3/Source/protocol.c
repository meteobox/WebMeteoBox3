/*
 * WebMeteoBox source code. ver 1.0.0.4
 * Created by Zager V.B. and Krylov A.I. 2012-2016 Dubna (c)
 * 
 * Project home page: http://webmeteobox.ru
 * Email: valery@jinr.ru
 *
 *
 * 11.06.2014 
 *
 */

#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <ctype.h>   
#include <math.h> 
#include <stdlib.h>
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "dhcp_client.h"
#include "protocol.h"
#include "term.h"
#include "DHT.h"
#include "net.h"
#include "dnslkup.h"
#include "i2csoft.h"
#include "task_config.h"

#define COUNT_AUTOUPDATE 8

typedef struct mode_block {
	char             *name;
	unsigned int     id;
}modes;

const static modes update_time[COUNT_AUTOUPDATE] =
{
	/*{"Max",  1},*/	   // Умножать на 100.
	{"10s",  10},
	{"30s",  30},
	{"1m",   60},
	{"3m",   180},
	{"5m",   300},
	{"10m",  600},
	{"15m",  900},
	{"20m",  1200}
};


extern StrConfigParam NetworkParam;
extern Server_config server_data[2];
//extern unsigned char custom_request[100]; 
//extern unsigned char custom_password[40]; //для пользовательского пароля Authorization: Basic 

unsigned char count_ds18b2;
unsigned char count_ds18b2_2;
unsigned long int sessionid; // Сессия соединения. устанавливается при правильном вводе логина и пароля.
unsigned long int usersession;
unsigned long int Time_ms=0L; // global timer
unsigned long int DS18B20refreshTime = 500; // *10ms
unsigned long int WebrefreshTime0 = 600; // 1min
unsigned long int WebrefreshTime1 = 600; // 1min
volatile unsigned long int TimeTs0=0L;
volatile unsigned long int TimeWt0=0L;
volatile unsigned long int TimeWt1=0L;
volatile unsigned char Tr0  = 0;
volatile unsigned char Wt0  = 0; // Logger 1 refresh timer
volatile unsigned char Wt1  = 0; // Logger 2 refresh timer
volatile int second=0;
// global packet buffer
uint8_t buf[BUFFER_SIZE+1];
char xbuf[350]; // Для разбора строки http запроса!
uint8_t * bufrecv;
uint16_t buf_pos=0;
char MPasswd[15]; // Password
char tmpstr[35];
char tmpstr2[35];
//char MLogin[15];  // Login
// --- there should not be any need to changes things below this line ---
#define TRANS_NUM_GWMAC 1
static volatile uint8_t start_web_client=0; 
static uint8_t gwmac[6];
static uint8_t otherside_www_ip[4]; // will be filled by dnslkup
static int8_t dns_state=0;
static int8_t gw_arp_state=0;
static uint8_t web_client_sendok=0;
static volatile uint8_t sec=0;
static volatile uint8_t sec_dhcp=0;
char sec_cnt=0;
unsigned char sensors_send=0;

volatile unsigned int tick_counter=0L; // для дозиметра

#ifdef DOZIMETR	// Дозиметр 08.08.2014
unsigned int geiger_counter=0L;
static volatile uint8_t sec_dozimetr=0;
unsigned int doz_buffer[DOZBUFSIZE+1];
unsigned char doz_index=0;
#endif

/*------------------- barometr bmp085 --------------------*/
 int ac1;
 int ac2;
 int ac3;
 unsigned int ac4;
 unsigned int ac5;
 unsigned int ac6;
 int b1;
 int b2;
 int mb;
 int mc;
 int md;
 int temperature;
 long pressure;
 
 //------------------- udp configuration --------------------------- 05.06.2014 ----------------
 static const uint16_t broadcastport = 0xFFFF;
 static const uint8_t broadcastip[4] = {255,255,255,255};
 static const uint8_t broadcastmac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
//------------------- udp configuration --------------------------- 05.06.2014 ----------------


char LED_READ()
{
	return ((PINE&(1<<6))>0);
} 
 
 void LED_ON_OFF( char ckl )
 {
	 if (ckl == 1) PORTE |= (1<<6);
	 else PORTE &= ~(1<<6);
 }
 
 
 char PA_READ(char pin)
 {
	 return ((PINA&(1<<pin))>0);
 }
 
 char PF_READ(char pin)
 {
	 return ((PINF&(1<<pin))>0);
 }
 
 void PA_WRITE( char pin, char val )
 {
	 if (val == 1) PORTA |= (1<<pin);
	 else PORTA &= ~(1<<pin);
 }

 void PF_WRITE( char pin, char val )
 {
	 if (val == 1) PORTF |= (1<<pin);
	 else PORTF &= ~(1<<pin);
 }
 
 void leddebugsignal (char a) //Мигалка для отладки
 {
	 char b;
	 for (b=0; b<a; b++)
	 {
		 LED_ON_OFF(1);
		 my_wait_1ms(50);
		 LED_ON_OFF(0);
		 my_wait_1ms(50);
		 asm("WDR");
	 }
 }
 
 void bmp085_read_temperature_and_pressure(int* temperature, long int* pressure) {
	 long ut;
	 long up;
	 long x1, x2, x3, b3, b5, b6, p;
	 unsigned long b4, b7;
	 char oversampling_setting = 3;
	 
	 //cli();
	 ut = bmp085_read_ut();
	 
	 bmp085_read_up(&up);
	 //sei();
	 
	 //ut=27898; up=23843;

	 //calculate the temperature
	 x1 = ((long int)ut - ac6) * ac5 >> 15;
	 x2 = ((long int) mc << 11) / (x1 + md);
	 b5 = x1 + x2;
	 *temperature = (b5 + 8) >> 4;

	 //calculate the pressure
	 b6 = b5 - 4000;
	 x1 = (b2 * (b6 * b6 >> 12)) >> 11;
	 x2 = ac2 * b6 >> 11;
	 x3 = x1 + x2;

	 //b3 = (((int32_t) ac1 * 4 + x3)<> 2;

	 //if (oversampling_setting == 3)
	 b3 = ((unsigned long int) ac1 * 4 + x3 + 2) << 1;
	 //if (oversampling_setting == 2) b3 = ((int32_t) ac1 * 4 + x3 + 2);
	 //if (oversampling_setting == 1) b3 = ((int32_t) ac1 * 4 + x3 + 2) >> 1;
	 //if (oversampling_setting == 0) b3 = ((int32_t) ac1 * 4 + x3 + 2) >> 2;

	 
	 x1 = ac3 * b6 >> 13;
	 x2 = (b1 * (b6 * b6 >> 12)) >> 16;
	 x3 = ((x1 + x2) + 2) >> 2;
	 b4 = (ac4 * (unsigned long int) (x3 + 32768)) >> 15;
	 b7 = ((unsigned long int) up - b3) * (50000 >> oversampling_setting);
	 p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;

	 x1 = (p >> 8) * (p >> 8);
	 x1 = (x1 * 3038) >> 16;
	 x2 = (-7357 * p) >> 16;
	 *pressure = p + ((x1 + x2 + 3791) >> 4);

 }
 /*------------------- barometr bmp085 --------------------*/
 
 unsigned int read_adc_int(unsigned char adc_input)
 {
	// float adc,bar;
	 ADMUX=adc_input | (ADC_REF_AVCC & 0xff);
	 // Start the AD conversion
	 ADCSRA|=0x40;
	 // Wait for the AD conversion to complete
	 while ((ADCSRA & 0x10)==0);
	 ADCSRA|=0x10;

	 return ADCW;
 }
 
 float read_current()
 {
	 unsigned int i,a;
	 float k=0,l=0;
	 
	 a=read_adc_int(6);
	 
	 // for(i=0; read_ADC(1)<10 && i<100; i++)
	 // continue;

	 for (i=0; i<400; i++)
	 {
		 k+= abs(read_adc_int(0)-a);
		 _delay_us(25);
	 }
	 //return k/256;
	 k/= 400;
	 l=k/1024.0*VREF*30;
	 //l*= .10;	// LSB for 30A range
	 return 220*l;   // return WATT 220Vrms x Irms
	 //return l;
 }

 float read_current_sqrt(char adc_num, int coef) /* вычисление среднеквадратичного */
 {
	 unsigned int i,a;
	 float k=0,l=0,q=0;
	 
	 a=read_adc_int(6);
	 
	 // for(i=0; read_ADC(1)<10 && i<100; i++)
	 // continue;

	 for (i=0; i<400; i++)
	 {
		 q=abs(read_adc_int(adc_num)-a);
		 
		 
		 k += (q*q);
		 
		 _delay_us(25);
	 }
	 //return k/256;
	 k/=400;
	 q=sqrt(k);
	 
	 //l=q/1024.0*VREF*1000;
	 //return l;
	 
	 l=q/1024.0*VREF*30;
	 //l*= .10;	// LSB for 30A range
	 //return 220*l;   // return WATT 220Vrms x Irms
	 return coef*l;   // return WATT 220Vrms x Irms
	 //return l;
 }


 float read_current_volt(char adc_num)
 {
	 unsigned int i,a;
	 float k=0,l=0,q=0;
	 
	 a=read_adc_int(6);
	 
	 // for(i=0; read_ADC(1)<10 && i<100; i++)
	 // continue;

	 for (i=0; i<400; i++)
	 {
		 q=abs(read_adc_int(adc_num)-a);
		 
		 
		 k += (q*q);
		 
		 _delay_us(25); // 25
	 }
	 //return k/256;
	 k/=400;
	 q=sqrt(k);
	 
	 //l=q/1024.0*VREF*1000;
	 //return l;
	 
	 l=q/1024.0*VREF;
	 //l*= .10;	// LSB for 30A range
	 //return 411.4*l;   // return WATT 220Vrms x Irms
	 return l;
 }


// the __attribute__((unused)) is a gcc compiler directive to avoid warnings about unsed variables.
void browserresult_callback(uint16_t webstatuscode,uint16_t datapos, uint16_t len __attribute__((unused))){
	if (webstatuscode==200){
		web_client_sendok++;
		//printf("Apache OK %d\r\n",web_client_sendok);
	}
	// clear pending state at sucessful contact with the
	// web server even if account is expired:
	//printf("callback start_web_client=%d\r\n",start_web_client);
	if (start_web_client==2) start_web_client=3;
}

// the __attribute__((unused)) is a gcc compiler directive to avoid warnings about unsed variables.
void arpresolver_result_callback(uint8_t *ip __attribute__((unused)),uint8_t transaction_number,uint8_t *mac){
	uint8_t i=0;
	if (transaction_number==TRANS_NUM_GWMAC){
		// copy mac address over:
		while(i<6){gwmac[i]=mac[i];i++;}
	}
}


ISR(INT2_vect){
	tick_counter++;
	//leddebugsignal(1);
}



ISR(TIMER0_COMP_vect){
Time_ms += 10; 
	sec_cnt++;
	if (sec_cnt>=100)
	{

#ifdef DOZIMETR	// Дозиметр 08.08.2014
	sec_dozimetr++;
	if (sec_dozimetr>60)
	{
		sec_dozimetr=0;	
		geiger_counter=tick_counter;
	
		doz_buffer[doz_index]=tick_counter;
		doz_index++;
		if (doz_index>DOZBUFSIZE) doz_index=0;
	
		tick_counter=0;
	}
#endif		
		
		sec_cnt=0;
		sec++;
		sec_dhcp++;
	    if (sec_dhcp>5){
		    sec_dhcp=0;
		    dhcp_6sec_tick();
	    }
	}
	
 if (Wt0 == 1){
	 if( TimeWt0 > 0) --TimeWt0;
	 else { Wt0 = 0; }
 }
 
 if (Wt1 == 1){
	 if( TimeWt1 > 0) --TimeWt1;
	 else { Wt1 = 0; }
 }
 
 
 if (Tr0 == 1){
	 if( TimeTs0 > 0) --TimeTs0;
	 else { Tr0 = 0; }
 }

}


#ifdef DOZIMETR	// Дозиметр
float calculate_doz(void)
{
	unsigned char a;
	float tmp=0;

	
	for (a=0; a<DOZBUFSIZE; a++) tmp+=doz_buffer[a];
	
	tmp=(tmp/DOZBUFSIZE)*CONV_FACTOR;
	
	//if (tmp<0.05) tmp = 0.05;
	//if (tmp>0.29) tmp = 0.29;
	
	return (tmp); // Среднее CPM от дозиметра
}
#endif


unsigned char D2C(
char c	/**< is a Hex(0x00~0x0F) to convert to a character */
)
{
	uint16_t t = (uint16_t) c;
	if (t >= 0 && t <= 9)
	return '0' + c;
	if (t >= 10 && t <= 15)
	return 'A' + c - 10;

	return c;
}

char C2D(
char c	/**< is a character('0'-'F') to convert to HEX */
)
{
	if (c >= '0' && c <= '9')
	return c - '0';
	if (c >= 'a' && c <= 'f')
	return 10 + c -'a';
	if (c >= 'A' && c <= 'F')
	return 10 + c -'A';

	return (char)c;
}

unsigned char h2int(char c)
{
	if (c >= '0' && c <='9'){
		return((unsigned char)c - '0');
	}
	if (c >= 'a' && c <='f'){
		return((unsigned char)c - 'a' + 10);
	}
	if (c >= 'A' && c <='F'){
		return((unsigned char)c - 'A' + 10);
	}
	return(0);
}

char* itoa_long(
unsigned long int value,	/**< is a integer value to be converted */
char* str,	/**< is a pointer to string to be returned */
uint16_t base	/**< is a base value (must be in the range 2 - 16) */
)
{
	ultoa(value,str,base); // add 15.08.2014
	return(str);
}

unsigned int ATOI(char* str, int base)
{
	int num = 0;

	while (*str != 0) num = num * base + C2D(*str++);

	return num;
}


void data_get(char *x, char delimiter, char base)
{
	int a,b,n=0,j=0;
	char buf[10];

	b = strlen(x);
	for (a=0; a<b; a++)
	{
		if (*(x+a) == delimiter) // if (*(x+a) == '.')
		{
			buf[j++] = '\0';
			xbuf[n++] = (char)ATOI(buf,base);
			j=0;
		}
		else buf[j++] = *(x+a);
	}
	
	buf[j++] = '\0';
	xbuf[n++] = ATOI(buf,base);
} // data_get

/*

char * findstr(char * s, char * find, char * stop)
{
	int len,len2;
	char *x2;	
	
	len = strlen(find);
	x2=strstr(s,find);
	if (x2==NULL) { xbuf[0]=0x00; return(xbuf); }
	
	strcpy(xbuf,&x2[len]);
	len = strlen(xbuf);
	
	x2=strstr(xbuf,stop);
	len2 = strlen(x2);
	len = len-len2;
	
	xbuf[len] = '\0';
	s = xbuf;
	
	return s; 
}*/


char * findstr_P(char * s, const char * find, const char * stop) // Храним константы в памяти программ 18.09.2014
{
	int len,len2;
	char *x2;
	
	len = strlen_P(find);
	x2=strstr_P(s,find);
	if (x2==NULL) { xbuf[0]=0x00; return(xbuf); }
	
	strcpy(xbuf,&x2[len]);
	len = strlen(xbuf);
	
	x2=strstr_P(xbuf,stop);
	len2 = strlen(x2);
	len = len-len2;
	
	xbuf[len] = '\0';
	s = xbuf;
	
	return s;
}



void urldecode(char *urlbuf)
{
	char c;
	char *dst;
	dst=urlbuf;
	while ((c = *urlbuf)) {
		if (c == '+') c = ' ';
		if (c == '%') {
			urlbuf++;
			c = *urlbuf;
			urlbuf++;
			c = (h2int(c) << 4) | h2int(*urlbuf);
		}
		*dst = c;
		dst++;
		urlbuf++;
	}
	*dst = '\0';
}


void exe_relay() // Работа с Реле
{
	unsigned char a,b;
	for (a=0; a<MAX_RELAY; a++)
	if (all_relay[a].flag != 0)
	{
		for (b=0; b<count_sensors; b++)
		{
			if (all_sensors[b].enable==0xFF)
			{
				if (memcmp(all_sensors[b].id,all_relay[a].id,8)==0)
				{
					//LED_ON_OFF(!LED_READ()); my_wait_1ms(100);
					if (a<=3) 
					{
						if ((all_sensors[b].value+all_sensors[b].offset)<all_relay[a].min) PA_WRITE(all_relay[a].pio,0);
						if ((all_sensors[b].value+all_sensors[b].offset)>all_relay[a].max) PA_WRITE(all_relay[a].pio,1);
					}
					else
					{
						if ((all_sensors[b].value+all_sensors[b].offset)<all_relay[a].min) PF_WRITE(all_relay[a].pio,0);
						if ((all_sensors[b].value+all_sensors[b].offset)>all_relay[a].max) PF_WRITE(all_relay[a].pio,1);												
					}

				}
			}
		}
	}
}



void exe_sensors(char flag) // Опрашиваем все сенсоры и обновляем массив all_sensors 28.12.2012
{
unsigned char a;
unsigned char cnt=0; // счетчик датчиков

	if (!Tr0 || flag==1) { // flag == 1 чтобы принудительно обновить показания сенсоров
		
		LED_ON_OFF(!LED_READ());
		
		DS18B20refreshTime=500; //опрашиваем датчики 1 раз в 5 секунд!
		TimeTs0=DS18B20refreshTime;		
		
#ifdef SCT_013_030
			asm("WDR");
			all_sensors[cnt].type=4; // 4 - Мощность Ватт
			all_sensors[cnt].value=read_current_sqrt(0,1); // read_current_sqrt
			
//all_sensors[cnt].type=10; // 4 - Мощность Ватт
//all_sensors[cnt].value=ut; // read_current_sqrt
			
			all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
			all_sensors[cnt].id[1]=0x0E;
			memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
			cnt++;		

			asm("WDR");
			all_sensors[cnt].type=4; // 4 - Мощность Ватт
			all_sensors[cnt].value=read_current_sqrt(1,1); // read_current_sqrt
			
//all_sensors[cnt].type=10; // 4 - Мощность Ватт
//all_sensors[cnt].value=up; // read_current_sqrt			
			
			all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
			all_sensors[cnt].id[1]=0x1E;
			memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
			cnt++;	

			asm("WDR");
			all_sensors[cnt].type=4; // 4 - Мощность Ватт
			all_sensors[cnt].value=read_current_sqrt(2,1); // read_current_sqrt
			all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
			all_sensors[cnt].id[1]=0x2E;
			memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
			cnt++;		


#ifdef SCT_VIRTUAL_SENSOR	// Суммируем можность всех 3х токовых клещей
			asm("WDR");
			all_sensors[cnt].type=4; // 4 - Мощность Ватт
			all_sensors[cnt].value=0;
		
			// Суммируем значения включенных токовых клещей 03.02.2014
			if (all_sensors[cnt-1].enable==0xFF) all_sensors[cnt].value=all_sensors[cnt-1].value;
			if (all_sensors[cnt-2].enable==0xFF) all_sensors[cnt].value+=all_sensors[cnt-2].value;
			if (all_sensors[cnt-3].enable==0xFF) all_sensors[cnt].value+=all_sensors[cnt-3].value;

			all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
			all_sensors[cnt].id[1]=0x3E;
			memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
			cnt++;	
#endif	
#endif				
		
#ifdef DOZIMETR	// Дозиметр
	/*
	SBM-20 0.0057
	SBM-19 0.0021
	SI-29BG 0.0082
	SI-180G 0.0031
	LND-712 0.0081
	LND-7317 0.0024
	J305 0.0081
	SBT11-A 0.0031
	SBT-9 0.0117
	SBT-10 0.0013
	*/
	asm("WDR");
	all_sensors[cnt].type=6; // ДОЗА в uSv/hr  28.02.2014
	all_sensors[cnt].value=calculate_doz();


	all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
	all_sensors[cnt].id[1]=0x77;
	memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
	cnt++;
#endif		
		
		
		
		
		if (ac1 != -1 && ac2 != -1 && ac3 != -1)
		{
			bmp085_read_temperature_and_pressure(&temperature,&pressure);
			all_sensors[cnt].type=3; // 3 - Давление
			all_sensors[cnt].value=(pressure/133.32); // Преобразуем Паскали в Мм
			all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
			all_sensors[cnt].id[1]=0x0D;
			memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
			//printf("pressure %f temperature=%d\r\n",(pressure/133.32),temperature);
			//printf("%d %d %d %d %d\r\n",b1,b2,mb,mc,md);
			//printf("bmp085_read_ut %x\r\n",bmp085_read_ut());
			cnt++;
		}
		
		//printf("count_ds18b2=%d\r\ncount_ds18b2_2=%d\r\n",count_ds18b2,count_ds18b2_2);		
		
		if (count_ds18b2==0) // На 2 порту вместо ds18b20 подключен датчик AM3202 10.10.2013
		{
			ReadDHT(DHTPIN2);
			
			//printf("DHTPIN2=%d\r\n",bGlobalErr);
		
			if (bGlobalErr==0) {// Есть данные с AM3202 - датчик  влажности и температуры
				// 1 датчик влажности
				all_sensors[cnt].type=2; // 2 - влажность
				all_sensors[cnt].value=(dht_hum/10.0);			
				all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
				all_sensors[cnt].id[1]=0x0B;
				memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
				cnt++;			
				// 2 датчик темературы
				all_sensors[cnt].type=1; // 1 - температура
				all_sensors[cnt].value=(dht_term/10.0);
				all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
				all_sensors[cnt].id[1]=0x0C;
				memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
				cnt++;
			}
		}
		
		if (count_ds18b2_2==0) // На 2 порту вместо ds18b20 подключен датчик AM3202 10.10.2013
		{
			ReadDHT(DHTPIN);
			//printf("DHTPIN=%d\r\n",bGlobalErr);
			
			if (bGlobalErr==0) {// Есть данные с AM3202 - датчик  влажности и температуры
				// 2 датчик влажности
				all_sensors[cnt].type=2; // 3 - влажность
				all_sensors[cnt].value=(dht_hum/10.0);			
				all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
				all_sensors[cnt].id[1]=0x1B;
				memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
				cnt++;
				// 2 датчик темературы
				all_sensors[cnt].type=1; // 4 - температура
				all_sensors[cnt].value=(dht_term/10.0);
				all_sensors[cnt].id[0]=0xAA; // Создаём уникальный ID на базе mac адреса
				all_sensors[cnt].id[1]=0x1C;
				memcpy(&all_sensors[cnt].id[2],NetworkParam.mac,6);
				cnt++;
			}
		}				
		

		if (count_ds18b2>0) // Если есть датчики температуры DS18B20
		{					
			for (a=0; a<count_ds18b2; a++) { 
				asm("WDR");
				if (DS18B20_ReadTemperature_Fast_Float(OWI_PIN_5, allDevices[a].id,&all_sensors[cnt].value) == READ_SUCCESSFUL)
				{
					all_sensors[cnt].type=1; // 1 - температура
					//printf("-%02x%02x\r\n",allDevices[count_ds18b2+a].id[0],allDevices[count_ds18b2+a].id[1]);
					memcpy(&all_sensors[cnt].id,allDevices[a].id,8);
					cnt++;
				}
			}
			Start_18b20_Convert(OWI_PIN_5);	
		}
		
//---------------------------------------------------------
		if (count_ds18b2_2>0) // Если есть датчики температуры DS18B20
		{
			for (a=0; a<count_ds18b2_2; a++) {
				asm("WDR");
				//printf("%02x\r\n",allDevices[count_ds18b2+a].id[0]);
				if (DS18B20_ReadTemperature_Fast_Float(OWI_PIN_4, allDevices[count_ds18b2+a].id,&all_sensors[cnt].value) == READ_SUCCESSFUL)
				{
					all_sensors[cnt].type=1; // 1 - температура
					memcpy(&all_sensors[cnt].id,allDevices[count_ds18b2+a].id,8);
					cnt++;
				}
			}
			Start_18b20_Convert(OWI_PIN_4);
		}		

		//cnt+=6;
		count_sensors=cnt;
		Tr0=1;
	}

}


uint16_t http200ok(void)
{
     return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t http404(void)
{
	return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 Not Found</h1>")));
}

uint16_t page_header(uint8_t *data,uint16_t pl)
{
	//return(fill_tcp_data_p(data,pl,PSTR("<html><head><title>WebMeteoBox3</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1251\" /></head><body>")));
	return(fill_tcp_data_p(data,pl,PSTR("<html><head><meta charset=\"windows-1251\"/></head><body>")));	
	//return((uint8_t *)data);
}


uint16_t get_sensor_value_type_str(unsigned char num,float offset, char *buf1,char *buf2) // buf1=value. buf2=char comment
{
	switch (all_sensors[num].type) {
		case 1:   sprintf(buf1,"%2.2f",all_sensors[num].value+offset); strcpy_P(buf2,PSTR("&deg;C"));
			break;
		case 2:   sprintf(buf1,"%2.1f",all_sensors[num].value+offset); strcpy_P(buf2,PSTR("%"));		
			break;
		case 3:   sprintf(buf1,"%3.1f",all_sensors[num].value+offset); strcpy_P(buf2,PSTR("mm"));
			break;
		case 4:   if (offset==0.00) { // Для токовых клещей
				sprintf(buf1,"%3.1f",all_sensors[num].value); 
				strcpy_P(buf2,PSTR("A"));
				}
				else { sprintf(buf1,"%3.1f",all_sensors[num].value*offset); strcpy_P(buf2,PSTR("W")); }
			break;
		case 5:   sprintf(buf1,"%3.3f",all_sensors[num].value*offset); strcpy_P(buf2,PSTR("V"));
			break;
		case 6:   sprintf(buf1,"%1.2f",all_sensors[num].value+offset); strcpy_P(buf2,PSTR("uSv/hr"));
			break;
		case 10:  sprintf(buf1,"%x",(int)all_sensors[num].value); strcpy(buf2,""); //sprintf(buf1,"%.0f",all_sensors[num].value); strcpy(buf2,"");
			break;
	}

/*
	if (all_sensors[a].type==1) { offset=((all_sensors[a].value)); sprintf(strmac,"%2.2f",offset); strcat(str,strmac); strcat(str,"&deg;C"); }
	if (all_sensors[a].type==2) { offset=((all_sensors[a].value)); sprintf(strmac,"%2.1f",offset); strcat(str,strmac); strcat(str,"%"); }
	if (all_sensors[a].type==3) { offset=(all_sensors[a].value); sprintf(strmac,"%3.1f",offset); strcat(str,strmac); strcat(str,"mm"); }
	if (all_sensors[a].type==4) { offset=(all_sensors[a].value); sprintf(strmac,"%3.1f",offset); strcat(str,strmac); strcat(str,"W"); }
	if (all_sensors[a].type==5) { offset=(all_sensors[a].value); sprintf(strmac,"%3.3f",offset); strcat(str,strmac); strcat(str,"V"); }
	if (all_sensors[a].type==6) { offset=(all_sensors[a].value); sprintf(strmac,"%1.2f",offset); strcat(str,strmac); strcat(str,"uSv/hr"); }
	if (all_sensors[a].type==10){ sprintf(strmac,"%.0f",all_sensors[a].value); strcat(str,strmac); }
*/
return(0);		
}


uint16_t print_webpage_mainframe(uint8_t *buf)
{
	uint16_t pl;
	pl=http200ok();
	itoa_long(sessionid,tmpstr,10);
	pl=fill_tcp_data_p(buf,pl,PSTR("<html><frameset cols=\"170,*\"><frameset rows=\"260, *\" scrolling=\"no\">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<frame src=\"/menu?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\" name=\"mf\">")); // menu frame
	pl=fill_tcp_data_p(buf,pl,PSTR("<frame src=\"/dev?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\" name=\"lf\">"));	// sensor frame
	pl=fill_tcp_data_p(buf,pl,PSTR("</frameset><frame src=\"/info?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);	
	pl=fill_tcp_data_p(buf,pl,PSTR("\" name=\"rf\">")); 
	pl=fill_tcp_data_p(buf,pl,PSTR("</frameset></html>"));
	return(pl);
}

uint16_t print_webpage_tcpip_settings(uint8_t *buf)
{
	uint16_t pl;
	pl=http200ok();
	pl=page_header(buf,pl);

	pl=fill_tcp_data_p(buf,pl,PSTR("<center><br><br>TCP/IP Settings<br><form method=\"get\" action=\"\">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<center><table border=0 cellspacing=0 cellpadding=10><tr><td align=center width=170>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<B>IP<br><input type=text name=i maxlength=25 value=\""));
	sprintf_P(tmpstr,PSTR("%d.%d.%d.%d"),NetworkParam.ip[0],NetworkParam.ip[1],NetworkParam.ip[2],NetworkParam.ip[3]); // делаем строку с IP
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("MAC address<br><input type=text name=m maxlength=25 value=\""));
	sprintf_P(tmpstr,PSTR("%02x:%02x:%02x:%02x:%02x:%02x"),NetworkParam.mac[0],NetworkParam.mac[1],NetworkParam.mac[2],NetworkParam.mac[3],NetworkParam.mac[4],NetworkParam.mac[5]); // делаем строку с MAC
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("Net mask<br><input type=text name=s maxlength=25 value=\""));	
	sprintf_P(tmpstr,PSTR("%d.%d.%d.%d"),NetworkParam.subnet[0],NetworkParam.subnet[1],NetworkParam.subnet[2],NetworkParam.subnet[3]); // делаем строку с MASK
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br>"));	
	pl=fill_tcp_data_p(buf,pl,PSTR("Gateway<br><input type=text name=g maxlength=25 value=\""));	
	sprintf_P(tmpstr,PSTR("%d.%d.%d.%d"),NetworkParam.gw[0],NetworkParam.gw[1],NetworkParam.gw[2],NetworkParam.gw[3]); // делаем строку с Gateway	
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br>"));		
	pl=fill_tcp_data_p(buf,pl,PSTR("DNS server<br><input type=text name=d maxlength=25 value=\""));
	sprintf_P(tmpstr,PSTR("%d.%d.%d.%d"),NetworkParam.dns[0],NetworkParam.dns[1],NetworkParam.dns[2],NetworkParam.dns[3]); // делаем строку с DNS
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br>"));	
	
	pl=fill_tcp_data_p(buf,pl,PSTR("<input type=checkbox name=dh"));
	if (NetworkParam.dhcp==0xFF) pl=fill_tcp_data_p(buf,pl,PSTR(" checked"));
	pl=fill_tcp_data_p(buf,pl,PSTR("><b>DHCP Enable/Disabe</b><br><br>"));
	
	pl=fill_tcp_data_p(buf,pl,PSTR("<input type=hidden name=sid value=\""));	
	itoa_long(sessionid,tmpstr,10); 
	pl=fill_tcp_data(buf,pl,tmpstr); // Вставляем сессию
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><input type=\"submit\" name=\"act\" value=\"Save & Reboot\"></form></center></body></html>"));		
	return(pl);
}


uint16_t print_relay_settings(uint8_t *buf, unsigned char num)
{
	uint16_t pl,a;
	pl=http200ok();
	pl=page_header(buf,pl);

	pl=fill_tcp_data_p(buf,pl,PSTR("<center><br><br>Relay "));
	sprintf(tmpstr,"%d",num+1);
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(" Settings<br><form method=get action=\"\">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("State "));	
	pl=fill_tcp_data_p(buf,pl,PSTR("<font style=\"background-color:"));
	if (num<=3) a=PA_READ(all_relay[num].pio); else a=PF_READ(all_relay[num].pio);
    if (a==0) { pl=fill_tcp_data_p(buf,pl,PSTR("#00FF00;")); strcpy(tmpstr,"ON"); } else { pl=fill_tcp_data_p(buf,pl,PSTR("#FF0000;")); strcpy(tmpstr,"OFF"); }	
	pl=fill_tcp_data_p(buf,pl,PSTR("font-size:12px;\"><b>&nbsp;&nbsp;"));	
    pl=fill_tcp_data(buf,pl,tmpstr); 
	pl=fill_tcp_data_p(buf,pl,PSTR("&nbsp;&nbsp;</b></font><input type=submit name=snd value=\"On/Off\">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<input type=hidden name=num value="));
	itoa_long(num,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr); 	
	pl=fill_tcp_data_p(buf,pl,PSTR("><input type=hidden name=sid value="));
	itoa_long(sessionid,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr); // Вставляем сессию
	pl=fill_tcp_data_p(buf,pl,PSTR(">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<br><br>MIN:<input type=text maxlength=5 size=5 name=min value=\""));
	sprintf_P(tmpstr,PSTR("%2.2f"),all_relay[num].min);
	pl=fill_tcp_data(buf,pl,tmpstr); 
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br>MAX:<input type=text maxlength=5 size=5 name=max value=\""));		
	sprintf_P(tmpstr,PSTR("%2.2f"),all_relay[num].max);
	pl=fill_tcp_data(buf,pl,tmpstr);	
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><select name=sen>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<option value=100>Not used</option>"));
	for (a=0; a<count_sensors; a++)
	{
		if (all_sensors[a].enable==0xFF)
		{
			pl=fill_tcp_data_p(buf,pl,PSTR("<option value="));
			sprintf(tmpstr,"%d",a);
			pl=fill_tcp_data(buf,pl,tmpstr);
			
			if (memcmp(all_sensors[a].id,all_relay[num].id,8)==0) pl=fill_tcp_data_p(buf,pl,PSTR(" selected"));
			
			pl=fill_tcp_data_p(buf,pl,PSTR(">"));
			pl=fill_tcp_data(buf,pl,(char *)all_sensors[a].name);
			pl=fill_tcp_data_p(buf,pl,PSTR("</option>"));
		}
	}
	pl=fill_tcp_data_p(buf,pl,PSTR("</select><br><input type=submit name=snd value=\"Save\">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("</form>"));
		
	return(pl);
}

char *replace_str(char *str, char *orig, char *rep)
{
	static char buffer[110];
	char *p;
	
	if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
	return str;
	
	strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
	buffer[p-str] = '\0';
	
	sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));
	
	return buffer;
}

uint16_t print_webpage_logger_settings(uint8_t *buf, unsigned char num)
{
	uint16_t pl,a;
	pl=http200ok();

	pl=fill_tcp_data_p(buf,pl,PSTR("<body><center><br><br>Logger "));
	sprintf(tmpstr,"%d",num+1); 
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(" Settings<br><form method=\"get\" action=\"\">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<center><table border=0 cellspacing=0 cellpadding=10><tr><td align=center width=250>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<input type=checkbox name=en"));
	if (server_data[num].enable==0xFF) pl=fill_tcp_data_p(buf,pl,PSTR(" checked"));
	pl=fill_tcp_data_p(buf,pl,PSTR("><b>Enable/Disabe</b><br><br>"));
	
	pl=fill_tcp_data_p(buf,pl,PSTR("<b>Host name</b><br><input type=text name=host maxlength=28 value=\""));
	pl=fill_tcp_data(buf,pl,(char *)server_data[num].server_name);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br><b>Port</b><br><input type=number name=port maxlength=5 max=65535 size=5 value=\""));
	sprintf(tmpstr,"%u",server_data[num].port);
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br><b>Path to script</b><input type=text maxlength=53 SIZE=40 NAME=path value=\""));
	pl=fill_tcp_data(buf,pl,(char *)server_data[num].script_path);
	
//----------------------------------------------------------------------------------------
#ifdef ENABLE_CUSTOM_REQUEST
if (num==1)	
{
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br><b><a href=/req?sid="));
	itoa_long(sessionid,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr); // Вставляем сессию
	pl=fill_tcp_data_p(buf,pl,PSTR(">Custom Request</a><br><br>"));
}
else
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br>"));
#else	
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br>"));
#endif	
//----------------------------------------------------------------------------------------		
	
	pl=fill_tcp_data_p(buf,pl,PSTR("<b>Update time</b><br><SELECT name=sp>"));
	
	for (a=0; a<COUNT_AUTOUPDATE; a++) // Массив speed
	{
		pl=fill_tcp_data_p(buf,pl,PSTR("<option value="));
		sprintf(tmpstr,"%d",a); 
		pl=fill_tcp_data(buf,pl,tmpstr);
		
		if (server_data[num].refresh_time==update_time[a].id) pl=fill_tcp_data_p(buf,pl,PSTR(" selected"));
		
		pl=fill_tcp_data_p(buf,pl,PSTR(">"));
		pl=fill_tcp_data(buf,pl,update_time[a].name);
		pl=fill_tcp_data_p(buf,pl,PSTR("</option>\n"));
	}

	pl=fill_tcp_data_p(buf,pl,PSTR("</SELECT>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<br><br><input type=\"submit\" name=\"ls"));
	sprintf(tmpstr,"%d",num); 
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\" value=\"Save & Test\">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<input type=hidden name=sid value=\""));
	itoa_long(sessionid,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr); // Вставляем сессию
	pl=fill_tcp_data_p(buf,pl,PSTR("\"></form></center></body></html>"));
	return(pl);
}

/*
uint16_t print_custom_request(uint8_t *buf)
{
	uint16_t pl,a;
	char tmpbuf[10];
	pl=http200ok();
	pl=page_header(buf,pl);
	pl=fill_tcp_data_p(buf,pl,PSTR("<br><br>Custom Request Settings<br><br><form method=\"get\" action=\"\">"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<b>base64 encoded password </b><input type=text maxlength=39 SIZE=39 NAME=aut value=\""));
	pl=fill_tcp_data(buf,pl,(char *)custom_password);
	pl=fill_tcp_data_p(buf,pl,PSTR("\">"));	
	pl=fill_tcp_data_p(buf,pl,PSTR("<br><br><b>Request </b><input type=text maxlength=99 SIZE=70 NAME=cus value=\""));
	pl=fill_tcp_data(buf,pl,(char *)custom_request);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br>"));

	strcpy(xbuf,(char *)custom_request);
	
	sprintf_P(tmpstr,PSTR("%02X%02X%02X%02X%02X%02X"),NetworkParam.mac[0],NetworkParam.mac[1],NetworkParam.mac[2],NetworkParam.mac[3],NetworkParam.mac[4],NetworkParam.mac[5]); // делаем строку с MAC

	strcpy(xbuf,replace_str(xbuf,"{id}", tmpstr));
	
	for (a=0; a<count_sensors; a++) {
		sprintf_P(tmpstr,PSTR("{n%d}"),a+1);
		strcpy(xbuf,replace_str(xbuf,tmpstr, (char *)all_sensors[a].name));
		
		sprintf_P(tmpstr,PSTR("{v%d}"),a+1);
		get_sensor_value_type_str(a,all_sensors[a].offset,tmpbuf,tmpstr2);
		strcpy(xbuf,replace_str(xbuf,tmpstr, tmpbuf));
		
		sprintf_P(tmpstr,PSTR("{i%d}"),a+1);
		sprintf_P(tmpstr2,PSTR("%02X%02X%02X%02X%02X%02X%02X%02X"),all_sensors[a].id[0],all_sensors[a].id[1],all_sensors[a].id[2],all_sensors[a].id[3],all_sensors[a].id[4],all_sensors[a].id[5],all_sensors[a].id[6],all_sensors[a].id[7]); // делаем строку с Sensor id
		strcpy(xbuf,replace_str(xbuf,tmpstr, tmpstr2));
	}
	
	pl=fill_tcp_data_p(buf,pl,PSTR("<input type=hidden name=sid value=\""));
	itoa_long(sessionid,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr); // Вставляем сессию
	
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><nobr>"));
	pl=fill_tcp_data(buf,pl,xbuf);
	pl=fill_tcp_data_p(buf,pl,PSTR("</nobr><br><br><input type=submit name=ss value=\"Save\"></form></center></body></html>"));
	
	return(pl);
}
*/


uint16_t print_webpage_login(uint8_t *buf)
{
	uint16_t pl;
	pl=http200ok();
	pl=page_header(buf,pl);
	//itoa_long(Time_ms,tmpstr,10);
	//pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("<center><br><br>Device settings<br>Password:<form><input type=password name=pa><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<input type=submit value=Login name=in>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("</html>"));
	return(pl);
}


uint16_t print_webpage_parol_change(uint8_t *buf, char flag)
{
	uint16_t pl;
	pl=http200ok();
	pl=page_header(buf,pl);
	pl=fill_tcp_data_p(buf,pl,PSTR("<center><br><br>Change Password<br><br><b>"));
	if (flag==0)
	{		
		pl=fill_tcp_data_p(buf,pl,PSTR("Enter new password:</b><form><input type=text name=new maxlength=14>"));
		pl=fill_tcp_data_p(buf,pl,PSTR("<br>(minimum password length 5 characters)"));		
		pl=fill_tcp_data_p(buf,pl,PSTR("<br><br><input type=submit value=\"Change password\" name=cha>"));
		pl=fill_tcp_data_p(buf,pl,PSTR("<input type=hidden name=sid value=\""));
		itoa_long(sessionid,tmpstr,10);
		pl=fill_tcp_data(buf,pl,tmpstr); // Вставляем сессию
		pl=fill_tcp_data_p(buf,pl,PSTR("\"></form>"));
	}
	else
	{
		pl=fill_tcp_data_p(buf,pl,PSTR("<br><br><b>Password Successfully Changed!</b>"));
	}
	pl=fill_tcp_data_p(buf,pl,PSTR("</html>"));
	return(pl);
}


uint16_t print_webpage_info(uint8_t *buf)
{
	uint16_t pl;
	pl=http200ok();
	pl=page_header(buf,pl);
	pl=fill_tcp_data_p(buf,pl,PSTR("<center><br><br><br><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<h1>&#9728; <u>WebMeteoBox3</u> &#9730;</h1>"));
#ifdef METEOBOX_DEMO_MODE	
	pl=fill_tcp_data_p(buf,pl,PSTR("<br><br><h1><font color=#f36223>DEMO MODE</font></h1>"));
#endif
	pl=fill_tcp_data_p(buf,pl,PSTR("<br><br>FIRMWARE VERSION: <b>" FIRMWARE_VERSION));
	pl=fill_tcp_data_p(buf,pl,PSTR("</b></html>"));
	return(pl);
}

uint16_t print_webpage_reboot(uint8_t *buf)
{
	uint16_t pl;
	pl=http200ok();
	pl=page_header(buf,pl);
	pl=fill_tcp_data_p(buf,pl,PSTR("<SCRIPT LANGUAGE=\"javascript\">function reboot_ask(){\nvar question = confirm(\"Are you sure to reboot?\");\n if (question == false)  return false; else  return true; }</SCRIPT>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<center><br><br><br><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<a href=\"/reb?yes&sid="));
	itoa_long(sessionid,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr); // Вставляем сессию
	pl=fill_tcp_data_p(buf,pl,PSTR("\" onClick=\"return reboot_ask();\">Reboot device</a>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("</b></html>"));
	return(pl);
}



uint16_t print_webpage_sensor_config(uint8_t *buf, unsigned char a, unsigned char flag)
{
	uint16_t pl;
	pl=http200ok();
	pl=page_header(buf,pl);
	if (flag == 1)	pl=fill_tcp_data_p(buf,pl,PSTR("<SCRIPT language=\"JavaScript\">parent.frames['lf'].location.reload();</SCRIPT>"));	
	pl=fill_tcp_data_p(buf,pl,PSTR("<center><br><br>Sensor configuration<br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("</b><br><form method=\"get\" action=\"\">"));
    pl=fill_tcp_data_p(buf,pl,PSTR("<center><table border=0 cellspacing=0 cellpadding=10><tr><td align=center width=250>"));
    pl=fill_tcp_data_p(buf,pl,PSTR("ID: <b>"));
	sprintf_P(tmpstr,PSTR("%02X%02X%02X%02X%02X%02X%02X%02X"),all_sensors[a].id[0],all_sensors[a].id[1],all_sensors[a].id[2],all_sensors[a].id[3],all_sensors[a].id[4],all_sensors[a].id[5],all_sensors[a].id[6],all_sensors[a].id[7]); // делаем строку с Sensor id
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("</b><br><br><input type=checkbox name=en"));
	if (all_sensors[a].enable==0xFF) pl=fill_tcp_data_p(buf,pl,PSTR(" checked"));
	pl=fill_tcp_data_p(buf,pl,PSTR("><b>Enable/Disabe</b><br><br>"));		
	pl=fill_tcp_data_p(buf,pl,PSTR("<b>Sensor name:</b><br><input type=text name=na maxlength=14 value=\""));	
	pl=fill_tcp_data(buf,pl,(char *)all_sensors[a].name);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><b>Offset</b><br><input type=text name=of maxlength=6 size=6 value=\""));
	sprintf_P(tmpstr,PSTR("%2.2f"),all_sensors[a].offset);
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><b>Current value</b><br>"));
	get_sensor_value_type_str(a,0,tmpstr,tmpstr2);
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data(buf,pl,tmpstr2);
	pl=fill_tcp_data_p(buf,pl,PSTR("<br><b>Calculated value</b><br>"));
	get_sensor_value_type_str(a,all_sensors[a].offset,tmpstr,tmpstr2);
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data(buf,pl,tmpstr2);	
	pl=fill_tcp_data_p(buf,pl,PSTR("<input type=hidden name=num value=\""));
	sprintf_P(tmpstr,PSTR("%d"),a);
	pl=fill_tcp_data(buf,pl,tmpstr);	
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><input type=hidden name=sid value=\""));
	itoa_long(sessionid,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr); // Вставляем сессию
	pl=fill_tcp_data_p(buf,pl,PSTR("\"><br><br><input type=submit value=Save name=sa>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("</html>"));
	return(pl);
}


uint16_t print_webpage_leftmenu(uint8_t *buf)
{
	uint16_t pl,a,b;
	pl=http200ok();
	pl=page_header(buf,pl);
	
	itoa_long(sessionid,tmpstr,10);
	pl=fill_tcp_data_p(buf,pl,PSTR("<body><center>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<a href=/ip?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);	
	pl=fill_tcp_data_p(buf,pl,PSTR(""" target=rf>TCP/IP settings</a><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<a href=/pa?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);	
	pl=fill_tcp_data_p(buf,pl,PSTR(" target=rf>Password</a><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<a href=/log1?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(" target=rf>Logger 1</a><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("<a href=/log2?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(" target=rf>Logger 2</a><br>"));	
	
	for (a=0; a<6; a++)
	{
		pl=fill_tcp_data_p(buf,pl,PSTR("<font color="));
		if (a<=3) b=PA_READ(all_relay[a].pio); else b=PF_READ(all_relay[a].pio);
		if (b==0) { pl=fill_tcp_data_p(buf,pl,PSTR("#00FF00")); strcpy(tmpstr,"ON"); } else { pl=fill_tcp_data_p(buf,pl,PSTR("#FF0000")); strcpy(tmpstr,"OFF"); }
		pl=fill_tcp_data_p(buf,pl,PSTR("><b>&#9611;</b></font>"));
		
		pl=fill_tcp_data_p(buf,pl,PSTR("<a href=/rel?num="));
		sprintf(tmpstr2,"%d",a);
		pl=fill_tcp_data(buf,pl,tmpstr2);
		pl=fill_tcp_data_p(buf,pl,PSTR("&sid="));
		itoa_long(sessionid,tmpstr,10);
		pl=fill_tcp_data(buf,pl,tmpstr);
	    pl=fill_tcp_data_p(buf,pl,PSTR(" target=rf>Relay "));
		sprintf(tmpstr2,"%d",a+1);
		pl=fill_tcp_data(buf,pl,tmpstr2);		
		pl=fill_tcp_data_p(buf,pl,PSTR("</a><br>"));			
	}
	
	pl=fill_tcp_data_p(buf,pl,PSTR("<a href=/reb?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(" target=rf>Reboot</a><br>"));
		
	pl=fill_tcp_data_p(buf,pl,PSTR("<a href=/ex?sid="));
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(">Exit</a><br>"));
	pl=fill_tcp_data_p(buf,pl,PSTR("</html>"));
	return(pl);
}


uint16_t print_webpage_sensors_left_menu(uint8_t *buf)
{
	uint16_t pl,a;	
	pl=http200ok();

	pl=fill_tcp_data_p(buf,pl,PSTR("<html><body><center>Sensors</center><br>"));
	for (a=0; a<count_sensors; a++)
	{
		pl=fill_tcp_data_p(buf,pl,PSTR("<a href=/sen?num="));
		sprintf(tmpstr,"%d",a);
		pl=fill_tcp_data(buf,pl,tmpstr);
		pl=fill_tcp_data_p(buf,pl,PSTR("&sid="));
		itoa_long(sessionid,tmpstr,10);
		pl=fill_tcp_data(buf,pl,tmpstr);
		pl=fill_tcp_data_p(buf,pl,PSTR(" target=rf>"));
		pl=fill_tcp_data(buf,pl,(char *)all_sensors[a].name);
		pl=fill_tcp_data_p(buf,pl,PSTR("</a><br>"));
	}
	return(pl);
}



uint16_t print_webpage_index(uint8_t *buf)
{
	uint16_t pl,a,b,c;
	unsigned long diff,day,hr,min,sec;
	pl=http200ok();
	pl=page_header(buf,pl);

	#ifdef METEOBOX_DEMO_MODE
	pl=fill_tcp_data_p(buf,pl,PSTR("<center><h2><u>WebMeteoBox-3 &#9730;<font color=red>!!!DEMO MODE!!! Save settings locked.</font></u><br><a href=http://meteobox.tk target=_blank>WebMeteoBox project home page.</a></h2><table border=0><tr><td valign=top><h2><pre>"));
	#else
	pl=fill_tcp_data_p(buf,pl,PSTR("<title>WebMeteoBox-3</title><center><h2><u>WebMeteoBox-3</u> &#9730;</h2><table border=0><tr><td valign=top><h2><pre>"));
	#endif

	for (a=0; a<count_sensors; a++)
	{
		if (all_sensors[a].enable==0xFF)
		{
			pl=fill_tcp_data(buf,pl,(char *)all_sensors[a].name);
			c=strlen((char *)all_sensors[a].name);
			for (b=c; b<15; b++) pl=fill_tcp_data_p(buf,pl,PSTR(" "));
			get_sensor_value_type_str(a,all_sensors[a].offset,tmpstr,tmpstr2);
			pl=fill_tcp_data(buf,pl,tmpstr);
			pl=fill_tcp_data(buf,pl,tmpstr2);
			pl=fill_tcp_data_p(buf,pl,PSTR("\r\n"));
		}
	}
	#ifdef METEOBOX_DEMO_MODE
	pl=fill_tcp_data_p(buf,pl,PSTR("</pre></h2></td></tr></table><br><br><a href=/lg>Login</a> (default password <b>admin</b>)<br>"));
	#else
	pl=fill_tcp_data_p(buf,pl,PSTR("</pre></h2></td></tr></table><br><br><a href=/lg>Login</a><br>"));
	#endif

	#ifdef DEBUG_METEOBOX
	pl=fill_tcp_data_p(buf,pl,PSTR("DBG_MS: "));
	itoa_long(Time_ms,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr);
	#endif
	
	pl=fill_tcp_data_p(buf,pl,PSTR("<p>Designed for <a href=\"http://narodmon.ru\" target=\"_blank\">Public monitoring</a> project</font></p>"));
	
	pl=fill_tcp_data_p(buf,pl,PSTR("<br>Uptime: "));
	
	diff=floor(Time_ms/1000);
	day=floor(diff/60/60/24);
	diff-=day*60*60*24;
	hr =floor(diff/60/60);
	diff-=hr*60*60;
	min=floor(diff/60);
	diff -= min*60;
	sec=diff;
	
	itoa_long(day,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(" days "));
	itoa_long(hr,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(":"));
	itoa_long(min,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr);
	pl=fill_tcp_data_p(buf,pl,PSTR(":"));
	itoa_long(sec,tmpstr,10);
	pl=fill_tcp_data(buf,pl,tmpstr);
	
	//pl=fill_tcp_data_p(buf,pl,PSTR("<p>Designed for <a href=\"http://narodmon.ru\" target=\"_blank\">Public monitoring</a> project<br>Powered By <a href=\"http://meteobox.tk\">MeteoBox.tk</a> ver " FIRMWARE_VERSION "</font></p>"));
	
	pl=fill_tcp_data_p(buf,pl,PSTR("</body></html>"));
	return(pl);
}



void process_http_server()
{
	uint16_t len;
	char sendflag=0;
	unsigned char flag,a,use_dns=0;
	unsigned char current_logger=0;
	unsigned char post_flag=0;
#ifdef ENABLE_CUSTOM_REQUEST	
	char tmpbuf[10];
#endif	

	//len=enc28j60PacketReceive(BUFFER_SIZE, buf);
	len=enc28j60PacketReceive(256, buf);
	buf[BUFFER_SIZE]='\0'; // 04.06.2014
//----------------DHCP initialization -------------- 04.06.2014 -----------------	
	if (NetworkParam.dhcp==0xFF)
	{		
		// DHCP renew IP:
		len=packetloop_dhcp_renewhandler(buf,len);
	}
//----------------DHCP initialization -------------- 04.06.2014 -----------------	
	enc28j60EnableBroadcast(); // 11.06.2014 for udp configurator
	buf_pos=packetloop_arp_icmp_tcp(buf,len);
    // dat_p will be unequal to zero if there is a valid  http get
/*---------------------------------------------------------------------------------------------------------*/    
if (buf_pos==0)
{
	// we are idle here trigger arp and dns stuff here
	if (gw_arp_state==0){
	// find the mac address of the gateway (e.g your dsl router).
		get_mac_with_arp(NetworkParam.gw,TRANS_NUM_GWMAC,&arpresolver_result_callback);
		gw_arp_state=1;
	}
	if (get_mac_with_arp_wait()==0 && gw_arp_state==1){
	// done we have the mac address of the GW
		gw_arp_state=2;
	}
}
	
	if ((Wt0==0) || (Wt1==0)) {
		
		if (Wt0==0) { current_logger=0; if (server_data[current_logger].enable != 0xFF) { TimeWt0=WebrefreshTime0; Wt0=1; } else post_flag=1; }
		if (Wt1==0) { current_logger=1; if (server_data[current_logger].enable != 0xFF) { TimeWt1=WebrefreshTime1; Wt1=1; } else post_flag=1; }

		if (post_flag==1) {
		
			if(len==0){
				
				for (a=0; a<strlen((char *)server_data[current_logger].server_name); a++) // ищем точки в имени сервера, если их 3 значит там ip адрес
					if (server_data[current_logger].server_name[a] == '.') use_dns++;
					
				if (use_dns<3) // используем DNS
				{
					if (dns_state==0 && gw_arp_state==2){
						if (!enc28j60linkup()) return; // only for dnslkup_request we have to check if the link is up.
						sec=0;
						dns_state=1;			
						dnslkup_request(buf,(char *)server_data[current_logger].server_name,gwmac);
						return;
					}
					if (dns_state==1 && dnslkup_haveanswer()){
						dns_state=2;
						dnslkup_get_ip(otherside_www_ip);
						//printf("host ip: %d.%d.%d.%d\r\n",otherside_www_ip[0],otherside_www_ip[1],otherside_www_ip[2],otherside_www_ip[3]);
					}
					if (dns_state!=2){
						// retry every minute if dns-lookup failed:
						if (sec > 60){
							dns_state=0;
							//printf("if (sec > 60)");
						}
						// don't try to use web client before we have a result of dns-lookup
						return;
					}
				} else  { // Вместо DNS используем свой сервер
					
					data_get((char *)server_data[current_logger].server_name,'.',10);
					memcpy(otherside_www_ip,xbuf,4);
	
				}
				//----------
/*==========================================================================================================================				
				if (start_web_client==1){
					start_web_client=2;
					
					if (current_logger==0) // 0 логгер работает с протоколом narodmon.ru a логгер 1 c custom protocol
					{					
						strcpy(xbuf,"ID=");
						sprintf_P(tmpstr,PSTR("%02X%02X%02X%02X%02X%02X"),NetworkParam.mac[0],NetworkParam.mac[1],NetworkParam.mac[2],NetworkParam.mac[3],NetworkParam.mac[4],NetworkParam.mac[5]); // делаем строку с MAC
						strcat(xbuf,tmpstr);
					
						for (a=sensors_send;a<9+sensors_send;a++) // 9
						{
							if (a>=count_sensors) break;
							if (all_sensors[a].enable != 0xFF) continue; // Сенсор выключен, не отправляем данные на сайт add 09.06.2014
							sprintf_P(tmpstr,PSTR("&%02X%02X%02X%02X%02X%02X%02X%02X"),all_sensors[a].id[0],all_sensors[a].id[1],all_sensors[a].id[2],all_sensors[a].id[3],all_sensors[a].id[4],all_sensors[a].id[5],all_sensors[a].id[6],all_sensors[a].id[7]); // делаем строку с Sensor id
							strcat(xbuf,tmpstr);
							get_sensor_value_type_str(a,all_sensors[a].offset,tmpstr,tmpstr2);
							sprintf_P(tmpstr2,PSTR("=%s"),tmpstr);
							strcat(xbuf,tmpstr2);
						}
						sensors_send=a;
					}
					// http://wiki.wunderground.com/index.php/PWS_-_Upload_Protocol
					// http://www.tuxgraphics.org/toolbox/base64-javascript.html
					// http://bugs.openweathermap.org/projects/api/wiki/Upload_api
					// Host: openweathermap.org
					// POST /data/post
					// Authorization: Basic WmFnZXI6dGVzdHRlc3Q=
					else // логгер 1 - custom protocol 
					{
						strcpy(xbuf,(char *)custom_request);					
						sprintf_P(tmpstr,PSTR("%02X%02X%02X%02X%02X%02X"),NetworkParam.mac[0],NetworkParam.mac[1],NetworkParam.mac[2],NetworkParam.mac[3],NetworkParam.mac[4],NetworkParam.mac[5]); // делаем строку с MAC

						strcpy(xbuf,replace_str(xbuf,"{id}", tmpstr));
					
						for (a=0; a<count_sensors; a++) {
							
							//if (all_sensors[a].enable != 0xFF) continue;
							
							sprintf_P(tmpstr,PSTR("{n%d}"),a+1);
							strcpy(xbuf,replace_str(xbuf,tmpstr, (char *)all_sensors[a].name));
						
							sprintf_P(tmpstr,PSTR("{v%d}"),a+1);
							get_sensor_value_type_str(a,all_sensors[a].offset,tmpbuf,tmpstr2);
							strcpy(xbuf,replace_str(xbuf,tmpstr, tmpbuf));
						
							sprintf_P(tmpstr,PSTR("{i%d}"),a+1);
							sprintf_P(tmpstr2,PSTR("%02X%02X%02X%02X%02X%02X%02X%02X"),all_sensors[a].id[0],all_sensors[a].id[1],all_sensors[a].id[2],all_sensors[a].id[3],all_sensors[a].id[4],all_sensors[a].id[5],all_sensors[a].id[6],all_sensors[a].id[7]); // делаем строку с Sensor id
							strcpy(xbuf,replace_str(xbuf,tmpstr, tmpstr2));
						}	
					}
//----------------------------------------------------------------------------------------------					
					
					if (current_logger==0)
					  client_http_post((char *)server_data[current_logger].script_path,"",(char *)server_data[current_logger].server_name,NULL,xbuf,&browserresult_callback,otherside_www_ip,gwmac,
									   (uint16_t)server_data[current_logger].port);
					else { 
						// 2 логгер с поддержкой Basic Authorization  
						client_http_post((char *)server_data[current_logger].script_path,"",(char *)server_data[current_logger].server_name,
										 (char *)custom_password,xbuf,&browserresult_callback,otherside_www_ip,gwmac,
										 (uint16_t)server_data[current_logger].port);
						sensors_send=count_sensors;					
					}

					if (sensors_send>=count_sensors)
					{
						if (current_logger==0) { TimeWt0=WebrefreshTime0; Wt0=1; } else { TimeWt1=WebrefreshTime1; Wt1=1; }
						dns_state=0;
						sec=0;
						sensors_send=0;
						//printf("-----------CLEAR TIMER\r\n");
					}
				}
				//if (start_web_client==3 || sec>=5  ){ start_web_client=1; }
			    if (start_web_client==3 || sec>=8  ){ start_web_client=1;}
==========================================================================================================================*/


				if (start_web_client==1){
					start_web_client=2;
					
					strcpy(xbuf,"ID=");
					sprintf_P(tmpstr,PSTR("%02X%02X%02X%02X%02X%02X"),NetworkParam.mac[0],NetworkParam.mac[1],NetworkParam.mac[2],NetworkParam.mac[3],NetworkParam.mac[4],NetworkParam.mac[5]); // делаем строку с MAC
					strcat(xbuf,tmpstr);
					
					for (a=sensors_send;a<9+sensors_send;a++) // 9
					{
						if (a>=count_sensors) break;
						if (all_sensors[a].enable != 0xFF) continue; // Сенсор выключен, не отправляем данные на сайт add 09.06.2014
						sprintf_P(tmpstr,PSTR("&%02X%02X%02X%02X%02X%02X%02X%02X"),all_sensors[a].id[0],all_sensors[a].id[1],all_sensors[a].id[2],all_sensors[a].id[3],all_sensors[a].id[4],all_sensors[a].id[5],all_sensors[a].id[6],all_sensors[a].id[7]); // делаем строку с Sensor id
						strcat(xbuf,tmpstr);
						get_sensor_value_type_str(a,all_sensors[a].offset,tmpstr,tmpstr2);
						sprintf_P(tmpstr2,PSTR("=%s"),tmpstr);
						strcat(xbuf,tmpstr2);
					}
					sensors_send=a;
					client_http_post((char *)server_data[current_logger].script_path,"",(char *)server_data[current_logger].server_name,NULL,xbuf,&browserresult_callback,otherside_www_ip,gwmac,(uint16_t)server_data[current_logger].port);
					sec=0;
					if (sensors_send>=count_sensors)
					{
						if (current_logger==0) { TimeWt0=WebrefreshTime0; Wt0=1; } else { TimeWt1=WebrefreshTime1; Wt1=1; }
						dns_state=0;
						sec=0;
						sensors_send=0;
						//printf("-----------CLEAR TIMER\r\n");
					}
				}
				if (start_web_client==3 || sec>=5  ){ start_web_client=1; }




						
				return;
			} // if(len==0){
		} // if (post_flag==1) { 
	} // if ((Wt0==0) && (Wt1==0)) { 
	

/*---------------------------------------------------------------------------------------------------------*/    	
	if(buf_pos==0) { udp_client_check_for_dns_answer(buf,len); 
/*------------------------------ UDP CONFIGURATION --------------------------------------------------------*/   		
		if (buf[IP_PROTO_P]==IP_PROTO_UDP_V&&buf[UDP_DST_PORT_H_P]==(char)(CONFIG_CLIENT_PORT>>8)&&buf[UDP_DST_PORT_L_P]==(char)(CONFIG_CLIENT_PORT)){

			if(strncmp_P((char *)&buf[UDP_DATA_P],PSTR("SRCH"),4) == 0)
			{
				//printf("SRCH!\r\n");
				memcpy_P((unsigned char*)xbuf,  PSTR("BOX3"), 4);
				memcpy((unsigned char*)xbuf+4 ,NetworkParam.gw, 4);
				memcpy((unsigned char*)xbuf+8 ,NetworkParam.subnet, 4);
				memcpy((unsigned char*)xbuf+12,NetworkParam.ip, 4);
				memcpy((unsigned char*)xbuf+16,NetworkParam.mac, 6);
				memcpy((unsigned char*)xbuf+22,NetworkParam.dns, 4);
				make_udp_reply_from_request(buf,xbuf,26,CONFIG_SERVER_PORT);
			}
			else if(strncmp_P((char *)&buf[UDP_DATA_P],PSTR("SETQ"),4) == 0) // Изменение конфигурации
			{
				if ((buf[UDP_DATA_P+16] == NetworkParam.mac[0]) &&
				(buf[UDP_DATA_P+17] == NetworkParam.mac[1]) &&
				(buf[UDP_DATA_P+18] == NetworkParam.mac[2]) &&
				(buf[UDP_DATA_P+19] == NetworkParam.mac[3]) &&
				(buf[UDP_DATA_P+20] == NetworkParam.mac[4]) &&
				(buf[UDP_DATA_P+21] == NetworkParam.mac[5]))
				{
					//printf("SETQ!\r\n");
					memcpy_P((unsigned char*)xbuf, PSTR("SETR"), 4);
					make_udp_reply_from_request(buf,xbuf,4,CONFIG_SERVER_PORT);
					
					memcpy(NetworkParam.gw,     &buf[UDP_DATA_P+4],  4);
					memcpy(NetworkParam.subnet, &buf[UDP_DATA_P+8],  4);
					memcpy(NetworkParam.ip,     &buf[UDP_DATA_P+12], 4);
					memcpy(NetworkParam.dns,    &buf[UDP_DATA_P+22], 4);
					eeprom_write_block(&NetworkParam, (unsigned char*)EEPOP, sizeof(NetworkParam));
					GoApp();
				}
			}
			else if(strncmp_P((char *)&buf[UDP_DATA_P],PSTR("R**T"),4) == 0) // сброс к настройкам по умолчанию
			{
				if ((buf[UDP_DATA_P+16] == NetworkParam.mac[0]) &&
				(buf[UDP_DATA_P+17] == NetworkParam.mac[1]) &&
				(buf[UDP_DATA_P+18] == NetworkParam.mac[2]) &&
				(buf[UDP_DATA_P+19] == NetworkParam.mac[3]) &&
				(buf[UDP_DATA_P+20] == NetworkParam.mac[4]) &&
				(buf[UDP_DATA_P+21] == NetworkParam.mac[5]))
				{
					//printf("R**T!\r\n");
					default_network(0);
					GoApp();
				}
			}			
			else if(strncmp_P((char *)&buf[UDP_DATA_P],PSTR("R11T"),4) == 0) // сброс всего кроме сетвых настроек и пароля
			{
				if ((buf[UDP_DATA_P+16] == NetworkParam.mac[0]) &&
				(buf[UDP_DATA_P+17] == NetworkParam.mac[1]) &&
				(buf[UDP_DATA_P+18] == NetworkParam.mac[2]) &&
				(buf[UDP_DATA_P+19] == NetworkParam.mac[3]) &&
				(buf[UDP_DATA_P+20] == NetworkParam.mac[4]) &&
				(buf[UDP_DATA_P+21] == NetworkParam.mac[5]))
				{
					//printf("R11T!\r\n");
					default_network(1);
					GoApp();
				}
			}
			else if(strncmp_P((char *)&buf[UDP_DATA_P],PSTR("R22T"),4) == 0) // сброс пароля
			{
				if ((buf[UDP_DATA_P+16] == NetworkParam.mac[0]) &&
				(buf[UDP_DATA_P+17] == NetworkParam.mac[1]) &&
				(buf[UDP_DATA_P+18] == NetworkParam.mac[2]) &&
				(buf[UDP_DATA_P+19] == NetworkParam.mac[3]) &&
				(buf[UDP_DATA_P+20] == NetworkParam.mac[4]) &&
				(buf[UDP_DATA_P+21] == NetworkParam.mac[5]))
				{
					//printf("R22T!\r\n");
					default_network(2);
					GoApp();
				}
			}			
		}
/*------------------------------ UDP CONFIGURATION --------------------------------------------------------*/   				
		return;
	}  // no http request
	
	if (strncmp_P((char *)&buf[buf_pos],PSTR("GET /"),5) != 0) return;
	
	bufrecv=&buf[buf_pos];
	//strcpy((char *)bufrecv,findstr((char *)&buf[buf_pos],"GET /"," HTTP")); // Вытаскиваем GET запрос
	strcpy((char *)bufrecv,findstr_P((char *)&buf[buf_pos],PSTR("GET /"),PSTR(" HTTP"))); // Вытаскиваем GET запрос
	
	if (strlen((char *)bufrecv) == 0 ){
		//Tr0=0;
		buf_pos=print_webpage_index(buf);
		sendflag=1;
		//printf(">print_webpage_index\r\n");
		goto SENDTCP;
	}
	

	if (strncmp_P((char *)bufrecv,PSTR("lg"),2) == 0 ){
		strcpy((char *)xbuf,findstr_P((char *)bufrecv,PSTR("pa="),PSTR("&")));
		if (strlen(xbuf) > 3)
		{
			if (strstr(xbuf,MPasswd))
			{
				sessionid=Time_ms; //Создали сессию
				buf_pos=print_webpage_mainframe(buf);
				//printf(">print_webpage_mainframe\r\n");
				sendflag=1;
				goto SENDTCP;
			}
		}
		buf_pos=print_webpage_login(buf);
		//printf(">print_webpage_login\r\n");
		sendflag=1;
		goto SENDTCP;
	}
	
	if (strncmp_P((char *)bufrecv,PSTR("json"),4) == 0 ){
		buf_pos=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-type: application/json\r\n\r\n"));
		buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("{\r\n\"Sensors\":{\r\n"));
		for (a=0; a<count_sensors; a++)
		{
			if (all_sensors[a].enable==0xFF)
			{
				sprintf_P(tmpstr,PSTR("\"%02X%02X%02X%02X%02X%02X%02X%02X"),all_sensors[a].id[0],all_sensors[a].id[1],all_sensors[a].id[2],all_sensors[a].id[3],all_sensors[a].id[4],all_sensors[a].id[5],all_sensors[a].id[6],all_sensors[a].id[7]); // делаем строку с Sensor id
				buf_pos=fill_tcp_data(buf,buf_pos,tmpstr);
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("\": {\r\n\"Type\":\""));
				sprintf(tmpstr,"%d",all_sensors[a].type);
				buf_pos=fill_tcp_data(buf,buf_pos,tmpstr);
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("\",\r\n\"Name\":\""));
				buf_pos=fill_tcp_data(buf,buf_pos,(char *)all_sensors[a].name);
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("\",\r\n\"Value\":\""));
				get_sensor_value_type_str(a,all_sensors[a].offset,tmpstr,tmpstr2);
				buf_pos=fill_tcp_data(buf,buf_pos,tmpstr);
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("\"\r\n}"));
				if (a<count_sensors-1) buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR(",\r\n"));	
			}
		}
		buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("\r\n}\r\n}"));
		sendflag=1;
		goto SENDTCP;
	}	
	
	if (strncmp_P((char *)bufrecv,PSTR("favicon.ico"),11) == 0 ){
		//printf(">favicon.ico\r\n");
		buf_pos=http404();
		sendflag=1;
		goto SENDTCP;
	}
	
	strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("sid="),PSTR("&")));
	itoa_long(sessionid,tmpstr2,10);
	//printf("session GET: %s   Saved: %s\r\n",tmpstr,tmpstr2);

#ifdef DEBUG_METEOBOX			
	if (1) //При отладке отключаем авторизацию
#else	
	if ((strstr(tmpstr,tmpstr2)) && (sessionid != 0)) // Сравниваем сессию пользователя и сохраненную, сессия должны быть проинициализированна т.е. не равна 0
#endif 	
	{
		if (strncmp_P((char *)bufrecv,PSTR("menu"),4) == 0 ){
			buf_pos=print_webpage_leftmenu(buf);
			//printf(">print_webpage_leftmenu\r\n");
			sendflag=1;
			goto SENDTCP;
		}
		
		if (strncmp_P((char *)bufrecv,PSTR("dev"),3) == 0 ){
			buf_pos=print_webpage_sensors_left_menu(buf);
			sendflag=1;
			goto SENDTCP;
		}		
	
		if (strncmp_P((char *)bufrecv,PSTR("info"),4) == 0 ){
			buf_pos=print_webpage_info(buf);
			//printf(">print_webpage_leftmenu\r\n");
			sendflag=1;
			goto SENDTCP;
		}
		
		if (strncmp_P((char *)bufrecv,PSTR("reb"),3) == 0 ){
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("reb?"),PSTR("&")));
			if (strncmp("yes",(char *)tmpstr,3) == 0 )
			{
				buf_pos=http200ok();
#ifdef METEOBOX_DEMO_MODE
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("<HTML><body><h1>Reboot disabled in demo mode!</h1></html>"));
				www_server_reply(buf,buf_pos);
				return;
#else				
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("<HTML><HEAD><SCRIPT language=\"JavaScript\">top.location=\"/"));
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("\";</SCRIPT>"));
				www_server_reply(buf,buf_pos);
				//printf(">REBOOT!!!\r\n");
				GoBoot();
#endif									
			}
			buf_pos=print_webpage_reboot(buf);
			sendflag=1;
			goto SENDTCP;
		}		
		
		if (strncmp_P((char *)bufrecv,PSTR("ex"),2) == 0 ){ // Обработчик ссылки Exit
			//buf_pos=print_webpage_index(buf);
			buf_pos=http200ok();
			buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("<HTML><HEAD><SCRIPT language=\"JavaScript\">top.location=\"/"));
			//sprintf_P(tmpstr,PSTR("%d.%d.%d.%d"),NetworkParam.ip[0],NetworkParam.ip[1],NetworkParam.ip[2],NetworkParam.ip[3]); // делаем строку с IP
			//buf_pos=fill_tcp_data(buf,buf_pos,tmpstr);
			buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("\";</SCRIPT>"));
			//printf(">CLEAR SESSION\r\n");
			sessionid=0; // Clear session
			sendflag=1;
			goto SENDTCP;
		}

/*
#ifdef ENABLE_CUSTOM_REQUEST		
		if (strncmp_P((char *)bufrecv,PSTR("req"),3) == 0 ){
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("ss="),PSTR("&")));
			if (strncmp_P((char *)tmpstr,PSTR("Save"),4) == 0 ) { // Save Custom Request Settings
				
				strcpy((char *)xbuf,findstr_P((char *)bufrecv,PSTR("cus="),PSTR("&"))); // свой запрос
				urldecode(xbuf);
				strcpy((char *)custom_request,xbuf);
				eeprom_update_block(custom_request,(unsigned char*)eeprom_custom_request_config,sizeof(custom_request));
				
				strcpy((char *)xbuf,findstr_P((char *)bufrecv,PSTR("aut="),PSTR("&"))); // пароль для авторизации
				urldecode(xbuf);
				strcpy((char *)custom_password,xbuf);
				//sprintf((char *)custom_password,"%d",strlen(xbuf));
				//strcpy((char *)custom_password,"123456");
				eeprom_update_block(custom_password,(unsigned char*)eeprom_custom_password,sizeof(custom_password));
				
			}
			buf_pos=print_custom_request(buf);
			sendflag=1;
			goto SENDTCP;
		}	
#endif		
*/	
			
		
		if (strncmp_P((char *)bufrecv, PSTR("sen"), 3) == 0 ){
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("num="),PSTR("&")));
			a=ATOI(tmpstr,10);
			//printf("SEN>%s-%d\r\n",tmpstr,a);
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("sa="),PSTR("&")));
			flag=0;
			if (strncmp_P((char *)tmpstr,PSTR("Save"),4) == 0 ){ // Save Logger configuration
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("en="),PSTR("&")));
				if (strncmp("on",(char *)tmpstr,2) == 0) all_sensors[a].enable=0xFF; else all_sensors[a].enable=0x00;// Sensor enable ?				
				strcpy(xbuf,findstr_P((char *)bufrecv,PSTR("na="),PSTR("&")));
				urldecode(xbuf);
				strcpy((char *)all_sensors[a].name,xbuf);
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("of="),PSTR("&")));
				all_sensors[a].offset=atof(tmpstr);	
				eeprom_update_block(&all_sensors[a].value, (unsigned char*)(eeprom_sensors_config+(a*sizeof(all_sensors[0]))), sizeof(all_sensors[0]));			
				flag=1; 
			}			
			buf_pos=print_webpage_sensor_config(buf,a,flag);
			sendflag=1;
			goto SENDTCP;
		}	
		
		if (strncmp_P((char *)bufrecv, PSTR("pa"), 2) == 0 ){
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("new="),PSTR("&")));
			flag=0;
#ifdef METEOBOX_DEMO_MODE
#else			
			if (strlen(tmpstr)>3)
			{
				eeprom_update_block((char*)tmpstr,(unsigned char*)eeprom_password, 15);
				strcpy(MPasswd,tmpstr);
				flag=1;
			}
#endif			
			buf_pos=print_webpage_parol_change(buf,flag);
			sendflag=1;
			goto SENDTCP;			
		}
		
		if (strncmp_P((char *)bufrecv, PSTR("rel"), 3) == 0 ){
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("num="),PSTR("&")));
			a=ATOI(tmpstr,10);
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("On%"),PSTR("&")));
			if (strncmp_P((char *)tmpstr, PSTR("2FO"), 3) == 0 ){ // Relay on/off button press
				if (a<=3) PA_WRITE(all_relay[a].pio,!PA_READ(all_relay[a].pio));
				else PF_WRITE(all_relay[a].pio,!PF_READ(all_relay[a].pio));
			}
			else
			{
#ifdef METEOBOX_DEMO_MODE
#else				
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("nd="),PSTR("&")));
				if (strncmp_P((char *)tmpstr, PSTR("Save"), 4) == 0 ){ // Relay Save button press		
					strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("min="),PSTR("&")));		
					all_relay[a].min=atof(tmpstr);
					strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("max="),PSTR("&")));
					all_relay[a].max=atof(tmpstr);	
					strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("sen="),PSTR("&")));	
					flag=ATOI(tmpstr,10);	
					if (flag==100) all_relay[a].flag=0; else all_relay[a].flag=0xFF;							
					memcpy(all_relay[a].id,all_sensors[flag].id,8);
					eeprom_update_block(&all_relay[a], (unsigned char*)eeprom_relay_config+(a*sizeof(all_relay[0])), sizeof(all_relay[0]));
					//printf("Save!\r\n");
				}
#endif				
			}
			buf_pos=print_relay_settings(buf,a);
			sendflag=1;
			goto SENDTCP;
		}			
			
		
		if ((strncmp_P((char *)bufrecv,PSTR("log1"),4) == 0) || (strncmp_P((char *)bufrecv,PSTR("log2"),4) == 0)){
			if (strncmp_P((char *)bufrecv,PSTR("log1"),4) == 0) flag=0; else flag=1;	
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("Save+%26+"),PSTR("&")));	
#ifdef METEOBOX_DEMO_MODE
#else			
			if (strncmp_P((char *)xbuf,PSTR("Test"),4) == 0 ){ // Save Logger configuration				
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("en="),PSTR("&")));
				if (strncmp_P((char *)xbuf,PSTR("on"),2) == 0) server_data[flag].enable=0xFF; else server_data[flag].enable=0x00;// logger 1 enable ?
				
/*
				strcpy((char *)tmpstr,findstr((char *)bufrecv,"i=","&"));
				data_get(tmpstr,'.',10);
				memcpy(server_data[flag].server_ip,xbuf,4);*/
				
				strcpy((char *)server_data[flag].server_name,findstr_P((char *)bufrecv,PSTR("host="),PSTR("&")));
				memset(server_data[flag].script_path,0x00,sizeof(server_data[0].script_path));
				strcpy((char *)xbuf,findstr_P((char *)bufrecv,PSTR("path="),PSTR("&")));				
				urldecode(xbuf);	
				strcpy((char *)server_data[flag].script_path,(char *)xbuf);				
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("port="),PSTR("&")));	
				server_data[flag].port=ATOI(tmpstr,10);
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("sp="),PSTR("&")));
				server_data[flag].refresh_time=update_time[ATOI(tmpstr,10)].id;	
				if (flag==0) {
					WebrefreshTime0=(unsigned long int)server_data[flag].refresh_time*100;
					eeprom_update_block(&server_data[flag], (unsigned char*)eeprom_server_config, sizeof(server_data[0]));
					Wt0 = 0;
				}
				else {
					WebrefreshTime1=(unsigned long int)server_data[flag].refresh_time*100;
					eeprom_update_block(&server_data[flag], (unsigned char*)eeprom_server_config+sizeof(server_data[0]), sizeof(server_data[0]));
					Wt1 = 0;
				}
			}	
#endif			
			buf_pos=print_webpage_logger_settings(buf, flag); // logger 
			sendflag=1;
			//printf(">print_webpage_logger_settings\r\n");
			goto SENDTCP;
		}		

		if (strncmp_P((char *)bufrecv,PSTR("ip"),2) == 0 ){
			strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("act="),PSTR("+")));
			
#ifdef METEOBOX_DEMO_MODE
#else			
			if (strncmp_P((char *)tmpstr,PSTR("Save"),4) == 0 ){ // Save EEPROM & REBOOT
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("m="),PSTR("&")));				
				urldecode(tmpstr);
				//printf("m=%s\r\n",tmpstr);				
				data_get((char *)tmpstr,':',16);
				NetworkParam.mac[0]=xbuf[0]; NetworkParam.mac[1]=xbuf[1]; NetworkParam.mac[2]=xbuf[2]; NetworkParam.mac[3]=xbuf[3]; NetworkParam.mac[4]=xbuf[4]; NetworkParam.mac[5]=xbuf[5];
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("s="),PSTR("&")));
				//printf("s=%s\r\n",tmpstr);
				data_get((char *)tmpstr,'.',10);
				NetworkParam.subnet[0]=xbuf[0]; NetworkParam.subnet[1]=xbuf[1]; NetworkParam.subnet[2]=xbuf[2]; NetworkParam.subnet[3]=xbuf[3];
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("g="),PSTR("&")));
				//printf("g=%s\r\n",tmpstr);	
				data_get((char *)tmpstr,'.',10);
				NetworkParam.gw[0]=xbuf[0]; NetworkParam.gw[1]=xbuf[1]; NetworkParam.gw[2]=xbuf[2]; NetworkParam.gw[3]=xbuf[3];							
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("d="),PSTR("&")));
				//printf("d=%s\r\n",tmpstr);	
				data_get((char *)tmpstr,'.',10);
				NetworkParam.dns[0]=xbuf[0]; NetworkParam.dns[1]=xbuf[1]; NetworkParam.dns[2]=xbuf[2]; NetworkParam.dns[3]=xbuf[3];
				
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("dh="),PSTR("&")));
				if (strncmp("on",(char *)tmpstr,2) == 0) NetworkParam.dhcp=0xFF; else NetworkParam.dhcp=0x00;// DHCP enable ? add.03.06.2014
				
				strcpy((char *)tmpstr,findstr_P((char *)bufrecv,PSTR("i="),PSTR("&")));
				data_get((char *)tmpstr,'.',10);
				NetworkParam.ip[0]=xbuf[0]; NetworkParam.ip[1]=xbuf[1]; NetworkParam.ip[2]=xbuf[2]; NetworkParam.ip[3]=xbuf[3];
				
				
				eeprom_update_block(&NetworkParam, (unsigned char*)EEPOP, sizeof(NetworkParam));	
				
				buf_pos=http200ok();
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("<HEAD><SCRIPT language=\"JavaScript\">top.location=\"http://"));				
				buf_pos=fill_tcp_data(buf,buf_pos,tmpstr);
				buf_pos=fill_tcp_data_p(buf,buf_pos,PSTR("\";</SCRIPT>"));
				www_server_reply(buf,buf_pos);
				//printf("!!! len %d\r\n",buf_pos);	
				GoApp();						
			}
#endif			
			
			buf_pos=print_webpage_tcpip_settings(buf);
			sendflag=1;
			//printf(">print_webpage_tcpip_settings\r\n");
			goto SENDTCP;
		}
	}
	else
	{
		//printf(">session wrong!!!\r\n");
		buf_pos=http404();
		sendflag=1;
		goto SENDTCP;
	}
	
	
SENDTCP:
	if (sendflag==1) www_server_reply(buf,buf_pos); // send web page data
	//printf("www  len %d\r\n",buf_pos);	
}



