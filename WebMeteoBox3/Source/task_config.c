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

#include <avr/eeprom.h> 
#include <avr/pgmspace.h>
#include "term.h"
#include "protocol.h"
#include "task_config.h"

//First mac address = BOX - 424F58
#define DefPassword		    "admin\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" //пароль по умолчанию admin
//\xc0\xa8\x01\xde = = 192.168.1.222
//\xc0\xa8\x00\x64 = = 192.168.0.100

/* для подсети 192.168.1.222 */
#define DefConfigParam		"\x82\x82\x82\x82\x00\x00\x42\x4F\x58\x00\x02\x05\xc0\xa8\x01\xde\xFF\xFF\xFF\x00\xc0\xa8\x01\x01\xc0\xa8\x01\x01\x00"

/* для домашней подсети 192.168.0.100 */
//#define DefConfigParam		"\x82\x82\x82\x82\x00\x00\x42\x4F\x58\x41\x02\x05\xc0\xa8\x00\x64\xFF\xFF\xFF\x00\xc0\xa8\x00\x01\xc0\xa8\x00\x01\x00"

StrConfigParam NetworkParam;
Server_config server_data[2]; // 2 логгера

#ifdef ENABLE_CUSTOM_REQUEST
	unsigned char custom_request[100]; //для пользовательского запроса
	unsigned char custom_password[40]; //для пользовательского пароля Authorization: Basic 
#endif


void ENC28J60_RST( char ckl )
{
	if (ckl == 1) PORTE |= (1<<7);
	else PORTE &= ~(1<<7);
}


void ENC28J60_REBOOT() 
{
	ENC28J60_RST(0);
	my_wait_1ms(10);
	ENC28J60_RST(1);
	my_wait_1ms(15);
}



void mcu_init(void)
{
	//EICRA = 0x00;			// External Interrupt Control Register A clear
	//EICRB = 0x02;			// External Interrupt Control Register B clear // edge
	//EIMSK = (1 << INT0);		// External Interrupt Mask Register : 0x10
	
#ifdef DOZIMETR		
	EICRA = 0x20;
	EICRB = 0x00;
	EIMSK = (1 << INT2); //11.08.2014	
#else
	EICRA = 0x00;
	EICRB = 0x00;
	EIMSK = 0x00;	
#endif 
	
	EIFR = 0xFF;			// External Interrupt Flag Register all clear
	
	PORTA=0xFF;     // 4 relay 0ff
    //PORTA=0x00;	// 6 relay off
	DDRA= 0x0F;

	PORTB=0xFF;
	DDRB= 0x07;

	PORTC=0xFF;
	DDRC= 0x00;

	PORTD=0xFF;
	DDRD= 0x00;
	
	//PORTD |= (1<<7); // XP3 - Hardware Reset 13.05.2013
	//DDRD &= ~(1<<7); // XP3 - Hardware Reset 13.05.2013

	

	PORTE=0xFF;
	DDRE= 0xF6; //fe
	
	PORTF=0x18; // 22.05.2014
	DDRF= 0x18; // 0x18
	
	// ADC initialization
	// ADC Clock frequency: 115,200 kHz
	// ADC Voltage Reference: AVCC pin
	//ADMUX=ADC_REF_AVCC & 0xff;
	// #define ADC_REF_VREF                        0
	ADMUX=ADC_REF_VREF & 0xff;
	ADCSRA=0x87;


	//_______________таймер-0 кварц 8 мгц ________________________
	ASSR  = 0x00;
	TCCR0 = 0x0F;// TIMER1 clock is xtal/1024
	OCR0  = 0x4E;// при xtal/1024 -- 10mc


	//_______________таймер-0 кварц 14.7456________________________
	//	ASSR  = 0x00;
	//	TCCR0 = 0x0F;// TIMER1 clock is xtal/1024
	//	OCR0  = 0x90;// при xtal/1024 -- 10mc
	//	OCR0  = 0x48;// при xtal/1024 -- 5mc

	//_______________enable_Timer_Interrapt's_____
	TIFR   = 0x00;// clear TIMER1 interrupts flags
	TIMSK  = 0x02;//enable TIMER0 compare interrupt //0x04;// enable TIMER1 overflow interrupt
	
	//	MCUCR = 0x80;		// MCU control regiseter : enable external ram
	//	XMCRA = 0x00;		// External Memory Control Register A :
	// Low sector   : 0x1100 ~ 0x7FFF
	// Upper sector : 0x8000 ~ 0xFFFF
	
	/* Set baud rate */
	UBRR0H 		= 0x00;//(unsigned char) (baud>>8);
	UBRR0L 		= 0x0C; //==38400 //0x33 = 9600;//(unsigned char) baud;
	/* Enable receiver and transmitter */
	UCSR0B 		= ((1<<RXCIE0)/*|(1<<TXCIE1)*/|(1<<RXEN0)|(1<<TXEN0));
	UCSR0C 		= 0x06;
}



