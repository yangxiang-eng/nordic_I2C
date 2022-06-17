#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef void (*ht_measure_cmp_handler_t)(uint32_t measureTemp,uint32_t measureHumidity);

void HT_Init(ht_measure_cmp_handler_t);

bool HT_SensorSelftest();

void HT_startMeasure(void);

void HT_SensorStop(void);

void stop_timer();