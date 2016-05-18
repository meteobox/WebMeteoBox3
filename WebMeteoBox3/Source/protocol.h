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


#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define BUFFER_SIZE 1430 // Макс что смог отправить! 

//#define ENABLE_CUSTOM_REQUEST
#define ADD_LOGGER_2 /* включает использование второго логгера  12.08.2014 */
//#define DEBUG_METEOBOX       /* При отладке включаем */
//#define DEBUG_METEOBOX_TIMER /* Добавляем к датчикам внутренний таймер, для отладки стабильности работы */
//#define METEOBOX_DEMO_MODE   /* Включаем если хотим сделать DEMO прошивку. с защитой от зименения ip и прочих параметров */
//#define SCT_013_030          /* Включаем если есть токовые клещи SCT-013-030 */
//#define SCT_VIRTUAL_SENSOR   /* Включаем Для создания виртуального суммирующего датчика */
//#define VOLT_METER         /* Включаем если есть датчик сетевого напряжения */

#define DOZIMETR             /* Дозиметр */

//#define NO_BREND


#define eeprom_config_s		0x81
//___________________________________________________________________________

#define VREF         3.30

#define CONV_FACTOR 0.005 /* Коэфициент для дозиметра */ //0.0057//0.00812

#define FIRMWARE_VERSION "1.0.3 SL@VA 24.03.2015"



#define EEPOP						   0
#define SPECIALOP					   0x82

#define eeprom_login                   0x40        /* добавлено 18.10.2010 */
#define eeprom_password                0x50		/* добавлено 18.10.2010 */
#define eeprom_server_config           0x0A0 // 0a1
#define eeprom_sensors_config          0x1A0 // Адрес для сохранения параметров датчиков
#define eeprom_relay_config            0x1A0+MAX_SENSORS*SIZE_SENSOR_STRUCT // Адрес для сохранения параметров датчиков
#define eeprom_custom_request_config   eeprom_relay_config+MAX_RELAY*SIZE_RELAY_STRUCT // Адрес для сохранения специального запроса
#define eeprom_custom_password         eeprom_custom_request_config + sizeof(custom_request) // Адрес для сохранения пароля Authorization: Basic  
#define DOZBUFSIZE                     10  // Буфер дозиметра

#define CONFIG_CLIENT_PORT		       1444 
#define CONFIG_SERVER_PORT			   8444 



#define GoBoot()	do{								\
	MCUCR |= _BV(IVCE);			\
	MCUCR |= _BV(IVSEL);		\
	((void (*)())0x1e000)();	\
}while(0)

#define GoApp()		do{								\
	MCUCR |= _BV(IVCE);		\
	MCUCR &= ~_BV(IVSEL);		\
	((void (*)())0x0000)();		\
}while(0)


typedef struct
{
	unsigned char op[4];
	unsigned char ver[2];
	unsigned char mac[6];
	unsigned char ip[4];
	unsigned char subnet[4];
	unsigned char gw[4];
	unsigned char dns[4]; // 17.12.2012
	unsigned char dhcp;
} StrConfigParam;

typedef struct
{
	unsigned char enable;
	unsigned int  refresh_time;
	unsigned char server_name[30];
	//unsigned char server_ip[4];
	unsigned char script_path[50];
	unsigned int  port;
} Server_config;

typedef struct  // Будем хранить это в EEPROM! 18.02.2013
{
	unsigned char id[8];           //!< The 64 bit identifier.
	unsigned char flag;			   // Разрешить использование?	0=off,
	float         min;             // min
	float         max;             // max
	unsigned char pio;            // номер порта atmega
	//unsigned char opearnd;	       // ???
} io_structure; // структура реле


void	 leddebugsignal (char a);
void     LED_ON_OFF( char ckl );
void     exe_relay();
void     process_http_server();
void     exe_sensors(char flag);
uint16_t http2002ok(void);
uint16_t print_webpage_login(uint8_t *buf);


#endif /* PROTOCOL_H_ */