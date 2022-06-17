#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include "stroge_data.h"
#define  DETLA_TEMPERATURE 40 ; 
typedef  struct storget{

  uint16_t temp_data ; 
  uint32_t adc_to_temp_data ; 
}storget_data ; 

static uint32_t temp1_value , temp2_value ; 

static storget_data gap_target_data[]={
  {0,95178} ,
  {1,94912} ,
  {2,94634} ,
  {3,94344} ,
  {4,94041} ,
  {5,93726} ,
  {6,93400} ,
  {7,93060} ,
  {8,92707} ,
  {9,92339} ,
  {10,91958} ,
  {11,91563} ,
  {12,91154} ,
  {13,90730} ,
  {14,90289} ,
  {15,89835} ,
  {16,89366} ,
  {17,88880} ,
  {18,88379} ,
  {19,87862} ,
  {20,87328} ,
  {21,86778} ,
  {22,86212} ,
  {23,85630} ,
  {24,85031} ,
  {25,84415} ,
  {26,83783} ,
  {27,83135} ,
  {28,82470} ,
  {29,81789} ,
  {30,81092} ,
  {31,80379} ,
  {32,79651} ,
  {33,78907} ,
  {34,78147} ,
  {35,77374} ,
  {36,76585} ,
  {37,75782} ,
  {38,74966} ,
  {39,74136} ,
  {40,73294} ,
  {41,72439} ,
  {42,71572} ,
  {43,70695} ,
  {44,69808} ,
  {45,68910} ,
  {46,68003} ,
  {47,67088} ,
  {48,66165} ,
  {49,65234} ,
  {50,64298} ,
  {51,63357} ,
  {52,62409} ,
  {53,61458} ,
  {54,60616} ,
  {55,59547} ,
  {56,58589} ,
  {57,57629} ,
  {58,56669} ,
  {59,55709} ,
  {60,54749} ,
  {61,53794} ,
  {62,52839} ,
  {63,51888} ,
  {64,50942} ,
  {65,50000} ,
  {66,49063} ,
  {67,48133} ,
  {68,47210} ,
  {69,46291} ,
  {70,45382} ,
  {71,44481} ,
  {72,43589} ,
  {73,42703} ,
  {74,41830} ,
  {75,40954} ,
  {76,40113} ,
  {77,39269} ,
  {78,38435} ,
  {79,37613} ,
  {80,36805} ,
  {81,36008} ,
  {82,35221} ,
  {83,34448} ,
  {84,33687} ,
  {85,32935} ,
  {86,31820} ,
  {87,31092} ,
  {88,30762} ,
  {89,30065} ,
  {90,29379} ,
  {91,28709} ,
  {92,28047} ,
  {93,27399} ,
  {94,26761} ,
  {95,26139} ,
  {96,25534} ,
  {97,24936} ,
  {98,24351} ,
  {99,23775} ,
  {100,23219} ,
  {101,22666} ,
  {102,22131} ,
  {103,21606} ,
  {104,21092} ,
  {105,20591} ,
  {106,20102} ,
  {107,19621} ,
  {108,19153} ,
  {109,18699} ,
  {110,18254} ,
  {111,17817} ,
  {112,17390} ,
  {113,16971} ,
  {114,16569} ,
  {115,16171} ,
  {116,15782} ,
  {117,15060} ,
  {118,14690} ,
  {119,14332} ,
  {120,14332} , 
}; 
static float adc_value_to_temp_situation(uint32_t big_measure_value , uint32_t small_measure_value ,uint16_t samle_value_temp,int target);
float search_data(int target)
{
  //target *100 ; 
   int target_val = target * 100 ; 

  //int 
   int left = 0 ;
   int right = sizeof(gap_target_data) / sizeof(gap_target_data[0]) -1 ;
   while(left < right)
   {
      int mid = (left + right) /2 ; 
      if(gap_target_data[mid].adc_to_temp_data == target_val)
      {
        //printf("one temp :%d\n",gap_target_data[mid].temp_data) ; 
        //adc_value_to_temp_situation2(gap_target_data[mid].temp_data);
        //NRF_LOG_INFO("one temp:%d\n",gap_target_data[mid].temp_data);
        return  gap_target_data[mid].temp_data ; 
      }
      //当前值大于目标值并且下一个值小于目标值
      else if(gap_target_data[mid].adc_to_temp_data > target_val && gap_target_data[mid+1].adc_to_temp_data < target_val)
      {
        //printf("two temp,temp1 :%d,temp2:%d\n",gap_target_data[mid].temp_data ,gap_target_data[mid+1].temp_data );

        return adc_value_to_temp_situation(gap_target_data[mid].adc_to_temp_data,gap_target_data[mid+1].adc_to_temp_data,gap_target_data[mid].temp_data,target_val);
      }
      //当前值小于目标值并且上一个值大于目标值
      else if(gap_target_data[mid].adc_to_temp_data < target_val && gap_target_data[mid-1].adc_to_temp_data > target_val)
      {
        //printf("two temp_value , value1:%d,value2:%d\n",gap_target_data[mid-1].temp_data,gap_target_data[mid].temp_data);
        
        return adc_value_to_temp_situation(gap_target_data[mid-1].adc_to_temp_data,gap_target_data[mid].adc_to_temp_data,gap_target_data[mid-1].temp_data,target_val);
      }
      else if(gap_target_data[mid].adc_to_temp_data > target_val)
      {
        left = mid +1 ; 
      }
      else
      {
        right = mid -1 ; 
      }
   }
}

//number1 > number2 and number1 > target , number2 < target , val == number2.temp

//onec situation
static float adc_value_to_temp_situation(uint32_t big_measure_value , uint32_t small_measure_value ,uint16_t samle_value_temp,int target)
{
  float ans = ((float)(target - small_measure_value))/(float)((big_measure_value - small_measure_value)) + samle_value_temp - DETLA_TEMPERATURE ;
  //NRF_LOG_INFO("measure temp value  :%d\n",(int)(ans * 100)); 
  return ans ; 
}

