/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/*
 * This file contains the function prototypes of ESP-IDF specific API additions
 * to the FreeRTOS kernel. These API additions are not part of Vanilla (i.e.,
 * upstream) FreeRTOS and include things such as....
 *    - Various helper functions
 *    - API for ESP-IDF feature additions to FreeRTOS (such as TSLP deletion
 * call backs)
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
    extern "C" {
#endif

/* -----------------------------------------------------------------------------
 * SMP related API additions to FreeRTOS
 *
 * Todo: Move IDF FreeRTOS SMP related additions to this header as well (see
 * IDF-7201)
 * -------------------------------------------------------------------------- */

#if CONFIG_FREERTOS_SMP || __DOXYGEN__

/**
 * @brief Create a new task that is pinned to a particular core
 *
 * Helper function to create a task that is pinned to a particular core, or has
 * no affinity. In other wrods, the created task will have an affinity mask of:
 * - (1 << xCoreID) if it is pinned to a particular core
 * - Set to tskNO_AFFINITY if it has no affinity
 *
 * @param pxTaskCode Pointer to the task entry function.
 * @param pcName A descriptive name for the task.
 * @param usStackDepth The size of the task stack.
 * @param pvParameters Pointer that will be used as the parameter for the task
 * being created.
 * @param uxPriority The priority at which the task should run.
 * @param pxCreatedTask Used to pass back a handle by which the created task can
 * be referenced.
 * @param xCoreID The core to which the task is pinned to, or tskNO_AFFINITY if
 * the task has no core affinity
 * @return pdPASS if the task was successfully created and added to a ready
 * list, otherwise an error code defined in the file projdefs.h
 */
    BaseType_t xTaskCreatePinnedToCore( TaskFunction_t pxTaskCode,
                                        const char * const pcName,
                                        const uint32_t usStackDepth,
                                        void * const pvParameters,
                                        UBaseType_t uxPriority,
                                        TaskHandle_t * const pxCreatedTask,
                                        const BaseType_t xCoreID );


/**
 * @brief Create a new static task that is pinned to a particular core
 *
 * This funciton is the static equivalent of xTaskCreatePinnedToCore().
 *
 * @param pxTaskCode Pointer to the task entry function.
 * @param pcName A descriptive name for the task.
 * @param ulStackDepth The size of the task stack.
 * @param pvParameters Pointer that will be used as the parameter for the task
 * being created.
 * @param uxPriority The priority at which the task should run.
 * @param puxStackBuffer Must point to a StackType_t array that has at least
 * ulStackDepth indexes
 * @param pxTaskBuffer Must point to a variable of type StaticTask_t, which will
 * then be used to hold the task's data structures,
 * @param xCoreID The core to which the task is pinned to, or tskNO_AFFINITY if
 * the task has no core affinity
 * @return The task handle if the task was created, NULL otherwise.
 */
    #if ( configSUPPORT_STATIC_ALLOCATION == 1 )
        TaskHandle_t xTaskCreateStaticPinnedToCore( TaskFunction_t pxTaskCode,
                                                    const char * const pcName,
                                                    const uint32_t ulStackDepth,
                                                    void * const pvParameters,
                                                    UBaseType_t uxPriority,
                                                    StackType_t * const puxStackBuffer,
                                                    StaticTask_t * const pxTaskBuffer,
                                                    const BaseType_t xCoreID );
    #endif /* configSUPPORT_STATIC_ALLOCATION */

/**
 * @brief Get the handle of the task running on a certain core
 *
 * Because of the nature of SMP processing, there is no guarantee that this
 * value will still be valid on return and should only be used for debugging
 * purposes.
 *
 * [refactor-todo] Mark this function as deprecated, call
 * xTaskGetCurrentTaskHandleCPU() instead
 *
 * @param xCoreID The core to query
 * @return Handle of the current task running on the queried core
 */
    TaskHandle_t xTaskGetCurrentTaskHandleForCPU( BaseType_t xCoreID );

/**
 * @brief Get the handle of idle task for the given CPU.
 *
 * [refactor-todo] Mark this function as deprecated, call
 * xTaskGetIdleTaskHandle() instead
 *
 * @param xCoreID The core to query
 * @return Handle of the idle task for the queried core
 */
    TaskHandle_t xTaskGetIdleTaskHandleForCPU( BaseType_t xCoreID );

/**
 * @brief Get the current core affintiy of a particular task
 *
 * Helper function to get the core affinity of a particular task. If the task is
 * pinned to a particular core, the core ID is returned. If the task is not
 * pinned to a particular core, tskNO_AFFINITY is returned.
 *
 * [refactor-todo] Mark this function as deprecated, call vTaskCoreAffinityGet()
 * instead
 *
 * @param xTask The task to query
 * @return The tasks coreID or tskNO_AFFINITY
 */
    BaseType_t xTaskGetAffinity( TaskHandle_t xTask );

#endif // CONFIG_FREERTOS_SMP || __DOXYGEN__

/* -----------------------------------------------------------------------------
 * TLSP Deletion Callback related API additions
 *
 * Todo: Move IDF FreeRTOS TLSP Deletion Callback related additions to this
 * header as well (see IDF-7201)
 * -------------------------------------------------------------------------- */

#if CONFIG_FREERTOS_SMP || __DOXYGEN__

    #if ( CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS )

/**
 * Prototype of local storage pointer deletion callback.
 */
        typedef void (* TlsDeleteCallbackFunction_t)( int,
                                                      void * );

/**
 * Set local storage pointer and deletion callback.
 *
 * Each task contains an array of pointers that is dimensioned by the
 * configNUM_THREAD_LOCAL_STORAGE_POINTERS setting in FreeRTOSConfig.h. The
 * kernel does not use the pointers itself, so the application writer can use
 * the pointers for any purpose they wish.
 *
 * Local storage pointers set for a task can reference dynamically allocated
 * resources. This function is similar to vTaskSetThreadLocalStoragePointer, but
 * provides a way to release these resources when the task gets deleted. For
 * each pointer, a callback function can be set. This function will be called
 * when task is deleted, with the local storage pointer index and value as
 * arguments.
 *
 * @param xTaskToSet  Task to set thread local storage pointer for
 * @param xIndex The index of the pointer to set, from 0 to
 * configNUM_THREAD_LOCAL_STORAGE_POINTERS - 1.
 * @param pvValue Pointer value to set.
 * @param pvDelCallback  Function to call to dispose of the local storage
 * pointer when the task is deleted.
 */
        void vTaskSetThreadLocalStoragePointerAndDelCallback( TaskHandle_t xTaskToSet,
                                                              BaseType_t xIndex,
                                                              void * pvValue,
                                                              TlsDeleteCallbackFunction_t pvDelCallback );
    #endif // CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS

#endif // CONFIG_FREERTOS_SMP || __DOXYGEN__

#ifdef __cplusplus
    }
#endif
