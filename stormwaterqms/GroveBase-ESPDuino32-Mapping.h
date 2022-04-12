/*
 * Grove Base & ESPDUINO-32 IO Mapping Header
 *  ******************************************************************************
 *
 * @author RTXSC 
 * @version
 * @date
 * @brief  GPIO Pin Mapping ESP32 to Grove Base Shield
 ******************************************************************************
 * @attention
 *
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

 /* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __ATMO_PIN_MAPPING_H_
#define __ATMO_PIN_MAPPING_H_


/* Includes ------------------------------------------------------------------*/

#ifdef __cplusplus
	extern "C"{
#endif

#define ATMO_ESP32_USB_UART 1
#define ATMO_ESP32_NUM_UARTS (2)
#define ESP32_UART1_TX GPIO_NUM_4
#define ESP32_UART1_RX GPIO_NUM_5

#define ESP32_UART2_TX GPIO_NUM_1
#define ESP32_UART2_RX GPIO_NUM_3

#define ESP32_I2C_SDA GPIO_NUM_21
#define ESP32_I2C_SCL GPIO_NUM_22

#define ESP32_SPI_HOST HSPI_HOST
#define ESP32_SPI_MOSI GPIO_NUM_23
#define ESP32_SPI_MISO GPIO_NUM_25
#define ESP32_SPI_CLK GPIO_NUM_19
#define ESP32_SPI_QUADWP -1
#define ESP32_SPI_QUADHD -1

#define IO0 0
#define IO1 1
#define IO2 2
#define IO3 3
#define IO4 4
#define IO5 5
#define IO6 6
#define IO7 7
#define IO8 8
#define IO9 9
#define IO10 10
#define IO11 11
#define IO12 12
#define IO13 13
#define IO14 14
#define IO15 15
#define IO16 16
#define IO17 17
#define IO18 18
#define IO19 19
#define IO20 20
#define IO21 21
#define IO22 22
#define IO23 23
#define IO24 24
#define IO25 25
#define IO26 26
#define IO27 27
#define IO28 28
#define IO29 29
#define IO30 30
#define IO31 31
#define IO32 32
#define IO33 33
#define IO34 34
#define IO35 35
#define IO36 36
#define IO37 37
#define IO38 38
#define IO39 39

#define ESP32_I2C_SDA   IO21
#define ESP32_I2C_SCL   IO22
#define ESP32_SPI_MOSI  IO23
#define ESP32_SPI_MISO  IO25 // INPUT || OUTPUT 
#define ESP32_SPI_CLK   IO19 // INPUT || OUTPUT
#define GROVE_D2        IO26 // INPUT || OUTPUT - DAC2
#define GROVE_D3        IO25 // INPUT || OUTPUT - DAC1
#define GROVE_D4        IO17 // INPUT || OUTPUT
#define GROVE_D5        IO16 // INPUT || OUTPUT
#define GROVE_D6        IO27 // INPUT || OUTPUT
#define GROVE_D7        IO14 // INPUT || OUTPUT - output PWM at boot
#define GROVE_D8        IO12 // INPUT || OUTPUT - boot fail if pulled high  - STRAPPING - BEST TO AVOID
#define GROVE_D9        IO13 // INPUT || OUTPUT 
#define GROVE_D10       IO5  // INPUT || OUTPUT - output PWM at boot        - STRAPPING - BEST TO AVOID
#define GROVE_D11       IO23 // INPUT || OUTPUT 
#define GROVE_D12       IO19 // INPUT || OUTPUT 
#define GROVE_D13       IO18 // INPUT || OUTPUT 
#define GROVE_IO0		IO0  // INPUT || OUTPUT
#define GROVE_IO15      IO15 // INPUT || OUTPUT 


#define GROVE_A0        IO2  // INPUT || OUTPUT - LED_BUILT_IN
#define GROVE_A1        IO4  // INPUT || OUTPUT                             - STRAPPING - BEST TO AVOID
#define GROVE_A2        IO35 // ADC1_7
#define GROVE_A3        IO34 // ADC1_6
#define GROVE_A4        IO36 // ADC1_0
#define GROVE_A5        IO39 // ADC1_3
#define GROVE_A6        IO32 // ADC1_4
#define GROVE_A7        IO33 // ADC1_5
// unused ADC
// IO15, IO32, IO33, IO37, IO38

/* Exported Structures -------------------------------------------------------*/

/* Exported Function Prototypes -----------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif
