/***********************************************************************************************************************
 * File Name    : spi_ep.c
 * Description  : Contains data structures and functions used in spi_ep.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * Copyright [2015-2017] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
 *
 * This file is part of Renesas RA Flex Software Package (FSP)
 *
 * The contents of this file (the "contents") are proprietary and confidential to Renesas Electronics Corporation
 * and/or its licensors ("Renesas") and subject to statutory and contractual protections.
 *
 * This file is subject to a Renesas FSP license agreement. Unless otherwise agreed in an FSP license agreement with
 * Renesas: 1) you may not use, copy, modify, distribute, display, or perform the contents; 2) you may not use any name
 * or mark of Renesas for advertising or publicity purposes or in connection with your use of the contents; 3) RENESAS
 * MAKES NO WARRANTY OR REPRESENTATIONS ABOUT THE SUITABILITY OF THE CONTENTS FOR ANY PURPOSE; THE CONTENTS ARE PROVIDED
 * "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT; AND 4) RENESAS SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, OR
 * CONSEQUENTIAL DAMAGES, INCLUDING DAMAGES RESULTING FROM LOSS OF USE, DATA, OR PROJECTS, WHETHER IN AN ACTION OF
 * CONTRACT OR TORT, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE CONTENTS. Third-party contents
 * included in this file may be subject to different terms.
 **********************************************************************************************************************/

#include "common_utils.h"
#include "spi_ep.h"

/*******************************************************************************************************************//**
 * @addtogroup spi_ep
 * @{
 **********************************************************************************************************************/

/*
 * private function declarations
 */
static void error_print(void);

/* Wait counter for wait operation monitoring */
static volatile uint32_t g_wait_count = MAX_COUNT;

/* Event flags for master and slave */
static volatile spi_event_t g_master_event_flag;    // Master Transfer Event completion flag
static volatile spi_event_t g_slave_event_flag;     // Slave Transfer Event completion flag

/* SPI module buffers for Master and Slave */
static char g_master_tx_buff[BUFF_LEN];   // Master Transmit Buffer
static char g_master_rx_buff[BUFF_LEN];   // Master Receive Buffer
static char g_slave_tx_buff[BUFF_LEN];    // Slave Transmit Buffer
static char g_slave_rx_buff[BUFF_LEN];    // Slave Receive Buffer

/*******************************************************************************************************************//**
 * @brief       This functions initializes SPI master and slave modules.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful open of SPI module
 * @retval      Any Other Error code apart from FSP_SUCCES  Unsuccessful open
 **********************************************************************************************************************/
fsp_err_t spi_init(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status

    /* Open/Initialize SPI Master module */
    err = R_SPI_Open (&g_spi_master_ctrl, &g_spi_master_cfg);
    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* SPI Master Failure message */
        APP_ERR_PRINT("** R_SPI_Open API for SPI Master failed ** \r\n");
        return err;
    }

    /* Open/Initialize SPI Slave module */
    err = R_SPI_Open (&g_spi_slave_ctrl, &g_spi_slave_cfg);
    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* Close SPI master */
        if ( (FSP_SUCCESS != R_SPI_Close(&g_spi_master_ctrl)))
        {
            /* SPI Master Close Failure message */
            APP_ERR_PRINT("** R_SPI_Close API for SPI Master failed ** \r\n");
        }
        /* SPI Slave Failure message */
        APP_ERR_PRINT("** R_SPI_Open API for SPI Master failed ** \r\n");
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief This function demos both R_SPI_Write() and R_SPI_Read() individually.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful SPI Write and SPI Read
 * @retval      Any Other Error code apart from FSP_SUCCES  Unsuccessful Write and Read
 **********************************************************************************************************************/
