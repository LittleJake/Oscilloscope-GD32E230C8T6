#include "gd32e23x.h"
#include "systick.h"
#include <string.h>
#include <stdio.h>
#include "led.h"
#include "tft.h"
#include "tft_init.h"
#include "adc.h"
#include "usart.h"
#include "timer.h"
#include "key.h"
#include "freq.h"
#include "main.h"
#include "img.h"

volatile struct Oscilloscope oscilloscope={0};

void Init_Oscilloscope(volatile struct Oscilloscope *value);

int main(void)
{      
    uint16_t i=0;
    
    //中间值
    float median=0;
    
    //峰峰值
    float voltage=0;
    
    //触发电压值
    float max_data=1.0f;
    
    //波形放大倍数
    float gainFactor=0;
	
		float adcValue = 0;
    
    //触发沿标记
    uint16_t Trigger_number=0;
    
    //初始化示波器参数
    Init_Oscilloscope(&oscilloscope);
    
    //时钟初始化
    systick_config();
    
    //LED初始化
    Init_LED_GPIO();
    
    //屏幕初始化
    TFT_Init();
    
		
    //填充白色
    IMG_Bootload();
    TFT_Fill(0,0,160,128,WHITE);
    
    //初始化串口
    Init_USART(115200);
    
    //ADC DMA初始化
    ADC_DMA_Init();
    
    //初始化ADC引脚
    Init_ADC();
    
    //初始化PWM输出
    Init_PWM_Output(oscilloscope.timerPeriod-1,oscilloscope.pwmOut);
    
    //初始化EC11引脚
    Init_EC11_GPIO();
    
    //初始化按键引脚
    Init_Key_GPIO();
    
    //初始化频率定时器2
    Init_FreqTimer();
		
		
    //初始化静态UI
    TFT_StaticUI();
    while(1)
    {  
        //按键扫描处理函数
        Key_Handle(&oscilloscope);
        
        //如果获取电压值完成，开始刷屏
        if(oscilloscope.showbit==1)
        {           
            oscilloscope.showbit=0;
            oscilloscope.vpp=0;
            
            //转换电压值
            for(i=0;i<300;i++)
            {
								adcValue = (Get_ADC_Value(i)*3.3f)/4096.0f;
							
								oscilloscope.voltageValue[i] = (5-(2.0f*adcValue));
							
                if((oscilloscope.vpp) < oscilloscope.voltageValue[i])
                {
                    oscilloscope.vpp = oscilloscope.voltageValue[i];
                }
                if(oscilloscope.vpp <= 0.3)
                {
                    oscilloscope.gatherFreq=0;
                }
            }
            
            //刷屏的同时获取电压值
            dma_transfer_number_config(DMA_CH0, 300);
            dma_channel_enable(DMA_CH0);
            
            //找到起始显示波形值
            for(i=0;i<200;i++)
            {
                if(oscilloscope.voltageValue[i] < max_data)
                {
                    for(;i<200;i++)
                    {
                        if(oscilloscope.voltageValue[i] > max_data)
                        {
                            Trigger_number=i;
                            break;
                        }
                    }
                    break;
                }
            }
            
            //如果幅值过小，会出现放大倍数过大导致波形显示异常的问题
            if(oscilloscope.vpp > 0.3)
            {
								//获取中间值
								median = oscilloscope.vpp / 2.0f;
							
                //放大倍数，需要确定放大之后的区间，我将波形固定显示在（18.75~41.25中），(41.25-18.75)/2=11.25f
                gainFactor = 11.25f/median;
              
            }
            
            //依次显示后续100个数据，这样可以防止波形滚动
            for(i=Trigger_number;i<Trigger_number+100;i++)
            {
                if(oscilloscope.keyValue == KEYD)
                {
                    oscilloscope.keyValue=0;
                    do
                    {
                        if(oscilloscope.keyValue == KEYD){
                            oscilloscope.keyValue=0;
                            break;
                        }
                    }while(1);
                }
                voltage=oscilloscope.voltageValue[i];

                if(voltage >= median)
                {
                    voltage = 30 - (voltage - median)*gainFactor;
                }
                else
                {
                    voltage = 30 + (median - voltage)*gainFactor;
                }
                drawCurve(80,voltage);
            }          
        }        
        //参数显示UI
        TFT_ShowUI(&oscilloscope); 
        
    }
}

/*
*   函数内容：初始化示波器参数结构体
*   函数参数：volatile struct Oscilloscope *value--示波器参数结构体指针
*   返回值：无
*/
void Init_Oscilloscope(volatile struct Oscilloscope *value)
{
    (*value).showbit    =0;                         //清除显示标志位
    (*value).sampletime =ADC_SAMPLETIME_239POINT5;  //adc采样周期
    (*value).keyValue   =0;                         //清楚按键值
    (*value).ouptputbit =0;                         //输出标志位
    (*value).gatherFreq =0;                         //采集频率
    (*value).outputFreq =1000;                      //输出频率
    (*value).pwmOut     =500;                       //PWM引脚输出的PWM占空比
    (*value).timerPeriod=1000;                      //PWM输出定时器周期
    (*value).vpp        =0.0f;                      //峰峰值
}


