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

/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 *
 * A very basic web server. 
 *
 * http://tuxgraphics.org/electronics/
 * Chip type           : Atmega88/168/328/644 with ENC28J60
 *********************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "dhcp_client.h"
#include "timeout.h"
#include "protocol.h"
#include "OWIPolled.h"
#include "OWIHighLevelFunctions.h"
#include "OWIBitFunctions.h"
#include "term.h"
#include "DHT.h"
#include "task_config.h"
#include "i2csoft.h"

/*
// set output to VCC, red LED off
#define LEDOFF PORTB|=(1<<PORTB1)
// set output to GND, red LED on
#define LEDON PORTB&=~(1<<PORTB1)
// to test the state of the LED
#define LEDISOFF PORTB&(1<<PORTB1)
*/
// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? Translate the first 3 numbers into ascii is: BOX
//static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x29};
//static uint8_t myip[4] = {192,168,1,99}; // aka http://10.0.0.29/

// server listen port for www
#define MYWWWPORT 80
extern uint8_t buf[BUFFER_SIZE+1];
extern unsigned long int WebrefreshTime0;
extern unsigned long int WebrefreshTime1;
extern Server_config server_data[2];
extern unsigned char count_ds18b2;
extern unsigned char count_ds18b2_2;
extern StrConfigParam NetworkParam;
//extern char MLogin[15];  // Login
extern char MPasswd[15]; // Password
//just taken from the BMP085 datasheet
extern int ac1;
extern int ac2;
extern int ac3;
extern unsigned int ac4;
extern unsigned int ac5;
extern unsigned int ac6;
extern int b1;
extern int b2;
extern int mb;
extern int mc;
extern int md;
#ifdef DOZIMETR	// Дозиметр 08.08.2014
extern unsigned int doz_buffer[DOZBUFSIZE+1];
#endif