fsp_err_t spi_write_and_read(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status
    unsigned num_bytes = RESET_VALUE;  // Number of bytes read by SEGGER real-time-terminal

    /* Cleaning buffers */
    memset(&g_master_tx_buff[0], NULL_CHAR, BUFF_LEN);
    memset(&g_master_rx_buff[0], NULL_CHAR, BUFF_LEN);
    memset(&g_slave_rx_buff[0], NULL_CHAR, BUFF_LEN);

    /* Input to master buffer */
    APP_PRINT("\r\nEnter text input for Master buffer. Data size should not exceed 20 bytes. \r\n");
    while (BYTES_RECEIVED_ZERO == num_bytes)
    {
        if (APP_CHECK_DATA)
        {
            num_bytes = APP_READ(g_master_tx_buff);
            if(BYTES_RECEIVED_ZERO == num_bytes)
            {
                APP_PRINT("\r\nNo Input\r\n");
            }
        }
    }

    /* Master send data to Slave */
    err = R_SPI_Write(&g_spi_master_ctrl, g_master_tx_buff, (uint32_t) num_bytes, SPI_BIT_WIDTH_8_BITS);
    /* Error handle */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nMaster R_SPI_Write() failed");
        return err;
    }

    /* Slave receive data from Master */
    err = R_SPI_Read(&g_spi_slave_ctrl, g_slave_rx_buff, (uint32_t) num_bytes, SPI_BIT_WIDTH_8_BITS);
    /* Error handle */
    if(err != FSP_SUCCESS)
    {
        APP_ERR_PRINT("\r\nSlave R_SPI_Read() failed");
        return err;
    }

    /* Wait until master write and slave read complete */
    while((SPI_EVENT_TRANSFER_COMPLETE != g_master_event_flag)||(SPI_EVENT_TRANSFER_COMPLETE != g_slave_event_flag))
    {
        g_wait_count--;
        if (MIN_COUNT >= g_wait_count)
        {
            /* Return time out error if SPI operation fails to complete */
            err = FSP_ERR_TIMEOUT;
            APP_ERR_PRINT("\r\nSPI module blocked in Write/Read operation.\r\n");
            error_print();
            return err;
        }
        else if ((SPI_EVENT_TRANSFER_ABORTED == g_master_event_flag) || (SPI_EVENT_TRANSFER_ABORTED == g_slave_event_flag))
        {
            err = FSP_ERR_TRANSFER_ABORTED;
            APP_ERR_PRINT("\r\nSPI module blocked in Write/Read operation.\r\n");
            error_print();
            return err;
        }
        else
        {
            /* Do nothing */
        }

    }
    g_wait_count = MAX_COUNT;

    g_master_event_flag = RESET_VALUE; // Reseting master_event flag
    g_slave_event_flag = RESET_VALUE;  // Reseting slave_event flag

    /* Display Master to Slave transmission */
    APP_PRINT("\r\nMaster transmitted user input data to Slave \r\n");

    /* Slave send data to Master */
    err = R_SPI_Write(&g_spi_slave_ctrl, g_slave_rx_buff, (uint32_t) num_bytes, SPI_BIT_WIDTH_8_BITS);
    /* Error handle */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nSlave R_SPI_Write() failed");
        return err;
    }

    /* Master receive data from Slave */
    err = R_SPI_Read(&g_spi_master_ctrl, g_master_rx_buff, (uint32_t) num_bytes, SPI_BIT_WIDTH_8_BITS);
    /* Error handle */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nMaster R_SPI_Read() failed");
        return err;
    }

    /* Wait until slave write and master read complete */
    while((SPI_EVENT_TRANSFER_COMPLETE != g_master_event_flag)||(SPI_EVENT_TRANSFER_COMPLETE != g_slave_event_flag))
    {
        /* Time out operation if SPI operation fails to complete */
        g_wait_count--;
        if(MIN_COUNT >= g_wait_count)
        {
            err = FSP_ERR_TIMEOUT;
            APP_ERR_PRINT("\r\nSPI module blocked in Write/Read operation.\r\n");
            error_print();
            return err;
        }
        else if ((SPI_EVENT_TRANSFER_ABORTED == g_master_event_flag) || (SPI_EVENT_TRANSFER_ABORTED == g_slave_event_flag))
        {
            err = FSP_ERR_TRANSFER_ABORTED;
            APP_ERR_PRINT("\r\nSPI module blocked in Write/Read operation.\r\n");
            error_print();
            return err;
        }
        else
        {
            /* Do nothing */
        }
    }
    g_wait_count = MAX_COUNT;

    g_master_event_flag = RESET_VALUE;  // Reseting master_event flag
    g_slave_event_flag = RESET_VALUE;   // Reseting slave_event flag

    /* Display Slave to Master transmission */
    APP_PRINT("\r\nSlave transmitted the data back to Master \r\n");

    /* Check whether transmitted data is equal to received data */
    if(BUFF_EQUAL == memcmp(g_master_tx_buff, g_master_rx_buff, sizeof(g_master_tx_buff)))
    {
        /* Display Master's received data on RTT */
        APP_PRINT("\r\nMaster received data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s \r\n" RTT_CTRL_RESET, g_master_rx_buff);
    }
    else
    {
        /* Return Parity error in case of data mismatch */
        err = FSP_ERR_SPI_PARITY;
        APP_PRINT("\r\nReceived data does not match with transmitted Data.\r\n");
        error_print();
        return err;
    }

    /* Delay of one second for user to verify the output */
    R_BSP_SoftwareDelay(DELAY_ONE_SEC, BSP_DELAY_UNITS_SECONDS);
    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief This function demos R_SPI_WriteRead() for both Master and Slave simultaneously.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  Upon successful Write and Read for both Master and Slav
 * @retval      Any Other Error code apart from FSP_SUCCES  Unsuccessful Write and Read
 **********************************************************************************************************************/