void default_network(char level) // 29.11.2013
{
    unsigned char a;	
	char * b;
	
	cli();
	
	if (level==2 || level==0) // 2 - сброс пароля
	{
		eeprom_write_block((unsigned char*)DefPassword,(unsigned char*)eeprom_password, 15);
		eeprom_write_block((unsigned char*)DefPassword, (unsigned char*)eeprom_login,   15);
		asm("WDR");
		if (level==2) return;
	}
	
	
	
	memset(&all_relay,0x00,sizeof(all_relay));
	
	
	all_relay[0].pio=0; // Присваиваем к номерам реле pio atmega
	all_relay[1].pio=1;
	all_relay[2].pio=2;
	all_relay[3].pio=3;
	all_relay[4].pio=3; // 25.11.2013
	all_relay[5].pio=4; // 25.11.2013
	
	
	b=(char *)(all_relay);
	for (a=0; a<sizeof(all_relay); a++) {
		eeprom_write_byte((unsigned char*)eeprom_relay_config+a,*(b+a));
		asm("WDR");
	}
	
#ifdef ENABLE_CUSTOM_REQUEST		
	strcpy_P((char *)custom_request,PSTR("ID={id}&{i1}={v1}&{i2}={v2}&{i3}={v3}&{i4}={v4}&{i5}={v5}"));
	//strcpy_P((char *)custom_request,PSTR("humidity={v1}&temp={v2}&lat=19.11&long=14.11&alt=199&name=Meteobox3Test"));
	eeprom_write_block(custom_request,(unsigned char*)eeprom_custom_request_config,sizeof(custom_request));
	
	//strcpy_P((char *)custom_password,PSTR("base64 password encode"));
	strcpy_P((char *)custom_password,PSTR(""));
	//strcpy_P((char *)custom_password,PSTR("WmFnZXI6dGVzdHRlc3Q="));
	eeprom_write_block(custom_password,(unsigned char*)eeprom_custom_password,sizeof(custom_password));	
#endif	
	
	//memset(server_data,0x00,sizeof(server_data));
	server_data[0].enable=0x00; // enable=0xff
	strcpy_P((char *)server_data[0].server_name,PSTR("narodmon.ru"));
	strcpy_P((char *)server_data[0].script_path,PSTR("/post.php"));
	
	//strcpy(server_data[0].server_name,"inflnr.jinr.ru");
	//strcpy(server_data[0].script_path,"/test/add.php");
	
	//используйте IP 91.122.49.168 или 94.19.113.221 до которого у Вас наименьший ping
	
/*
	server_data[0].server_ip[0]=0;  // ip 0.0.0.0 = use dns
	server_data[0].server_ip[1]=0;
	server_data[0].server_ip[2]=0;
	server_data[0].server_ip[3]=0;*/
	

	server_data[0].refresh_time=300; // Время обовления по умолчанию 1 раз в 5 минут = 300
	server_data[0].port=80;
	
	memcpy(&server_data[1],&server_data[0],sizeof(server_data[0]));
	
	strcpy_P((char *)server_data[1].server_name,PSTR("meteobox.tk"));
	strcpy_P((char *)server_data[1].script_path,PSTR("/script/add.php"));

	
	//strcpy_P((char *)server_data[1].server_name,PSTR("openweathermap.org"));
	//strcpy_P((char *)server_data[1].script_path,PSTR("/data/post"));


	
	server_data[1].refresh_time=60; // 1 раз в минуту = 60
	
	
	//eeprom_write_block(&server_data[0], (unsigned char*)eeprom_server_config, sizeof(server_data[0]));
	//eeprom_write_block((const void*)&server_data[0], (unsigned char*)eeprom_server_config, 85);
	b=(char *)(server_data);
	for (a=0; a<sizeof(server_data); a++) {
		eeprom_write_byte((unsigned char*)eeprom_server_config+a,*(b+a));
		asm("WDR");
	}
	
	//asm("WDR");
	//eeprom_write_block(&server_data[1], (unsigned char*)eeprom_server_config+sizeof(server_data[0]), sizeof(server_data[0]));
	
	
	if (level==0)
	{
		memcpy_P(&NetworkParam, PSTR(DefConfigParam), sizeof(NetworkParam));
	
		my_wait_1ms(5);	
		asm("WDR");
	
		eeprom_write_block(&NetworkParam, (unsigned char*)EEPOP, sizeof(NetworkParam));
	}
	
	
	my_wait_1ms(5);	
	asm("WDR");
	
	
	strcpy_P((char *)all_sensors[0].name,PSTR("No name"));
	//strcpy((char *)all_sensors[0].name,"No name1234567");
	strcpy_P((char *)all_sensors[0].id,PSTR("NONE"));
	all_sensors[0].value=0;
	all_sensors[0].type=0;
	all_sensors[0].flag=0;
	all_sensors[0].offset=0.0000f;
	all_sensors[0].enable=0xFF;

	for (a=0; a<MAX_SENSORS; a++) {
		
		//if (a<3) all_sensors[a].offset=220.0f;
		
		memcpy(&all_sensors[a],&all_sensors[0],sizeof(all_sensors[0]));
		eeprom_update_block(&all_sensors[0], (unsigned char*)(eeprom_sensors_config)+(a*sizeof(all_sensors[0])), sizeof(all_sensors[0]));
		
/*
		memcpy(&all_sensors[a],&all_sensors[0],sizeof(all_sensors[0]));
		
		eeprom_write_block(all_sensors[0].name, (unsigned char*)(eeprom_sensors_config+12)+(a*sizeof(all_sensors[0])), sizeof(all_sensors[0].name));
		my_wait_1ms(5);	
		asm("WDR");
		eeprom_write_block((unsigned char*)(&all_sensors[0].offset), (unsigned char*)(eeprom_sensors_config+29)+(a*sizeof(all_sensors[0])), 4);
		eeprom_write_block((unsigned char*)(&all_sensors[0].enable), (unsigned char*)(eeprom_sensors_config+29+4)+(a*sizeof(all_sensors[0])), 1);
		my_wait_1ms(5);	*/
		asm("WDR");
	}

	sei();
	
}


