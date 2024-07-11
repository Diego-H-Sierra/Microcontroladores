#include <stdio.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/timers.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include "esp_log.h"
#include "i2c-lcd.h"

// Definiciones de estados
typedef enum {
    INIT,
    ABRIENDO,
    CERRANDO,
    CERRADO,
    ABIERTO,
    EMERGENCIA,
    ERROR,
    ESPERA
} estado_t;

// Variables globales y configuración
static const char *TAG = "Main";
static TimerHandle_t xTimer;
int timerID = 1;
const int INTERVALO = 50;

volatile estado_t estado_actual = INIT;
volatile estado_t estado_siguiente = INIT;
volatile estado_t estado_anterior = INIT;
volatile unsigned int contador = 0;

volatile struct {
    unsigned int LSA : 1;
    unsigned int LSC : 1;
    unsigned int CA : 1;
    unsigned int CC : 1;
    unsigned int FC : 1;
} inputs;

volatile struct {
    unsigned int MC : 1;
    unsigned int MA : 1;
    unsigned int LED_EMERGENCIA : 1;
    unsigned int LED_MOVIMIENTO : 1;
} outputs;

// Prototipos de funciones
esp_err_t init_i2c(void);
esp_err_t configurar_timer(void);
void callback_timer(TimerHandle_t pxTimer);
esp_err_t manejar_interrupcion(void);

// Prototipos de funciones de estado
estado_t estado_init(void);
estado_t estado_abriendo(void);
estado_t estado_cerrando(void);
estado_t estado_cerrado(void);
estado_t estado_abierto(void);
estado_t estado_espera(void);
estado_t estado_emergencia(void);
estado_t estado_error(void);

void app_main()
{
    ESP_ERROR_CHECK(init_i2c());
    ESP_LOGI(TAG, "I2C inicializado exitosamente");

    estado_siguiente = estado_init();
    configurar_timer();

    lcd_put_cur(0, 0);
    lcd_send_string("Albert Cabra");
    lcd_put_cur(1, 0);
    lcd_send_string("PUERTON.");
    lcd_clear();

    while (1)
    {
        switch (estado_siguiente)
        {
        case INIT:
            estado_siguiente = estado_init();
            break;
        case ESPERA:
            estado_siguiente = estado_espera();
            break;
        case ABRIENDO:
            estado_siguiente = estado_abriendo();
            break;
        case CERRANDO:
            estado_siguiente = estado_cerrando();
            break;
        case CERRADO:
            estado_siguiente = estado_cerrado();
            break;
        case ABIERTO:
            estado_siguiente = estado_abierto();
            break;
        case EMERGENCIA:
            estado_siguiente = estado_emergencia();
            break;
        case ERROR:
            estado_siguiente = estado_error();
            break;
        default:
            ESP_LOGE(TAG, "Estado desconocido: %d", estado_siguiente);
            estado_siguiente = ERROR;
            break;
        }
    }
}

estado_t estado_init(void)
{
    ESP_LOGI(TAG, "Iniciando programa");
    lcd_put_cur(0, 0);
    lcd_send_string("INICIANDO");

    estado_anterior = estado_actual;
    estado_actual = INIT;

    ESP_LOGI(TAG, "Configuración de pines");
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << 13) | (1ULL << 12) | (1ULL << 14) | (1ULL << 27) | (1ULL << 26);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << 4) | (1ULL << 16) | (1ULL << 17) | (1ULL << 5);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    return ESPERA;
}