fsp_err_t spi_write_read(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status
    /* Number of bytes read by SEGGER real-time-terminal for master and slave inputs */
    unsigned num_bytes_master = RESET_VALUE;
    unsigned num_bytes_slave = RESET_VALUE;

    /* Cleaning buffers */
    memset(&g_master_tx_buff[0], NULL_CHAR, BUFF_LEN);
    memset(&g_master_rx_buff[0], NULL_CHAR, BUFF_LEN);
    memset(&g_slave_tx_buff[0], NULL_CHAR, BUFF_LEN);
    memset(&g_slave_rx_buff[0], NULL_CHAR, BUFF_LEN);

    /* Input to master buffer */
    APP_PRINT("\r\nEnter text input for Master buffer. Data size should not exceed 20 bytes.\r\n");
    while (BYTES_RECEIVED_ZERO == num_bytes_master)
    {
        if (APP_CHECK_DATA)
        {
            num_bytes_master = APP_READ(g_master_tx_buff);
            if(BYTES_RECEIVED_ZERO == num_bytes_master)
            {
                APP_PRINT("\r\nNo Input\r\n");
            }
        }
    }

    /* Input to slave buffer */
    APP_PRINT("\r\nEnter text input for Slave buffer. Data size should not exceed 20 bytes.\r\n");
    while (BYTES_RECEIVED_ZERO == num_bytes_slave)
    {
        if (APP_CHECK_DATA)
        {
            num_bytes_slave = APP_READ(g_slave_tx_buff);
            if(BYTES_RECEIVED_ZERO == num_bytes_slave)
            {
                APP_PRINT("\r\nNo Input\r\n");
            }
        }
    }

    /* Master send data to Slave and receive data from Slave */
    err = R_SPI_WriteRead(&g_spi_master_ctrl, g_master_tx_buff, g_master_rx_buff, (uint32_t) num_bytes_master, SPI_BIT_WIDTH_8_BITS);
    /* Error handle */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nMaster R_SPI_WriteRead() failed");
        return err;
    }

    /* Slave send data to Master and receive data from Master */
    err = R_SPI_WriteRead(&g_spi_slave_ctrl, g_slave_tx_buff, g_slave_rx_buff, (uint32_t) num_bytes_master, SPI_BIT_WIDTH_8_BITS);
    /* Error handle */
    if (FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nSlave R_SPI_WriteRead() failed");
        return err;
    }

    /* Wait until master and slave WriteRead() complete */
    while((SPI_EVENT_TRANSFER_COMPLETE != g_master_event_flag)||(SPI_EVENT_TRANSFER_COMPLETE != g_slave_event_flag))
    {
        /* Timer out operation if SPI operation fails to complete */
        g_wait_count--;
        if (MIN_COUNT >= g_wait_count)
        {
            err = FSP_ERR_TIMEOUT;
            APP_ERR_PRINT("\r\nSPI module failed in Write/Read operation.\r\n");
            error_print();
            return err;
        }
        else if ((SPI_EVENT_TRANSFER_ABORTED == g_master_event_flag) || (SPI_EVENT_TRANSFER_ABORTED == g_slave_event_flag))
        {
            err = FSP_ERR_TRANSFER_ABORTED;
            APP_ERR_PRINT("\r\nSPI module failed in Write/Read operation.\r\n");
            error_print();
            return err;
        }
        else
        {
            /* Do nothing */
        }
    }
    g_wait_count = MAX_COUNT;

    g_master_event_flag = RESET_VALUE;  // Reseting master_event flag
    g_slave_event_flag = RESET_VALUE;  // Reseting master_event flag

    /* Display Master to Slave and Slave to Master transmission */
    APP_PRINT("\r\nMaster buffer data transmitted to Slave \r\n");
    APP_PRINT("\r\nSlave buffer data transmitted to Master \r\n");

    /* Check whether Slave transmitted data is equal to Master received data */
    if(BUFF_EQUAL == memcmp(g_slave_tx_buff, g_master_rx_buff, sizeof(g_slave_tx_buff)))
    {
        /* Display Master's received data on RTT */
        APP_PRINT("\r\nMaster received data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s\r\n" RTT_CTRL_RESET, g_master_rx_buff);
    }
    else
    {
        /* Return Parity error in case of data mismatch */
        err = FSP_ERR_SPI_PARITY;
        APP_ERR_PRINT("\r\nMaster received data does not match with Slave transmitted Data.\r\n");
        error_print();
        return err;
    }

    /* Check whether Master transmitted data is equal to Slave received data */
    if(BUFF_EQUAL == memcmp(g_master_tx_buff, g_slave_rx_buff, sizeof(g_master_tx_buff)))
    {
        /* Display Slave's received data on RTT */
        APP_PRINT("\r\nSlave received data:" RTT_CTRL_TEXT_BRIGHT_GREEN " %s\r\n" RTT_CTRL_RESET, g_slave_rx_buff);
    }
    else
    {
        /* Return Parity error in case of data mismatch */
        err = FSP_ERR_SPI_PARITY;
        APP_ERR_PRINT("\r\nSlave received data does not match with Master transmitted Data.\r\n");
        error_print();
        return err;
    }

    /* Delay of one second for user to verify the output */
    R_BSP_SoftwareDelay(DELAY_ONE_SEC, BSP_DELAY_UNITS_SECONDS);
    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief This function close both SPI Master and Slave modules.
 * @param[IN]   None
 * @retval      FSP_SUCCESS                  SPI module closed successfully
 * @retval      Any Other Error code apart from FSP_SUCCES  Unsuccessful close
 **********************************************************************************************************************/
