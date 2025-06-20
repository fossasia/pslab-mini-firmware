/**
 * @file uart.c
 * @brief UART (Universal Asynchronous Receiver/Transmitter) module implementation.
 *
 * This module provides functions and routines for configuring and handling UART communication.
 * It includes initialization, data transmission, and reception functionalities for serial communication.
 *
 * @author Alexander Bessman
 * @date 2025-06-13
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stm32h5xx_hal.h"

#include "uart.h"


#define UART_TIMEOUT 100000 /** 100 ms */

static UART_HandleTypeDef huart = { 0 };

DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;

static volatile uint32_t rx_counter = 0; /**< Counter for received bytes */
static volatile uint32_t tx_counter = 0; /**< Counter for transmitted bytes */

/**
 * @brief Callback function for UART receive complete interrupt.
 *
 * This function is called when a UART receive operation is completed.
 * It increments the receive counter to keep track of the number of received bytes.
 *
 * @param huart Pointer to the UART handle structure.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	rx_counter++;

}

/**
 * @brief Callback function for UART transmit complete interrupt.
 *
 * This function is called when a UART transmit operation is completed.
 * It increments the transmit counter to keep track of the number of transmitted bytes.
 *
 * @param huart Pointer to the UART handle structure.
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    tx_counter++;
}

/**
 * @brief Initialize the UART peripheral.
 *
 * This function configures the UART hardware, including baud rate, data bits,
 * stop bits, and parity, to prepare it for serial communication.
 *
 * @return 0 on success, non-zero value on failure.
 */
HAL_StatusTypeDef UART_init(void)
{
    huart.Instance = USART3;
    huart.Init.BaudRate = 57600;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    huart.Init.Mode = UART_MODE_TX_RX;
    huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;  
    huart.Init.OverSampling = UART_OVERSAMPLING_16; 

    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    return HAL_UART_Init(&huart);
}

/**
 * @brief Initialize the DMA for UART receive operation.
 *
 * This function configures the DMA channels for UART receive operation.
 * It sets up the DMA to transfer data from the UART peripheral to memory,
 * enabling efficient data transfer without CPU intervention.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef hdma_usart3_rx_init(void)
{

    __HAL_RCC_GPDMA1_CLK_ENABLE(); // Enable GPDMA1 clock

    HAL_DMA_DeInit(&hdma_usart3_tx);   // Deinitialize DMA to ensure clean state

    // Initialize DMA for UART RX
    hdma_usart3_rx.Instance = GPDMA1_Channel0;                              // Use GPDMA1 Channel 0 for USART3 RX                  
    hdma_usart3_rx.Init.Request = GPDMA1_REQUEST_USART3_RX;                 // Set the request for USART3 RX
    hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;                   // Set direction from peripheral to memory
    hdma_usart3_rx.Init.SrcInc = DMA_SINC_FIXED;                            // Disable source increment
    hdma_usart3_rx.Init.DestInc = DMA_DINC_INCREMENTED;                     // Enable destination increment
    hdma_usart3_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;              // Set source data width to byte
    hdma_usart3_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;            // Set destination data width to byte
    hdma_usart3_rx.Init.Mode = DMA_NORMAL;                                  // Set DMA mode to normal
    hdma_usart3_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;               // Set block hardware request to single burst
    hdma_usart3_rx.Init.SrcBurstLength = 1;                                 // Set source burst length to 1
    hdma_usart3_rx.Init.DestBurstLength = 1;                                // Set destination burst length to 1 
    hdma_usart3_rx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT1 | DMA_DEST_ALLOCATED_PORT0; // Set Receive allocated ports  
    hdma_usart3_rx.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;            // Set DMA priority to low with high weight 
    
    
    huart.hdmarx = &hdma_usart3_rx;
    hdma_usart3_rx.Parent = &huart;

    __HAL_LINKDMA(&huart, hdmarx, hdma_usart3_rx);

    __HAL_DMA_ENABLE_IT(&hdma_usart3_rx, DMA_IT_TC);  // Enable transfer complete interrupt

    // Enable DMA interrupts
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    return HAL_DMA_Init(&hdma_usart3_rx);
}

/**
 * @brief Initialize the DMA for UART transmit operation.
 *
 * This function configures the DMA channels for UART transmit operation.
 * It sets up the DMA to transfer data from memory to the UART peripheral,
 * enabling efficient data transfer without CPU intervention.
 *
 * @return HAL status (HAL_OK on success).
 */

