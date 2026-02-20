/*******************************************************************************
 * File: ipc_communication.h
 * Description: IPC Pipe infrastructure configuration for CM33 <-> CM55
 *
 * Defines endpoint addresses, channels, interrupts, and function prototypes
 * for setting up the Infineon IPC Pipe communication layer.
 *
 * Based on: psoc-e84-lvgl-ipc-pipe reference project
 ******************************************************************************/

#ifndef IPC_COMMUNICATION_H
#define IPC_COMMUNICATION_H

#include "cy_ipc_pipe.h"
#include "cy_pdl.h"
#include "cybsp.h"
#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * IPC Pipe Configuration
 ******************************************************************************/

#define CY_IPC_MAX_ENDPOINTS            (5UL)
#define CY_IPC_CYPIPE_CLIENT_CNT        (8UL)

/* Endpoint 1 (CM33) - Channel and Interrupt */
#define CY_IPC_CHAN_CYPIPE_EP1          (4UL)
#define CY_IPC_INTR_CYPIPE_EP1         (4UL)

/* Endpoint 2 (CM55) - Channel and Interrupt */
#define CY_IPC_CHAN_CYPIPE_EP2          (15UL)
#define CY_IPC_INTR_CYPIPE_EP2         (5UL)

/* EP1 (CM33) derived masks and config */
#define CY_IPC_CYPIPE_CHAN_MASK_EP1     CY_IPC_CH_MASK(CY_IPC_CHAN_CYPIPE_EP1)
#define CY_IPC_CYPIPE_INTR_MASK_EP1    CY_IPC_INTR_MASK(CY_IPC_INTR_CYPIPE_EP1)
#define CY_IPC_INTR_CYPIPE_PRIOR_EP1   (1UL)
#define CY_IPC_INTR_CYPIPE_MUX_EP1     (CY_IPC0_INTR_MUX(CY_IPC_INTR_CYPIPE_EP1))
#define CM33_IPC_PIPE_EP_ADDR          (1UL)
#define CM33_IPC_PIPE_CLIENT_ID        (3UL)

/* EP2 (CM55) derived masks and config */
#define CY_IPC_CYPIPE_CHAN_MASK_EP2     CY_IPC_CH_MASK(CY_IPC_CHAN_CYPIPE_EP2)
#define CY_IPC_CYPIPE_INTR_MASK_EP2    CY_IPC_INTR_MASK(CY_IPC_INTR_CYPIPE_EP2)
#define CY_IPC_INTR_CYPIPE_PRIOR_EP2   (1UL)
#define CY_IPC_INTR_CYPIPE_MUX_EP2     (CY_IPC0_INTR_MUX(CY_IPC_INTR_CYPIPE_EP2))
#define CM55_IPC_PIPE_EP_ADDR          (2UL)
#define CM55_IPC_PIPE_CLIENT_ID        (5UL)

/* Combined Interrupt Mask (used in pipe config) */
#define CY_IPC_CYPIPE_INTR_MASK        (CY_IPC_CYPIPE_CHAN_MASK_EP1 | CY_IPC_CYPIPE_CHAN_MASK_EP2)

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/** CM33: Initialize IPC pipe infrastructure (semaphores, config, init) */
void cm33_ipc_communication_setup(void);

/** CM33: IPC pipe ISR handler */
void cm33_ipc_pipe_isr(void);

/** CM55: Initialize IPC pipe infrastructure (config, init) */
void cm55_ipc_communication_setup(void);

/** CM55: IPC pipe ISR handler */
void cm55_ipc_pipe_isr(void);

#endif /* IPC_COMMUNICATION_H */