fsp_err_t spi_exit_demo(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status

    /* Closing SPI Master module */
    err = R_SPI_Close(&g_spi_master_ctrl);
    /* Error Handle */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nMaster R_SPI_Close() failed");
        return err;
    }

    /* Closing SPI Slave module */
    err = R_SPI_Close(&g_spi_slave_ctrl);
    /* Error Handle */
    if(FSP_SUCCESS != err)
    {
        APP_ERR_PRINT("\r\nSlave R_SPI_Close() failed");
    }
    return err;
}

/*******************************************************************************************************************//**
 * @brief Master SPI callback function.
 * @param[in]  p_args
 * @retval     None
 **********************************************************************************************************************/
void spi_master_callback(spi_callback_args_t * p_args)
{
    if (SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
        g_master_event_flag = SPI_EVENT_TRANSFER_COMPLETE;
    }
    else
    {
        g_master_event_flag = SPI_EVENT_TRANSFER_ABORTED;
    }
}

/*******************************************************************************************************************//**
 * @brief Slave SPI callback function.
 * @param[in]  p_args
 * @retval     None
 **********************************************************************************************************************/
void spi_slave_callback(spi_callback_args_t * p_args)
{
    if (SPI_EVENT_TRANSFER_COMPLETE == p_args->event)
    {
        g_slave_event_flag = SPI_EVENT_TRANSFER_COMPLETE;
    }
    else
    {
        g_slave_event_flag = SPI_EVENT_TRANSFER_ABORTED;
    }
}

/*******************************************************************************************************************//**
 * @brief This function prints the error message.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
static void error_print(void)
{
    APP_PRINT("\r\nError in configuration or connection.\r\n");
    APP_PRINT( "\r\nReset the MCU...\r\n");
}

/*******************************************************************************************************************//**
 * @brief This function closes all the opened SPI modules before the project ends up in an Error Trap.
 * @param[IN]   None
 * @retval      None
 **********************************************************************************************************************/
void spi_clean_up(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close SPI module */
    err = R_SPI_Close(&g_spi_master_ctrl);
    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* SPI Close failure message */
        APP_ERR_PRINT("** R_SPI_Close API for master failed **  \r\n");
    }

    err = R_SPI_Close(&g_spi_slave_ctrl);
    /* handle error */
    if (FSP_SUCCESS != err)
    {
        /* SPI Close failure message */
        APP_ERR_PRINT("** R_SPI_Close API for slave failed **  \r\n");
    }
}

/*******************************************************************************************************************//**
 * @} (end addtogroup spi_ep)
 **********************************************************************************************************************/
