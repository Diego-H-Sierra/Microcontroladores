#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/adc.h>

adc1_channel_t adc_pot = ADC1_CHANNEL_6;
void init_hw(void);

void app_main() {

    init_main();
    while (1)
    {
        int pot = adc1_get_raw(adc_pot);
        printf("ADC: %d\n", pot);
        vTaskDelay(300/ portTICK_PERIOD_MS);
    }
    

}

void init_hw(void){
adc1_config_width(ADC_WIDTH_BIT_11);
adc1_config_channel_atten(adc_pot, ADC_ATTEN_DB_11);

}