#include "temp_adc.h"
#include "nrf_drv_saadc.h"
#include "app_timer.h"
#include "nrf_log.h"
#include "stroge_data.h"

//GPIO define
#define TP1_CH_ADC  NRF_SAADC_INPUT_AIN5
#define TP2_CH_ADC  NRF_SAADC_INPUT_AIN4
#define TP_POWER_SWITCH 3

//基于时间缓冲区队列
#define MAX_TIME_FILTER_NUM 3
static int gap_time_filter_list[MAX_TIME_FILTER_NUM] = {-1};
static int gap_time_filter_index = 0;   //当前更新的数据
static int gapMeasureAverageRate = 0;

//单次测量缓存数据
#define MAX_SHOT_FILTER_NUM 3
static int gap_shot_result_ist[MAX_SHOT_FILTER_NUM] = {-1};
static int gap_shot_data_num = 0;
static int gapShotAverateRate = 0;

//adc buffer
#define START_MEASURE 0
#define ADC_SUCCESS 1
#define ADC_FAILED 2
#define SAMPLES_IN_BUFFER 2
static nrf_saadc_value_t     m_buffer_pool[2][SAMPLES_IN_BUFFER];
static int m_adc_evt_counter = 0;

//测量定时器
APP_TIMER_DEF(m_temp_timer_id); 

//measure delay
static void measure_delay_timer_handler(void*p);
static void clear_time_filter(void);
static void saadc_driver_start(void);
static void saadc_driver_stop(void);

static temp_measure_handler_t gap_measure_callback = NULL;

//init sensor
void Temp_Init(temp_measure_handler_t callback)
{
    int err_code = app_timer_create(&m_temp_timer_id,\
                                APP_TIMER_MODE_SINGLE_SHOT,measure_delay_timer_handler);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_cfg_output(TP_POWER_SWITCH);
    nrf_gpio_pin_set(TP_POWER_SWITCH);

    //init filter buffer
    clear_time_filter();
    gap_measure_callback = callback;
}

//清除基于时间过滤的数据
static void clear_time_filter(void)
{
    for (int i = 0; i < MAX_TIME_FILTER_NUM; i++)
    {
        gap_time_filter_list[i] = -1;
    }
    gap_time_filter_index = 0;
}

//网时间过滤队列中追加数据
static int append_2_time_filter(int currentRate)
{
    //filter data
    if (gap_time_filter_index < MAX_TIME_FILTER_NUM)
    {
        gap_time_filter_list[gap_time_filter_index] = currentRate;
    }

    gap_time_filter_index++;
    gap_time_filter_index = gap_time_filter_index % MAX_TIME_FILTER_NUM;

    //计算基于时间过滤的结果
    int totalRecordRate = 0, totalRecordNum = 0;
    for (int i = 0; i < MAX_TIME_FILTER_NUM; i++)
    {
        if (gap_time_filter_list[i] != -1)
        {
            totalRecordRate = totalRecordRate + gap_time_filter_list[i];
            totalRecordNum++;
        }
    }
    if (totalRecordNum > 0)
    {
        return totalRecordRate / totalRecordNum;
    }
    else
    {
        return currentRate;
    }
}


