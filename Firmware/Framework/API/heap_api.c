/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "heap_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/
 
/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

static SemaphoreHandle_t g_heap_mutex = NULL;
static StaticSemaphore_t g_heap_mutex_bufer = {0};

/**********************************************************************************************************************
 * Exported variables and references
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of private functions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Definitions of exported functions
 *********************************************************************************************************************/

bool Heap_API_Init (void) {
    if (NULL == g_heap_mutex) {
        g_heap_mutex = xSemaphoreCreateRecursiveMutexStatic(&g_heap_mutex_bufer);
    }

    if (NULL == g_heap_mutex) {
        return false;
    }

    return true;
}

void *Heap_API_MemoryAllocate(const size_t number_of_elements, const size_t size) {
    if ((0 == number_of_elements) || (0 == size)) {
        return NULL;
    }

    if (NULL == g_heap_mutex) {
        return NULL;
    }
    
    if (pdTRUE != xSemaphoreTakeRecursive(g_heap_mutex, HEAP_API_MUTEX_TIMEOUT)) {
        return NULL;
    }

    void *allocated_memory = NULL;

    allocated_memory = calloc(number_of_elements, size);

    xSemaphoreGiveRecursive(g_heap_mutex);

    return allocated_memory;
}

bool Heap_API_Free (void *pointer_to_memory) {
    if (NULL == pointer_to_memory) {
        return false;
    }

    if (NULL == g_heap_mutex) {
        return false;
    }
    
    if (pdTRUE != xSemaphoreTakeRecursive(g_heap_mutex, HEAP_API_MUTEX_TIMEOUT)) {
        return false;
    }

    free(pointer_to_memory);

    xSemaphoreGiveRecursive(g_heap_mutex);

    return true;
}
