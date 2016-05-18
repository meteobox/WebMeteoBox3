/*
 * task_config.h
 *
 * Created: 07.04.2014 14:43:17
 *  Author: lexa
 */ 


#ifndef TASK_CONFIG_H_
#define TASK_CONFIG_H_

//-----------------------------------------------------------------------------
// AVR ADC Defination
#define ADC_SR_REG							ADCSR

#define ADC_PRESCALE_DIV2                   1
#define ADC_PRESCALE_DIV4                   2
#define ADC_PRESCALE_DIV8                   3
#define ADC_PRESCALE_DIV16                  4
#define ADC_PRESCALE_DIV32                  5
#define ADC_PRESCALE_DIV64                  6
#define ADC_PRESCALE_DIV128                 7

#define ADC_ENABLE                          0x80
#define ADC_START_CONVERSION                0x40
#define ADC_FREE_RUNNING                    0x20
#define ADC_COMPLETE                        0x10
#define ADC_INT_ENABLE                      0x08

#define ADC_REF_VREF                        0
#define ADC_REF_AVCC                        0x40
#define ADC_REF_INTERNAL                    0xC0

void default_network(char level);
void mcu_init(void);
void SetConfig(void);
void ENC28J60_RST( char ckl );
void ENC28J60_REBOOT();


#endif /* TASK_CONFIG_H_ */