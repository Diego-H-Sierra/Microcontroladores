#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <driver/adc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <esp_log.h>

// Definiciones
#define ADC_CHANNEL ADC1_CHANNEL_0 // Canal ADC que estamos usando (GPIO 36)
#define NUM_MUESTRAS 100

// Variables globales
static const char *TAG = "ADC_RMS";
volatile int muestra_actual = 0;
volatile float suma_cuadrados = 0;
TimerHandle_t xTimer;
int timerID = 1;

// Prototipos de funciones
void configurar_adc(void);
void iniciar_timer(void);
void vTimerCallback(TimerHandle_t xTimer);
float calcular_rms(void);
void mostrar_resultado(float valor);

void app_main(void) {
    configurar_adc();
    iniciar_timer();

    // Bucle principal vacío porque todo se maneja en la interrupción
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Espera 1 segundo
    }
}

// Configuración del ADC
void configurar_adc(void) {
    adc1_config_width(ADC_WIDTH_BIT_12); // Configura resolución de 12 bits
    adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB_0); // Configura la atenuación del canal
}

// Configuración del Timer
void iniciar_timer(void) {
    ESP_LOGI(TAG, "Inicializando configuración del timer...");

    xTimer = xTimerCreate("Timer",
                          pdMS_TO_TICKS(1), // 1 ms
                          pdTRUE, // Repetir
                          (void *)timerID,
                          vTimerCallback);

    if (xTimer == NULL) {
        ESP_LOGE(TAG, "El timer no fue creado");
    } else {
        if (xTimerStart(xTimer, 0) != pdPASS) {
            ESP_LOGE(TAG, "El timer no pudo ser activado");
        }
    }
}

// Función de callback del Timer
void vTimerCallback(TimerHandle_t xTimer) {
    int adc_reading = adc1_get_raw(ADC_CHANNEL); // Tomar muestra con el ADC
    float voltaje = (adc_reading / 4095.0) * 3.3; // Convertir lectura ADC a voltaje (3.3V rango)

    // Elevar al cuadrado y sumar
    suma_cuadrados += (voltaje * voltaje);
    muestra_actual++;

    // Si alcanzamos 100 muestras, calcular RMS
    if (muestra_actual >= NUM_MUESTRAS) {
        float rms = calcular_rms();
        mostrar_resultado(rms);

        // Resetear variables para la siguiente ronda
        muestra_actual = 0;
        suma_cuadrados = 0;
    }
}

// Calcular RMS
float calcular_rms(void) {
    float promedio_cuadrados = suma_cuadrados / NUM_MUESTRAS;
    return sqrt(promedio_cuadrados);
}

// Mostrar el resultado (puede ser por Serial, LCD, Wi-Fi, etc.)
void mostrar_resultado(float valor) {
    ESP_LOGI(TAG, "RMS del voltaje: %.2f V", valor);
    // Aquí puedes agregar código para enviar el resultado a un LCD, Wi-Fi, o display de 7 segmentos
}