HAL_StatusTypeDef hdma_usart3_tx_init(void)
{   
    __HAL_RCC_GPDMA1_CLK_ENABLE();  // Enable GPDMA1 clock

	HAL_DMA_DeInit(&hdma_usart3_tx);    // Deinitialize DMA to ensure clean state

    // Initialize DMA for UART TX    
    hdma_usart3_tx.Instance = GPDMA1_Channel1;  // Use GPDMA1 Channel 1 for USART3 TX
    hdma_usart3_tx.Init.Request = GPDMA1_REQUEST_USART3_TX; // Set the request for USART3 TX
    hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;   // Set direction from memory to peripheral
    hdma_usart3_tx.Init.SrcInc = DMA_SINC_INCREMENTED;      // Enable source increment
    hdma_usart3_tx.Init.DestInc = DMA_DINC_FIXED;           // Disable destination increment
    hdma_usart3_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;      // Set source data width to byte
    hdma_usart3_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;    // Set destination data width to byte
    hdma_usart3_tx.Init.Mode = DMA_NORMAL;                          // Set DMA mode to normal
    hdma_usart3_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;       // Set block hardware request to single burst
	hdma_usart3_tx.Init.SrcBurstLength = 1 ;                        // Set source burst length to 1
	hdma_usart3_tx.Init.DestBurstLength = 1 ;                       // Set destination burst length to 1  
    hdma_usart3_tx.Init.Priority = DMA_LOW_PRIORITY_HIGH_WEIGHT;    // Set DMA priority to low with high weight
    hdma_usart3_tx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;     // Set transfer allocated ports

    huart.hdmatx = &hdma_usart3_tx;
	hdma_usart3_tx.Parent = &huart;

    __HAL_LINKDMA(&huart, hdmatx, hdma_usart3_tx);

    __HAL_DMA_ENABLE_IT(&hdma_usart3_tx,DMA_IT_TC);

    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

    return HAL_DMA_Init(&hdma_usart3_tx);

}


/**
 * @brief Write data to the UART interface.
 *
 * This function transmits a specified number of bytes from the provided buffer
 * over the UART peripheral.
 *
 * @param txbuf      Pointer to the data buffer to be transmitted.
 * @param sz         Number of bytes to write from the buffer.
 *
 * @return 0 on success, non-zero value on failure.
 */
HAL_StatusTypeDef UART_write(uint8_t const *txbuf, uint32_t const sz)
{
    return HAL_UART_Transmit(&huart, txbuf, sz, UART_TIMEOUT);
}


/**
 * @brief Read data from the UART interface.
 *
 * This function receives a specified number of bytes into the provided buffer
 * over the UART peripheral.
 *
 * @param rxbuf      Pointer to allocated memory into which to read data.
 * @param sz         Number of bytes to read into the buffer.
 *
 * @return 0 on success, non-zero value on failure.
 */
HAL_StatusTypeDef UART_read(uint8_t *const rxbuf, uint32_t const sz)
{
    return HAL_UART_Receive(&huart, rxbuf, sz, UART_TIMEOUT);
}


/**
 * @brief Read data from the UART interface using DMA.
 *
 * This function initiates a DMA transfer to receive a specified number of bytes
 * into the provided buffer over the UART peripheral.
 *
 * @param rxbuf      Pointer to allocated memory into which to read data.
 * @param sz         Number of bytes to read into the buffer.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_read_DMA(uint8_t *const rxbuf, uint32_t const sz)
{
    return HAL_UART_Receive_DMA(&huart, rxbuf, sz);
}


/**
 * @brief Write data to the UART interface using DMA.
 *
 * This function initiates a DMA transfer to transmit a specified number of bytes
 * from the provided buffer over the UART peripheral.
 *
 * @param txbuf      Pointer to the data buffer to be transmitted.
 * @param sz         Number of bytes to write from the buffer.
 *
 * @return HAL status (HAL_OK on success).
 */
HAL_StatusTypeDef UART_write_DMA(uint8_t const *txbuf, uint32_t const sz)
{
    return HAL_UART_Transmit_DMA(&huart, txbuf, sz);
}

/**
 * @brief Check if data is available to read from the UART receive buffer.
 *
 * This function returns true if the UART receive data register is not empty,
 * indicating that there is unread data available to be received.
 *
 * @return true if at least one byte is available to be read, false otherwise.
 */
bool UART_rx_ready(void)
{
    return huart.RxState == HAL_UART_STATE_READY;
}

bool UART_tx_ready(void)
{
    return huart.gState == HAL_UART_STATE_READY;
}

/**
 * @brief  Initialize the UART MSP (MCU Support Package).
 *
 * This function configures the hardware resources used for the UART peripheral,
 * including GPIO pins and clocks. It is called by HAL_UART_Init().
 *
 * @param huart UART handle pointer.
 *
 * @return None
 */
void HAL_UART_MspInit(__attribute__((unused))UART_HandleTypeDef *huart)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();
    GPIO_InitTypeDef uart_gpio_init = {
        .Pin = GPIO_PIN_8 | GPIO_PIN_9,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_MEDIUM,
        .Alternate = GPIO_AF7_USART3,
    };
    HAL_GPIO_Init(GPIOD, &uart_gpio_init);
}


/**
 * @brief  Initialize the IRQ handlers for the UART peripheral.
 *
 * @param None
 * 
 * @return None
 */
void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart);
}

/**
 * @brief  Initialize the IRQ handlers for the UART RX DMA peripheral.
 *
 * @param None
 * 
 * @return None
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(huart.hdmarx);
}

/**
 * @brief  Initialize the IRQ handlers for the UART TX DMA peripheral.
 *
 * @param None
 * 
 * @return None
 */
void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(huart.hdmatx);
}