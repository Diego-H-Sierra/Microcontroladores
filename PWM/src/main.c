#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <driver/ledc.h>
adc1_channel_t adc_pot = ADC1_CHANNEL_6;
#define LED 2
void init_hw(void);

void app_main()
{

    init_hw();
    while (1)
    {
        int pot = adc1_get_raw(adc_pot);
        printf("Voltaje: %fV\n", (pot*3.3/2047));

        vTaskDelay(500 / portTICK_PERIOD_MS);

    }
}

void init_hw(void)
{
    adc1_config_width(ADC_WIDTH_BIT_11);
    adc1_config_channel_atten(adc_pot, ADC_ATTEN_DB_11);

    gpio_config_t io_config;
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.pin_bit_mask = (1 << LED);
    io_config.pull_down_en = GPIO_PULLDOWN_ONLY;
    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    io_config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_config);


}