void SetConfig(void)
{
	unsigned char temp;
	

	// Get Network Parameters
	temp = eeprom_read_byte((unsigned char*)EEPOP);

	if( temp != SPECIALOP)
	{
		// This board is initial state
		default_network(0); // Full initialize
	}
	else
	{
		cli();
#ifdef ENABLE_CUSTOM_REQUEST			
		eeprom_read_block(&custom_password,(unsigned char*)eeprom_custom_password, sizeof(custom_password));
		eeprom_read_block(&custom_request,(unsigned char*)eeprom_custom_request_config, sizeof(custom_request));
#endif		
		
		eeprom_read_block(&all_relay, (unsigned char*)eeprom_relay_config, sizeof(all_relay));
		asm("WDR");
		
		for (temp=0; temp<MAX_SENSORS; temp++)
		{
			eeprom_read_block((unsigned char*)(&all_sensors[temp]),   (unsigned char*)(eeprom_sensors_config)+(temp*sizeof(all_sensors[0])), sizeof(all_sensors[0]));			
			asm("WDR");
		}
		
		eeprom_read_block(&server_data, (unsigned char*)eeprom_server_config, sizeof(server_data));
		asm("WDR");
		
		eeprom_read_block(&NetworkParam,(unsigned char*)EEPOP, sizeof(NetworkParam));
		asm("WDR");
		
		sei();

	}

	
}


