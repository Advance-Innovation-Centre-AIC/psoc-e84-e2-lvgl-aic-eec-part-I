/*******************************************************************************
 * File: cm55_ipc_communication.c
 * Description: CM55 IPC Pipe infrastructure setup
 *
 * Configures pipe endpoints and registers ISR for CM55 <-> CM33 communication.
 *
 * Based on: psoc-e84-lvgl-ipc-pipe reference project
 ******************************************************************************/

#include "ipc_communication.h"

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

/* Endpoint structure array */
static cy_stc_ipc_pipe_ep_t cm55_ipc_pipe_ep_array[CY_IPC_MAX_ENDPOINTS];

/* Callback array for EP2 (CM55 receiver) */
static cy_ipc_pipe_callback_ptr_t ep2_cb_array[CY_IPC_CYPIPE_CLIENT_CNT];

/*******************************************************************************
 * Function: cm55_ipc_pipe_isr
 * ISR for the IPC pipe on CM55 side
 ******************************************************************************/
void cm55_ipc_pipe_isr(void)
{
    Cy_IPC_Pipe_ExecuteCallback(CM55_IPC_PIPE_EP_ADDR);
}

/*******************************************************************************
 * Function: cm55_ipc_communication_setup
 * Initialize IPC pipe infrastructure for CM55
 *
 * Must be called BEFORE Cy_IPC_Pipe_RegisterCallback()
 * CM33 should have already called cm33_ipc_communication_setup() before
 * enabling CM55.
 ******************************************************************************/
void cm55_ipc_communication_setup(void)
{
    /* IPC pipe endpoint-2 (CM55 receiver) and endpoint-1 (CM33 sender) config */
    static const cy_stc_ipc_pipe_config_t cm55_ipc_pipe_config =
    {
        /* Receiver endpoint: CM55 (EP2) */
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
        /* Sender endpoint: CM33 (EP1) */
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
        .endpointClientsCount    = CY_IPC_CYPIPE_CLIENT_CNT,
        .endpointsCallbacksArray = ep2_cb_array,
        .userPipeIsrHandler      = &cm55_ipc_pipe_isr
    };

    /* Phase 1: Configure endpoint array */
    Cy_IPC_Pipe_Config(cm55_ipc_pipe_ep_array);

    /* Phase 2: Initialize pipe with full config */
    Cy_IPC_Pipe_Init(&cm55_ipc_pipe_config);
}
