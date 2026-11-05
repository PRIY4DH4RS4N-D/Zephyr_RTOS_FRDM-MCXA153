
#include "fsl_common.h"
#include "fsl_gpio.h"
#include "pin_mux.h"
#include "clock_config.h"

/* FRDM-MCXA153 RGB LED pins */
#define RED_LED_PIN      12U
#define GREEN_LED_PIN    13U
#define BLUE_LED_PIN      0U

#define LED_PORT GPIO3

/* RGB onboard LEDs are active LOW */
#define LED_ON   0U
#define LED_OFF  1U

static void RGB_SetColor(uint8_t red, uint8_t green, uint8_t blue)
{
    GPIO_PinWrite(LED_PORT, RED_LED_PIN, red ? LED_ON : LED_OFF);
    GPIO_PinWrite(LED_PORT, GREEN_LED_PIN, green ? LED_ON : LED_OFF);
    GPIO_PinWrite(LED_PORT, BLUE_LED_PIN, blue ? LED_ON : LED_OFF);
}

int main(void)
{
    /* Initialize MCU configuration */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();

    /* Initialize GPIO port */
    GPIO_PortInit(LED_PORT);

    gpio_pin_config_t led_config =
    {
        kGPIO_DigitalOutput,
        LED_OFF
    };

    /* Configure RGB pins as outputs */
    GPIO_PinInit(LED_PORT, RED_LED_PIN, &led_config);
    GPIO_PinInit(LED_PORT, GREEN_LED_PIN, &led_config);
    GPIO_PinInit(LED_PORT, BLUE_LED_PIN, &led_config);

    /* Initially switch OFF all LEDs */
    RGB_SetColor(0, 0, 0);

    while (1)
    {
        /* RED */
        RGB_SetColor(1, 0, 0);
        SDK_DelayAtLeastUs(700000U, SystemCoreClock);

        /* GREEN */
        RGB_SetColor(0, 1, 0);
        SDK_DelayAtLeastUs(700000U, SystemCoreClock);

        /* BLUE */
        RGB_SetColor(0, 0, 1);
        SDK_DelayAtLeastUs(700000U, SystemCoreClock);

        /* YELLOW = RED + GREEN */
        RGB_SetColor(1, 1, 0);
        SDK_DelayAtLeastUs(700000U, SystemCoreClock);

        /* CYAN = GREEN + BLUE */
        RGB_SetColor(0, 1, 1);
        SDK_DelayAtLeastUs(700000U, SystemCoreClock);

        /* MAGENTA = RED + BLUE */
        RGB_SetColor(1, 0, 1);
        SDK_DelayAtLeastUs(700000U, SystemCoreClock);

        /* WHITE = RED + GREEN + BLUE */
        RGB_SetColor(1, 1, 1);
        SDK_DelayAtLeastUs(1000000U, SystemCoreClock);

        /* OFF */
        RGB_SetColor(0, 0, 0);
        SDK_DelayAtLeastUs(700000U, SystemCoreClock);
    }
}
