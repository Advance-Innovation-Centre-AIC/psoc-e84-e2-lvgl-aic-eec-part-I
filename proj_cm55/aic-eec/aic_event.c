/*******************************************************************************
 * File: aic_event.c
 * Description: AIC-EEC Event Bus System Implementation
 *
 * Publish-Subscribe event system for decoupling sensor updates from UI.
 *
 * Part of BiiL Course: Embedded C for IoT
 ******************************************************************************/

#include "aic_event.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

/*******************************************************************************
 * Type Definitions
 ******************************************************************************/

typedef struct {
    aic_event_cb_t callback;
    void *user_data;
} subscriber_t;

typedef struct {
    aic_event_t event;
    aic_event_data_t data;
    bool has_data;
} event_entry_t;

/*******************************************************************************
 * Static Variables
 ******************************************************************************/

static bool event_initialized = false;
static subscriber_t subscribers[AIC_EVENT_MAX][AIC_EVENT_MAX_SUBSCRIBERS];
static uint8_t subscriber_counts[AIC_EVENT_MAX];

static QueueHandle_t event_queue = NULL;
static TaskHandle_t event_task_handle = NULL;
static SemaphoreHandle_t event_mutex = NULL;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/

static void deliver_event(aic_event_t event, const aic_event_data_t *data)
{
    if (event >= AIC_EVENT_MAX) {
        return;
    }

    /* Take mutex for thread-safe access to subscribers */
    if (xSemaphoreTake(event_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }

    /* Call all subscribers for this event */
    for (uint8_t i = 0; i < subscriber_counts[event]; i++) {
        if (subscribers[event][i].callback != NULL) {
            subscribers[event][i].callback(event, data, subscribers[event][i].user_data);
        }
    }

    xSemaphoreGive(event_mutex);
}

/*******************************************************************************
 * Event Task
 ******************************************************************************/

static void event_task(void *param)
{
    (void)param;
    event_entry_t entry;

    while (1) {
        if (xQueueReceive(event_queue, &entry, portMAX_DELAY) == pdTRUE) {
            deliver_event(entry.event, entry.has_data ? &entry.data : NULL);
        }
    }
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

bool aic_event_init(void)
{
    if (event_initialized) {
        return true;
    }

    /* Clear subscriber arrays */
    memset(subscribers, 0, sizeof(subscribers));
    memset(subscriber_counts, 0, sizeof(subscriber_counts));

    /* Create queue */
    event_queue = xQueueCreate(AIC_EVENT_QUEUE_SIZE, sizeof(event_entry_t));
    if (event_queue == NULL) {
        return false;
    }

    /* Create mutex */
    event_mutex = xSemaphoreCreateMutex();
    if (event_mutex == NULL) {
        vQueueDelete(event_queue);
        event_queue = NULL;
        return false;
    }

    /* Create task */
    aic_event_create_task();

    event_initialized = true;
    return true;
}

void aic_event_deinit(void)
{
    if (!event_initialized) {
        return;
    }

    aic_event_delete_task();

    if (event_mutex != NULL) {
        vSemaphoreDelete(event_mutex);
        event_mutex = NULL;
    }

    if (event_queue != NULL) {
        vQueueDelete(event_queue);
        event_queue = NULL;
    }

    /* Clear all subscribers */
    memset(subscribers, 0, sizeof(subscribers));
    memset(subscriber_counts, 0, sizeof(subscriber_counts));

    event_initialized = false;
}

bool aic_event_is_init(void)
{
    return event_initialized;
}

bool aic_event_subscribe(aic_event_t event, aic_event_cb_t callback, void *user_data)
{
    if (event >= AIC_EVENT_MAX || callback == NULL) {
        return false;
    }

    if (event_mutex == NULL) {
        return false;
    }

    if (xSemaphoreTake(event_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    bool result = false;

    /* Check if already subscribed */
    for (uint8_t i = 0; i < subscriber_counts[event]; i++) {
        if (subscribers[event][i].callback == callback) {
            /* Already subscribed, update user_data */
            subscribers[event][i].user_data = user_data;
            result = true;
            goto done;
        }
    }

    /* Add new subscriber */
    if (subscriber_counts[event] < AIC_EVENT_MAX_SUBSCRIBERS) {
        subscribers[event][subscriber_counts[event]].callback = callback;
        subscribers[event][subscriber_counts[event]].user_data = user_data;
        subscriber_counts[event]++;
        result = true;
    }

done:
    xSemaphoreGive(event_mutex);
    return result;
}

bool aic_event_unsubscribe(aic_event_t event, aic_event_cb_t callback)
{
    if (event >= AIC_EVENT_MAX || callback == NULL) {
        return false;
    }

    if (event_mutex == NULL) {
        return false;
    }

    if (xSemaphoreTake(event_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    bool result = false;

    for (uint8_t i = 0; i < subscriber_counts[event]; i++) {
        if (subscribers[event][i].callback == callback) {
            /* Found - shift remaining subscribers */
            for (uint8_t j = i; j < subscriber_counts[event] - 1; j++) {
                subscribers[event][j] = subscribers[event][j + 1];
            }
            subscriber_counts[event]--;

            /* Clear last slot */
            subscribers[event][subscriber_counts[event]].callback = NULL;
            subscribers[event][subscriber_counts[event]].user_data = NULL;

            result = true;
            break;
        }
    }

    xSemaphoreGive(event_mutex);
    return result;
}

void aic_event_unsubscribe_all(aic_event_t event)
{
    if (event >= AIC_EVENT_MAX) {
        return;
    }

    if (event_mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(event_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return;
    }

    for (uint8_t i = 0; i < AIC_EVENT_MAX_SUBSCRIBERS; i++) {
        subscribers[event][i].callback = NULL;
        subscribers[event][i].user_data = NULL;
    }
    subscriber_counts[event] = 0;

    xSemaphoreGive(event_mutex);
}

bool aic_event_publish(aic_event_t event, const aic_event_data_t *data)
{
    if (event >= AIC_EVENT_MAX) {
        return false;
    }

    /* If no subscribers, don't bother queuing */
    if (subscriber_counts[event] == 0) {
        return true;  /* Success - nothing to do */
    }

    if (!event_initialized || event_queue == NULL) {
        /* Not initialized - deliver immediately */
        deliver_event(event, data);
        return true;
    }

    event_entry_t entry = {
        .event = event,
        .has_data = (data != NULL)
    };

    if (data != NULL) {
        memcpy(&entry.data, data, sizeof(aic_event_data_t));
    }

    /* Try to queue (don't block) */
    BaseType_t result;
    if (xPortIsInsideInterrupt()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        result = xQueueSendFromISR(event_queue, &entry, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        result = xQueueSend(event_queue, &entry, 0);
    }

    return (result == pdTRUE);
}

void aic_event_publish_immediate(aic_event_t event, const aic_event_data_t *data)
{
    if (event >= AIC_EVENT_MAX) {
        return;
    }

    deliver_event(event, data);
}

uint8_t aic_event_subscriber_count(aic_event_t event)
{
    if (event >= AIC_EVENT_MAX) {
        return 0;
    }
    return subscriber_counts[event];
}

uint32_t aic_event_queue_count(void)
{
    if (event_queue == NULL) {
        return 0;
    }
    return uxQueueMessagesWaiting(event_queue);
}

void aic_event_create_task(void)
{
    if (event_task_handle != NULL) {
        return;
    }

    xTaskCreate(
        event_task,
        "AIC_EVT",
        AIC_EVENT_TASK_STACK,
        NULL,
        AIC_EVENT_TASK_PRIORITY,
        &event_task_handle
    );
}

void aic_event_delete_task(void)
{
    if (event_task_handle != NULL) {
        vTaskDelete(event_task_handle);
        event_task_handle = NULL;
    }
}

void aic_event_process(void)
{
    if (!event_initialized || event_queue == NULL) {
        return;
    }

    event_entry_t entry;
    while (xQueueReceive(event_queue, &entry, 0) == pdTRUE) {
        deliver_event(entry.event, entry.has_data ? &entry.data : NULL);
    }
}