static int my_putchar(char c, FILE *stream){
	while((UCSR0A&(1<<UDRE0)) == 0);
	UDR0 = c;
	return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM (my_putchar, NULL, _FDEV_SETUP_WRITE);



	
int main(void){
int a;	
		stdout = &mystdout;
		
//________________init_watchdog______________________
/* Write logical one to WDCE and WDE */
asm("WDR"); // сбрасыввем WATCHDOG
WDTCR |= (1<<WDCE) | (1<<WDE);
/* Turn off WDT */
// WDTCR = 0x07;
WDTCR = (1<<WDCE) | 0x07;
asm("WDR");
//___________________________________________________		
		
		my_wait_1ms(5);	    
		
        mcu_init();   // Инициализируем регистры процессора
		
		if (((PIND&(1<<7))>0)==0) {
			default_network(0);  // Full initialize
			while ((((PIND&(1<<7))>0) == 0)) leddebugsignal(1);
		}
		
		
		SetConfig();  // Подгружаем параметры из EEPROM

 		//eeprom_read_block(&MLogin,           (void *)eeprom_login,   15);
		eeprom_read_block(&MPasswd,          (void *)eeprom_password,  15);	
		
		
		WebrefreshTime0  = (unsigned long int)server_data[0].refresh_time*100;
		WebrefreshTime1  = (unsigned long int)server_data[1].refresh_time*100;	

#ifdef DOZIMETR			
		for (a=0; a<DOZBUFSIZE; a++) doz_buffer[a]=20;
#endif		
		
		
//-------------- Sensos configure ---------------------------------
		InitDHT(OWI_PIN_4);  // DHT
		OWI_Init(OWI_PIN_5); // 1-Wire 
		
		OWI_SearchDevices(allDevices, MAX_DEVICES, OWI_PIN_5, &count_ds18b2);

		for (a=0; a<count_ds18b2; a++)
		{
			Read_scratchpad(OWI_PIN_5,a);
			if ((char)allDevices[a].scratchpad[4] != 0x7F) Write_scratchpad(OWI_PIN_5,a);
			asm("WDR");
		}
		
		if (count_ds18b2==0) // Если нету ds18b20 поищем там DHT
		{
/*
			ReadDHT(DHTPIN2);
			my_wait_1ms(250);asm("WDR");
			my_wait_1ms(250);asm("WDR");
			ReadDHT(DHTPIN2); // Check AM2301 in DS18B20 port 10.10.2013*/
		}
		else
			Start_18b20_Convert(OWI_PIN_5);

		
//----------------------------------------------------------
	
		OWI_SearchDevices(&allDevices[count_ds18b2], MAX_DEVICES-count_ds18b2, OWI_PIN_4, &count_ds18b2_2);

		for (a=0; a<count_ds18b2_2+count_ds18b2; a++)
		{
			Read_scratchpad(OWI_PIN_4,a);
			if ((char)allDevices[a].scratchpad[4] != 0x7F) Write_scratchpad(OWI_PIN_4,a);
			asm("WDR");
		}
		
		if (count_ds18b2_2==0) // Если нету ds18b20 поищем там DHT
		{
/*
			ReadDHT(DHTPIN);
			my_wait_1ms(250);asm("WDR");
			my_wait_1ms(250);asm("WDR");
			ReadDHT(DHTPIN); // Check AM2301 in DS18B20 port 10.10.2013*/
		}
		else 
			Start_18b20_Convert(OWI_PIN_4);

		
		if (count_ds18b2_2 !=0 || count_ds18b2 != 0 ) my_wait_1ms(751); // Если есть ds18b20 делаем задержку 
		
		//--- bmp085 ----------------------------------------------
		SoftI2CInit();		
		SoftI2CStart();

		ac1=read_bmp085_int_register(0xAA);
		ac2=read_bmp085_int_register(0xAC);
		ac3=read_bmp085_int_register(0xAE);
		ac4=read_bmp085_int_register(0xB0);
		ac5=read_bmp085_int_register(0xB2);
		ac6=read_bmp085_int_register(0xB4);
		b1 = read_bmp085_int_register(0xB6);
		b2 = read_bmp085_int_register(0xB8);
		mb = read_bmp085_int_register(0xBA);
		mc = read_bmp085_int_register(0xBC);
		md = read_bmp085_int_register(0xBE);
		//--- bmp085 ----------------------------------------------

//-----------------------------------  enc28j60 initialization ------------------	
		ENC28J60_REBOOT();			
		asm("WDR");
        //initialize the hardware driver for the enc28j60
        enc28j60Init(NetworkParam.mac);
        //enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
        _delay_loop_1(0); // 60us
        enc28j60PhyWrite(PHLCON,0x476);
		my_wait_1ms(20); 
		sei();
//----------------DHCP initialization -------------- 03.06.2014 -----------------		
		if (NetworkParam.dhcp==0xFF)
		{
			init_mac(NetworkParam.mac);
			a=0;
			while(a==0){
				a=enc28j60PacketReceive(BUFFER_SIZE, buf);
				buf[BUFFER_SIZE]='\0';
				a=packetloop_dhcp_initial_ip_assignment(buf,a,NetworkParam.mac[5]);
				leddebugsignal(1);
			}
			dhcp_get_my_ip(NetworkParam.ip,NetworkParam.subnet,NetworkParam.gw);
			memcpy(NetworkParam.dns,NetworkParam.gw,4); // Делаем DNS как Gateway // 11.06.2014
			client_ifconfig(NetworkParam.ip,NetworkParam.subnet);		
		}
//----------------DHCP initialization -------------- 03.06.2014 -----------------				
		
        //init the ethernet/ip layer:
        init_udp_or_www_server(NetworkParam.mac,NetworkParam.ip);
        www_server_port(MYWWWPORT);
//-----------------------------------  enc28j60 initialization ------------------			

        while(1)
		{
			asm("WDR");
			process_http_server();
			exe_sensors(0);
			exe_relay();
        }

return (0);
}
