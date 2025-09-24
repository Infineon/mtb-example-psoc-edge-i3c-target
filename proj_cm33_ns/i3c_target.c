/*****************************************************************************
 * File Name        : i3c_target.c
 *
 * Description      : This file contains the functions and variables required 
 *                    for operation of I3C target
 *
 * Related Document : See README.md
 *
 ******************************************************************************
 * Copyright 2025, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 ******************************************************************************/

#include "i3c_target.h"
#include <stdio.h>

/******************************************************************************
 *            Constants
 *****************************************************************************/
/* Buffer and packet size */
#define PACKET_SIZE          (3UL)

/* Buffer and packet size in the target*/
#define TR_TX_BUFFER_SIZE    (7UL)
#define TR_RX_BUFFER_SIZE    (7UL)

/* Start and end of packet markers */
#define PACKET_SOP           (0x01UL)
#define PACKET_EOP           (0x9UL)

/* Command valid status */
#define STS_CMD_DONE         (0x00UL)
#define STS_CMD_FAIL         (0xFFUL)

/* Packet positions */
#define PACKET_SOP_POS       (0UL)
#define PACKET_STS_POS       (1UL)
#define PACKET_CMD_POS       (1UL)
#define PACKET_EOP_POS       (6UL)

#define ZERO                 (0)
#define CHECK_STATUS         (0)

#define RX_BUFFER_START_INDEX (0)
#define RX_BUFFER_LENGTH     (7U)
#define CHECKSUM_VAL         (39U)

/*mandatory data byte*/
#define MDB                  (0x1FU) 
#define IBI_PAYLOAD_SIZE     (0x4U)
#define DATA_1               (4U)
#define DATA_2               (5U)
#define DATA_3               (6U)
#define DATA_4               (7U)
#define DATA_5               (8U)


/*******************************************************************************
 * Global variables
 *******************************************************************************/
/* I3C target read and write buffers. */
//uint8_t i3cTxBuffer[TR_TX_BUFFER_SIZE] = {5,7,8,9};
uint8_t i3cRxBuffer[TR_RX_BUFFER_SIZE] ;

cy_stc_i3c_context_t CYBSP_I3C_TARGET_context;
uint8_t buffertx[TR_TX_BUFFER_SIZE] = {PACKET_SOP,DATA_1, DATA_2, DATA_3, DATA_4,DATA_5, PACKET_EOP};

/*******************************************************************************
* Function Name: CYBSP_I3C_TARGET_Interrupt
*******************************************************************************
* Summary:
* Invokes the Cy_I3C_Interrupt() PDL driver function.
*
* Parameters:
*  none
*
* Return:
*  void
*
 *******************************************************************************/
void CYBSP_I3C_TARGET_Interrupt(void)
{
    Cy_I3C_Interrupt(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_context);
}