estado_t estado_abriendo(void)
{
    ESP_LOGI(TAG, "Abriendo el portón");
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("ABRIENDO");
    lcd_put_cur(1, 0);
    lcd_send_string("PUERTON");

    estado_anterior = estado_actual;
    estado_actual = ABRIENDO;

    outputs.LED_MOVIMIENTO = 1;
    outputs.LED_EMERGENCIA = 0;
    outputs.MA = 1;
    outputs.MC = 0;

    while (1)
    {
        manejar_interrupcion();

        if (inputs.LSA)
        {
            return ABIERTO;
        }
        if (inputs.LSA && inputs.LSC)
        {
            return ERROR;
        }
        if (inputs.FC)
        {
            return EMERGENCIA;
        }
        if (inputs.CC)
        {
            return CERRANDO;
        }
        if (contador >= 3600)
        {
            return ERROR;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

estado_t estado_cerrando(void)
{
    ESP_LOGI(TAG, "Cerrando el portón");
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("CERRANDO");
    lcd_put_cur(1, 0);
    lcd_send_string("PUERTON");

    estado_anterior = estado_actual;
    estado_actual = CERRANDO;

    outputs.LED_MOVIMIENTO = 1;
    outputs.LED_EMERGENCIA = 0;
    outputs.MA = 0;
    outputs.MC = 1;

    while (1)
    {
        manejar_interrupcion();

        if (inputs.LSC)
        {
            return CERRADO;
        }
        if (inputs.LSA && inputs.LSC)
        {
            return ERROR;
        }
        if (inputs.FC)
        {
            return EMERGENCIA;
        }
        if (inputs.CA)
        {
            return ABRIENDO;
        }
        if (contador >= 3600)
        {
            return ERROR;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

estado_t estado_cerrado(void)
{
    ESP_LOGI(TAG, "Portón cerrado");
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("PUERTON");
    lcd_put_cur(1, 0);
    lcd_send_string("CERRADO");

    estado_anterior = estado_actual;
    estado_actual = CERRADO;

    outputs.LED_MOVIMIENTO = 0;
    outputs.LED_EMERGENCIA = 0;
    outputs.MA = 0;
    outputs.MC = 0;

    return ESPERA;
}

estado_t estado_abierto(void)
{
    ESP_LOGI(TAG, "Portón abierto");
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("PUERTON");
    lcd_put_cur(1, 0);
    lcd_send_string("ABIERTO");

    estado_anterior = estado_actual;
    estado_actual = ABIERTO;

    outputs.LED_MOVIMIENTO = 0;
    outputs.LED_EMERGENCIA = 0;
    outputs.MA = 0;
    outputs.MC = 0;

    return ESPERA;
}

estado_t estado_emergencia(void)
{
    ESP_LOGE(TAG, "¡Emergencia!");
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("EMERGENCIA!!!");

    estado_anterior = estado_actual;
    estado_actual = EMERGENCIA;

    outputs.LED_MOVIMIENTO = 0;
    outputs.LED_EMERGENCIA = 1;
    outputs.MA = 0;
    outputs.MC = 0;

    while (1)
    {
        manejar_interrupcion();

        vTaskDelay(1500 / portTICK_PERIOD_MS);
        if (!inputs.FC)
        {
            return estado_anterior;
        }
    }
}

estado_t estado_error(void)
{
    ESP_LOGE(TAG, "¡Error!");
    lcd_clear();
    lcd_put_cur(0, 3);
    lcd_send_string("ERROR!!!");
    lcd_put_cur(1, 3);
    lcd_send_string("ERROR!!!");

    estado_anterior = estado_actual;
    estado_actual = ERROR;

    outputs.LED_MOVIMIENTO = 0;
    outputs.LED_EMERGENCIA = 1;
    outputs.MA = 0;
    outputs.MC = 0;

    while (1)
    {
        manejar_interrupcion();

        vTaskDelay(500 / portTICK_PERIOD_MS);
        outputs.LED_EMERGENCIA = 0;
        vTaskDelay(500 / portTICK_PERIOD_MS);
        outputs.LED_EMERGENCIA = 1;

        return ESPERA;
    }
}

estado_t estado_esper
estado_t estado_espera(void)
{
    ESP_LOGI(TAG, "Esperando");
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("ESPERANDO");

    estado_anterior = estado_actual;
    estado_actual = ESPERA;

    outputs.LED_MOVIMIENTO = 0;
    outputs.LED_EMERGENCIA = 0;
    outputs.MA = 0;
    outputs.MC = 0;

    while (1)
    {
        manejar_interrupcion();

        if (!inputs.LSA && !inputs.FC && !inputs.LSC)
        {
            return CERRANDO;
        }
        if (inputs.CA && !inputs.FC)
        {
            return ABRIENDO;
        }
        if (inputs.CC && !inputs.FC)
        {
            return CERRANDO;
        }
        if (inputs.FC && (inputs.LSA || inputs.LSC))
        {
            return ERROR;
        }
        if (inputs.LSA && inputs.LSC)
        {
            return ERROR;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

esp_err_t configurar_timer(void)
{
    ESP_LOGI(TAG, "Configurando el timer...");
    xTimer = xTimerCreate("Timer",
                          pdMS_TO_TICKS(INTERVALO),
                          pdTRUE,
                          (void *)&timerID,
                          callback_timer);

    if (xTimer == NULL)
    {
        ESP_LOGE(TAG, "Error al crear el timer");
        return ESP_FAIL;
    }

    if (xTimerStart(xTimer, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Error al iniciar el timer");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Timer configurado y activo");
    return ESP_OK;
}

void callback_timer(TimerHandle_t pxTimer)
{
    if (estado_actual == CERRANDO || estado_actual == ABRIENDO)
    {
        if (inputs.CA || inputs.CC)
        {
            contador = 0;
        }
        contador++;
    }
    else
    {
        contador = 0;
    }
    manejar_interrupcion();
}

esp_err_t init_i2c(void)
{
    int i2c_master_port = I2C_NUM_0;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

esp_err_t manejar_interrupcion(void)
{
    inputs.LSC = gpio_get_level(GPIO_NUM_13);
    inputs.LSA = gpio_get_level(GPIO_NUM_12);
    inputs.FC = gpio_get_level(GPIO_NUM_14);
    inputs.CC = gpio_get_level(GPIO_NUM_27);
    inputs.CA = gpio_get_level(GPIO_NUM_26);

    gpio_set_level(GPIO_NUM_4, outputs.LED_MOVIMIENTO);
    gpio_set_level(GPIO_NUM_16, outputs.LED_EMERGENCIA);
    gpio_set_level(GPIO_NUM_17, outputs.MC);
    gpio_set_level(GPIO_NUM_5, outputs.MA);

    return ESP_OK;
}