//系统回调，采样结束
static void saadc_done_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        int err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, SAMPLES_IN_BUFFER);
        APP_ERROR_CHECK(err_code);

        int currentRate = 0;
        if (p_event->data.done.size < 2)
        {
            saadc_driver_stop();
            NRF_LOG_INFO("adc failed");
            app_timer_start(m_temp_timer_id, APP_TIMER_TICKS(1), (void*)ADC_FAILED);
            return;
        }

        //debug only
        int tp1Voltage = (p_event->data.done.p_buffer[0] * ADC_REF_VOLTAGE_IN_MILLIVOLTS * ADC_PRE_SCALING_COMPENSATION / ADC_RES_12BIT);
        int tp2Voltage = (p_event->data.done.p_buffer[1] * ADC_REF_VOLTAGE_IN_MILLIVOLTS * ADC_PRE_SCALING_COMPENSATION / ADC_RES_12BIT);
        

        //放大1000倍
        currentRate = (p_event->data.done.p_buffer[0] * 1000) / p_event->data.done.p_buffer[1];

        //check if shot adc complete
        if (gap_shot_data_num < MAX_SHOT_FILTER_NUM)
        {
            gap_shot_result_ist[gap_shot_data_num] = currentRate;
            gap_shot_data_num++;

            NRF_LOG_INFO("current measure result:%d, tp1:%d, tp2:%d, rate:%d", gap_shot_data_num, tp1Voltage, tp2Voltage, currentRate);

            //continue adc
            nrf_drv_saadc_sample();
        }
        else
        {
            //stop adc
            saadc_driver_stop();

            //calc shot average rate
            int totalRate = 0;
            for (int i = 0; i < MAX_SHOT_FILTER_NUM; i++)
            {
                totalRate = totalRate + gap_shot_result_ist[i];
            }
            gapShotAverateRate = totalRate / MAX_SHOT_FILTER_NUM;

            //add to time filter
            gapMeasureAverageRate = append_2_time_filter(gapShotAverateRate);
            app_timer_start(m_temp_timer_id, APP_TIMER_TICKS(1), (void*)ADC_SUCCESS);
            
            //get temp_data 
            float res = search_data((int)gapMeasureAverageRate);
            NRF_LOG_INFO("MEASURE TEMP VALUE * 100:%d\n",(int)(res*100));
            //to timer
            NRF_LOG_INFO("%d, tp1:%d, rate:%d, shotRate:%d, filterRate:%d", 
                m_adc_evt_counter++,
                tp1Voltage,
                currentRate,
                gapShotAverateRate, 
                gapMeasureAverageRate);

        }
    }
}

//启动采样
static void saadc_driver_start(void)
{
    ret_code_t err_code;
    nrfx_saadc_config_t adc_config = NRFX_SAADC_DEFAULT_CONFIG;
    adc_config.resolution = NRF_SAADC_RESOLUTION_12BIT;
    //adc_config.low_power_mode = true;

    err_code = nrf_drv_saadc_init(&adc_config, saadc_done_callback);
    APP_ERROR_CHECK(err_code);

    //tp1 channel
    nrf_saadc_channel_config_t tp1_adc =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(TP1_CH_ADC);
    tp1_adc.gain = NRF_SAADC_GAIN1_4;
    err_code = nrf_drv_saadc_channel_init(0, &tp1_adc);
    APP_ERROR_CHECK(err_code);

    //tp2 channel
    nrf_saadc_channel_config_t tp2_adc =
        NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(TP2_CH_ADC);
    tp2_adc.gain = NRF_SAADC_GAIN1_4;
    err_code = nrf_drv_saadc_channel_init(1, &tp2_adc);
    APP_ERROR_CHECK(err_code);



    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[0], SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_saadc_buffer_convert(m_buffer_pool[1], SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);

    gap_shot_data_num = 0;
    nrf_drv_saadc_sample();
}

//停止采样
static void saadc_driver_stop(void)
{
    //uninstall adc driver
    nrf_drv_saadc_uninit();
    NRF_SAADC->INTENCLR = (SAADC_INTENCLR_END_Clear << SAADC_INTENCLR_END_Pos);
    NVIC_ClearPendingIRQ(SAADC_IRQn);

    //power off adc
    nrf_gpio_pin_set(TP_POWER_SWITCH);
    gap_shot_data_num = 0;
}

static void measure_delay_timer_handler(void*p)
{
    uint32_t cmdType = (uint32_t)p;
    if (cmdType == START_MEASURE)
    {
        NRF_LOG_INFO("start measure");
        saadc_driver_start();
    }
    else if (cmdType == ADC_SUCCESS)
    {
        NRF_LOG_INFO("measure compelte, shot:%d, avg:%d", gapShotAverateRate, gapMeasureAverageRate);
        gap_measure_callback(TEMP_MEASURE_SUCCESS, gapShotAverateRate, gapMeasureAverageRate);
    }
    else if (cmdType == ADC_FAILED)
    {
        gap_measure_callback(TEMP_MEASURE_FAILED, -1, -1);
    }
}

//self test
bool Temp_SensorSelftest(void)
{
    
}

//start measure
uint32_t Temp_startMeasure(void)
{
    nrf_gpio_pin_clear(TP_POWER_SWITCH);

    //上电等待十毫秒，稳定
    app_timer_start(m_temp_timer_id, APP_TIMER_TICKS(ADV_STABLE_TIME_MS), (void*)START_MEASURE);

    return 0;
}

//stop sensor
void Temp_SensorStop(void)
{
    clear_time_filter();
    nrf_gpio_pin_set(TP_POWER_SWITCH);
}