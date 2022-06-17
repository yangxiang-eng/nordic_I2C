#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sht4x_i2c.h"
#include <stdio.h>
#include <stdbool.h>
#include "app_timer.h"


static uint32_t temperatrue =10; 
static uint32_t humidity =10 ; 
APP_TIMER_DEF(sensor_timer);
APP_TIMER_DEF(serial_number_timer);
APP_TIMER_DEF(high_precision_timer);

void sensor_timerout_handler(){
  HT_startMeasure();
}

void high_precision_timer_handler(){
  sht4x_measure_high_precision(&temperatrue,&humidity);
  temperatrue /= 1000 ; 
  humidity /= 1000 ; 

  printf("temp:%d,hum:%d\n",temperatrue,humidity);
}

static void timers_init(){
  //ret_code_t err_code = app_timer_init();
  ret_code_t err_code ; 
  //APP_ERROR_CHECK(err_code);

  err_code = app_timer_create(&sensor_timer,APP_TIMER_MODE_REPEATED,sensor_timerout_handler);
  APP_ERROR_CHECK(err_code);
}

void serial_number_timer_handler(){
    uint32_t serial_number;
    sht4x_serial_number_getdata(&serial_number);
    printf("serial_number_value:%d",serial_number);
}




void HT_Init(ht_measure_cmp_handler_t){
  sensirion_i2c_hal_init();
  timers_init();
  
}

bool HT_SensorSelftest(){
  if(temperatrue<0 || temperatrue>200){
    return false ; 
  }
  if(humidity >200 || humidity <0){
    return false ; 
  }

  int16_t error = 0 ; 
  error = sht4x_serial_number();
  app_timer_create(&serial_number_timer,APP_TIMER_MODE_SINGLE_SHOT,serial_number_timer_handler);
  app_timer_start(serial_number_timer,10000,NULL);

  if(error){
    return false ; 
  }
  return true ; 
}

void HT_startMeasure(){
  int16_t error = 0 ; 
  sht4x_measure_high_precision_ticks_write();
  app_timer_start(sensor_timer,50000,NULL);
  app_timer_create(&high_precision_timer,APP_TIMER_MODE_SINGLE_SHOT,high_precision_timer_handler);
  app_timer_start(high_precision_timer,10000,NULL);
 
}

void HT_SensorStop(){
  sht4x_soft_reset();
}

void stop_timer(){
  app_timer_stop(sensor_timer);
}