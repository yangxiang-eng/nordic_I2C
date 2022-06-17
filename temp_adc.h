#ifndef TEMP_ADC_H
#define TEMP_ADC_H
#include <stdint.h>
#include <stdbool.h>
#include "boards.h"

#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600                                     /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION    4 
#define ADC_RES_12BIT                   4096 
#define ADV_STABLE_TIME_MS 10

typedef enum
{
    TEMP_MEASURE_FAILED = 1,
    TEMP_MEASURE_SUCCESS = 0,
}ETempMeasureResult;

typedef void (*temp_measure_handler_t) (ETempMeasureResult nMeasureRslt, float shotTemp, float averageTemp);

//init sensor
void Temp_Init(temp_measure_handler_t callback);

//self test
bool Temp_SensorSelftest(void);

//start measure
uint32_t Temp_startMeasure(void);

//stop sensor
void Temp_SensorStop(void);


#endif