#include "bsp_spi.h"

#define SPI_PORT         GPIOA
#define SPI_SCK_PIN      5U
#define SPI_MISO_PIN     6U
#define SPI_MOSI_PIN     7U
#define SPI_CS_PIN       8U

void BspSpi_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* CS：普通推挽输出，初始拉高（不选中） */
    SPI_PORT->MODER   &= ~(3U << (SPI_CS_PIN * 2));
    SPI_PORT->MODER   |=  (1U << (SPI_CS_PIN * 2));       /* 01 = 输出 */
    SPI_PORT->OTYPER  &= ~(1U << SPI_CS_PIN);
    SPI_PORT->OSPEEDR |=  (1U << (SPI_CS_PIN * 2));       /* 中速 */
    SPI_PORT->BSRR     =  (1U << SPI_CS_PIN);             /* CS 高 = 释放 */

    /* SCK / MISO / MOSI：复用（三个都要 10） */
    SPI_PORT->MODER   &= ~(3U << (SPI_SCK_PIN  * 2));
    SPI_PORT->MODER   |=  (2U << (SPI_SCK_PIN  * 2));
    SPI_PORT->MODER   &= ~(3U << (SPI_MISO_PIN * 2));
    SPI_PORT->MODER   |=  (2U << (SPI_MISO_PIN * 2));
    SPI_PORT->MODER   &= ~(3U << (SPI_MOSI_PIN * 2));
    SPI_PORT->MODER   |=  (2U << (SPI_MOSI_PIN * 2));

    /* ★ AF5 —— PA5 shift=20, PA6 shift=24, PA7 shift=28 */
    SPI_PORT->AFR[0] &= ~((0xFU << 20) | (0xFU << 24) | (0xFU << 28));
    SPI_PORT->AFR[0] |=  ((5U   << 20) | (5U   << 24) | (5U   << 28));

    /* SCK / MOSI 高速 */
    SPI_PORT->OSPEEDR &= ~(3U << (SPI_SCK_PIN * 2));
    SPI_PORT->OSPEEDR |=  (3U << (SPI_SCK_PIN * 2));
    SPI_PORT->OSPEEDR &= ~(3U << (SPI_MOSI_PIN * 2));
    SPI_PORT->OSPEEDR |=  (3U << (SPI_MOSI_PIN * 2));

    /* 主机 + 软件 NSS + 分频 /256（328kHz）+ 模式0 + 8bit + MSB */
    SPI1->CR1 = SPI_CR1_MSTR
              | SPI_CR1_SSM
              | SPI_CR1_SSI
              | (7U << 3)
              | SPI_CR1_SPE;
    /* CPOL/CPHA/DFF/LSBFIRST 复位默认就是 0，不用手动清 */
}

void BspSpi_Start(void)   /* 选中从机：CS 低 */
{
    SPI_PORT->BSRR = (1U << (SPI_CS_PIN + 16U));
}

void BspSpi_Stop(void)    /* 释放从机：CS 高 */
{
    SPI_PORT->BSRR = (1U << SPI_CS_PIN);
}

uint8_t BspSpi_SwapByte(uint8_t byte)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    SPI1->DR = byte;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return  SPI1->DR;
}
