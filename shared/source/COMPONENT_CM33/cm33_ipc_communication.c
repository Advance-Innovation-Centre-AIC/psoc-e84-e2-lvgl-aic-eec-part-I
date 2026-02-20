/*******************************************************************************
 * File: cm33_ipc_communication.c
 * Description: CM33 IPC Pipe infrastructure setup
 *
 * Initializes IPC semaphores, configures pipe endpoints, and registers ISR
 * for CM33 <-> CM55 communication.
 *
 * Based on: psoc-e84-lvgl-ipc-pipe reference project
 ******************************************************************************/

#include "ipc_communication.h"

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

/* Endpoint structure array */
static cy_stc_ipc_pipe_ep_t cm33_ipc_pipe_ep_array[CY_IPC_MAX_ENDPOINTS];

/* Callback array for EP1 (CM33 receiver) */
static cy_ipc_pipe_callback_ptr_t ep1_cb_array[CY_IPC_CYPIPE_CLIENT_CNT];

/* Semaphore array in shared memory */
CY_SECTION_SHAREDMEM
static uint32_t ipc_sema_array[CY_IPC_SEMA_COUNT / CY_IPC_SEMA_PER_WORD];

/*******************************************************************************
 * Function: cm33_ipc_pipe_isr
 * ISR for the IPC pipe on CM33 side
 ******************************************************************************/
void cm33_ipc_pipe_isr(void)
{
    Cy_IPC_Pipe_ExecuteCallback(CM33_IPC_PIPE_EP_ADDR);
}

/*******************************************************************************
 * Function: cm33_ipc_communication_setup
 * Initialize IPC pipe infrastructure for CM33
 *
 * Must be called BEFORE Cy_IPC_Pipe_RegisterCallback() and BEFORE enabling CM55
 ******************************************************************************/
void cm33_ipc_communication_setup(void)
{
    /* IPC pipe endpoint-1 (CM33) and endpoint-2 (CM55) config */
    static const cy_stc_ipc_pipe_config_t cm33_ipc_pipe_config =
    {
        /* Receiver endpoint: CM33 (EP1) */
        {
            .ipcNotifierNumber   = CY_IPC_INTR_CYPIPE_EP1,
            .ipcNotifierPriority = CY_IPC_INTR_CYPIPE_PRIOR_EP1,
            .ipcNotifierMuxNumber = CY_IPC_INTR_CYPIPE_MUX_EP1,
            .epAddress           = CM33_IPC_PIPE_EP_ADDR,
            {
                .epChannel       = CY_IPC_CHAN_CYPIPE_EP1,
                .epIntr          = CY_IPC_INTR_CYPIPE_EP1,
                .epIntrmask      = CY_IPC_CYPIPE_INTR_MASK
            }
        },
        /* Sender endpoint: CM55 (EP2) */
        {
            .ipcNotifierNumber   = CY_IPC_INTR_CYPIPE_EP2,
            .ipcNotifierPriority = CY_IPC_INTR_CYPIPE_PRIOR_EP2,
            .ipcNotifierMuxNumber = CY_IPC_INTR_CYPIPE_MUX_EP2,
            .epAddress           = CM55_IPC_PIPE_EP_ADDR,
            {
                .epChannel       = CY_IPC_CHAN_CYPIPE_EP2,
                .epIntr          = CY_IPC_INTR_CYPIPE_EP2,
                .epIntrmask      = CY_IPC_CYPIPE_INTR_MASK
            }
        },
        .endpointClientsCount    = CY_IPC_CYPIPE_CLIENT_CNT,
        .endpointsCallbacksArray = ep1_cb_array,
        .userPipeIsrHandler      = &cm33_ipc_pipe_isr
    };

    /* Phase 1: Initialize IPC semaphores */
    Cy_IPC_Sema_Init(IPC0_SEMA_CH_NUM, CY_IPC_SEMA_COUNT, ipc_sema_array);

    /* Phase 2: Configure endpoint array */
    Cy_IPC_Pipe_Config(cm33_ipc_pipe_ep_array);

    /* Phase 3: Initialize pipe with full config */
    Cy_IPC_Pipe_Init(&cm33_ipc_pipe_config);
}