/*******************************************************************************
* Function Name: HandleEvents
********************************************************************************
* Summary:
* Callback function when an event occurs
*
* Parameters:
*  event : Retuns to the callback based on the event
*
* Return:
*  void
*
*******************************************************************************/
void HandleEvents(uint32_t event)
{
    uint32_t len;
    uint8_t addr;
    uint32_t status;
    uint32_t count;
    uint8_t idx;
    /*Target Reset Pattern */
    if( CHECK_STATUS != (CY_I3C_TARGET_RESET_EVENT & event))
    {
        if(CY_I3C_NO_RESET == Cy_I3C_Target_GetResetMode(&CYBSP_I3C_TARGET_context)){
            printf("Target Reset mode 0.\n\r");

        }else if(CY_I3C_PERIPHERAL_RESET == Cy_I3C_Target_GetResetMode(&CYBSP_I3C_TARGET_context)){
            Cy_I3C_Reset(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_config, &CYBSP_I3C_TARGET_context);
            printf("Target Reset mode 1 done.\n\r");
        }else{
            printf("Target Reset mode 2.\n\r");
        }
    }
    /*Read length updated */
    if( CHECK_STATUS != (CY_I3C_TARGET_MAX_RD_LEN_UPDT_EVENT & event))
    {
        len = Cy_I3C_TargetGetMaxReadLength(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_context);
        printf("Updated Max Read Length = %u \n\r",(unsigned int)len);
        Cy_I3C_Resume(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_context);
    }
    /*Write length updated */
    else if(CHECK_STATUS != (CY_I3C_TARGET_MAX_WR_LEN_UPDT_EVENT & event))
    {
        len = Cy_I3C_TargetGetMaxWriteLength(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_context);
        printf("Updated Max Write Length = %u \n\r",(unsigned int)len);
        Cy_I3C_Resume(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_context);
    }
    else if(CHECK_STATUS != (CY_I3C_TARGET_CCC_REG_UPDATED_EVENT & event))
    {
        printf("CCC Register updated\n\r");
    }
    /* Dynamic Address Assigned */
    else if(CHECK_STATUS != (CY_I3C_TARGET_ASSIGNED_DYN_ADDR_EVENT & event))
    {
        printf("Dynamic Address Assigned\n\r");
        status = Cy_I3C_TargetGetDynamicAddress(CYBSP_I3C_TARGET_HW, &addr, &CYBSP_I3C_TARGET_context);
        if(I3C_SUCCESS == status)
        {
            printf("Assigned dynamic address = %d\n\r", addr);
        }
        else
        {
            printf("Invalid address\n\r");
        }
    }
    else if(CHECK_STATUS != (CY_I3C_TARGET_RD_CMPLT_EVENT & event))
    {
        int sum = 0;
        count = Cy_I3C_TargetGetWriteTransferCount(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_context);

        for (int i = RX_BUFFER_START_INDEX; i < RX_BUFFER_LENGTH; i++) {
            sum += i3cRxBuffer[i];
        }
        if (CHECKSUM_VAL == sum)
        {
            printf("Number of bytes received from controller = %u\n\r",(unsigned int)count);
            printf(" SOP\t");
            printf("\t Data packets received      \t");
            printf("EOP\n");
            printf(" ----\t");
            printf("----------------------------------\t");
            printf("----\t");
            printf("\n");
            printf (" %-2d\t", i3cRxBuffer[PACKET_SOP_POS]);
            for(idx = BYTE_UPDATE; idx <= MAX_LEN; idx++)
            {
                printf("%-2d\t", i3cRxBuffer[idx]);
            }
            printf ("%-2d\t", i3cRxBuffer[PACKET_EOP_POS]);
            printf("\n");

            Cy_I3C_TargetConfigWriteBuf(CYBSP_I3C_TARGET_HW, i3cRxBuffer, TR_RX_BUFFER_SIZE, &CYBSP_I3C_TARGET_context);
        }
        else {
            printf("Error in packets received");
        }
    }
    else if(CHECK_STATUS != (CY_I3C_TARGET_WR_CMPLT_EVENT & event))
    {
        printf("Controller Read complete\n\r ");
        count = Cy_I3C_TargetGetReadTransferCount(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_context);
        printf("Number of bytes written from the target = %u\n\r",(unsigned int) count);
        printf(" SOP\t");
        printf("\t Data packets sent      \t");
        printf("EOP\n");
        printf(" ----\t");
        printf("----------------------------------\t");
        printf("----\t");
        printf("\n");
        printf (" %-2d\t", buffertx[PACKET_SOP_POS]);
        for(idx = BYTE_UPDATE; idx <= MAX_LEN; idx++)
        {
            printf("%-2d\t", buffertx[idx]);
        }
        printf ("%-2d\t", buffertx[PACKET_EOP_POS]);
        printf("\n");
        Cy_I3C_TargetConfigReadBuf(CYBSP_I3C_TARGET_HW, buffertx, 7, &CYBSP_I3C_TARGET_context);
    }
    else if(CHECK_STATUS != (CY_I3C_TARGET_WR_BUFFER_NOT_CONFIGURED & event)){

        printf("no write buffer configured\n\r ");
    }
    else if(CHECK_STATUS != ((CY_I3C_TARGET_NO_VALID_CMD_IN_CMDQ_EVENT | CY_I3C_TARGET_DATA_NOT_READY_EVENT) & event))
    {
        if(CY_I3C_TARGET_NO_VALID_CMD_IN_CMDQ_EVENT & event)
            if(CY_I3C_TARGET_DATA_NOT_READY_EVENT & event)
                printf("Data is not ready in the Tx FIFO or Response queue is full\n\r");
    }
    else if(CHECK_STATUS != (CY_I3C_TARGET_IBI_NOT_ATTEMPTED_EVENT  & event)){
        printf("IBI NOT attempted reseon might be:\n\r \
            1. Controller has not assigned the dynamic address\n\r \
            2. Controller has cleared the assigned dynamic address\n\r \
            3. Controller has disabled the IBI (DISEC)\n\r \
            4. The taget has switched to controller mode\n\r");
    }
    else if(CHECK_STATUS != (CY_I3C_TARGET_IBI_ACKNOWLEDGED_EVENT  & event)){
        printf("IBI Acked. \n\r");
    }
    else if(CHECK_STATUS != (CY_I3C_TARGET_IBI_EARLY_STOP_EVENT  & event)){
        printf("IBI Terminated Early by controller, No of bytes remained to be transfered: ");
        printf("%u\n\r", Cy_I3C_Target_getTIRRemainingDataLength(CYBSP_I3C_TARGET_HW));
    }
}

