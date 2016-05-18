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

#include "OWIHighLevelFunctions.h"
#include "OWIPolled.h"
#include "protocol.h"

/*
#define MAX_DEVICES       11 //10
#define MAX_SENSORS       19//19 // 17 
#define MAX_RELAY   	  6 // 4*/

#define MAX_DEVICES       6 //10
#define MAX_SENSORS       6//19 // 17
#define MAX_RELAY   	  1 // 4*/


#define SIZE_SENSOR_STRUCT  sizeof(sensor_structure)
#define SIZE_RELAY_STRUCT   sizeof(io_structure)


//#define BUS   		      OWI_PIN_5 // нужно OWI_PIN_4  21.12.12
// Порты датчиков
// PORT D pin 1 
// PORT D pin 2

#define DS18B20_FAMILY_ID                0x28 
#define DS18B20_CONVERT_T                0x44
#define DS18B20_READ_SCRATCHPAD          0xbe
#define DS18B20_WRITE_SCRATCHPAD         0x4e
#define DS18B20_COPY_SCRATCHPAD          0x48
#define DS18B20_RECALL_E                 0xb8
#define DS18B20_READ_POWER_SUPPLY        0xb4

#define READ_SUCCESSFUL   0x00
#define READ_CRC_ERROR    0x01

unsigned char DS18B20_ReadTemperature(unsigned char bus, unsigned char * id, unsigned int* temperature);
unsigned char DS18B20_ReadTemperature_Fast_Float(unsigned char bus, unsigned char * id, float* temperature);
void DS18B20_to_float(unsigned int temperature, float * out);
void DS18B20_PrintTemperature(unsigned int temperature, char * out);
unsigned char Read_scratchpad(unsigned char bus, unsigned char num);
unsigned char Write_scratchpad(unsigned char bus, unsigned char num);
void readsingle();
void my_wait_1ms(int cnt);
void Start_18b20_Convert(unsigned char bus);

OWI_device allDevices[MAX_DEVICES]; // Для хранения ds18b20
unsigned char count_sensors; // Сколько всего датчиков подключено в систему.
sensor_structure all_sensors[MAX_SENSORS]; // Все сенсоры
io_structure all_relay[MAX_RELAY]; // Сколько всего реле