/*******************************************************************************
* Function Name: iniCYBSP_I3C_TARGETTarget
****************************************************************************//**
* Summary :
* This function preforms I3C target initialization
* 
* Parameters:
* None
*
* Return:
* uint32_t
*
*******************************************************************************/
uint32_t initI3CTarget(void)
{
    cy_en_i3c_status_t initStatus;
    cy_en_sysint_status_t sysStatus;
    cy_stc_sysint_t CYBSP_I3C_TARGET_IRQ_cfg =
    {
            .intrSrc = CYBSP_I3C_TARGET_IRQ,
            .intrPriority = 2UL
    };

    initStatus = Cy_I3C_Init(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_config, &CYBSP_I3C_TARGET_context);
    if(CY_I3C_SUCCESS != initStatus)
    {
        printf("I3C Init failed");

        return I3C_FAILURE;
    }

    /* Hook interrupt service routine. */
    sysStatus = Cy_SysInt_Init(&CYBSP_I3C_TARGET_IRQ_cfg, &CYBSP_I3C_TARGET_Interrupt);
    if(CY_SYSINT_SUCCESS != sysStatus)
    {
        return I3C_FAILURE;
    }

    /* Enable interrupt in NVIC. */
    NVIC_EnableIRQ((IRQn_Type) CYBSP_I3C_TARGET_IRQ_cfg.intrSrc);
    
    /* buffer from which controller reads data */
    Cy_I3C_TargetConfigReadBuf(CYBSP_I3C_TARGET_HW,  buffertx, TR_TX_BUFFER_SIZE, &CYBSP_I3C_TARGET_context);
    
    /* buffer to which controller writes data */
    Cy_I3C_TargetConfigWriteBuf(CYBSP_I3C_TARGET_HW, i3cRxBuffer, TR_RX_BUFFER_SIZE, &CYBSP_I3C_TARGET_context);
    Cy_I3C_RegisterEventCallback(CYBSP_I3C_TARGET_HW, (cy_cb_i3c_handle_events_t) HandleEvents, &CYBSP_I3C_TARGET_context);
    Cy_I3C_Enable(CYBSP_I3C_TARGET_HW, &CYBSP_I3C_TARGET_context);

    return I3C_SUCCESS;
}

/* [] END OF FILE */
