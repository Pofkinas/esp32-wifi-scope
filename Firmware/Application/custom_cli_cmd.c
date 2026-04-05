
/***********************************************************************************************************************
 * @file 
 * @brief Custom CLI command handler file for the Pofkinas Development Framework (PDF).
 * 
 * This file is part of the Pofkinas Development Framework (PDF) and contains custom command handler implimentation for the CLI application.
 * 
 * @note Place this file in the Application/ folder of your project define.
 * 
 * @details
 * custom_cli_cmd_handler.c
 * 
 * Usage:
 * 1. Place this file in your Application/ folder of your project (e.g. ProjectName/Application/).
 * 2. Add PDF (Pofkinas Development Framework) to your project. Latest version can be found at: https://github.com/Pofkinas/pdf
 * 3. Include the used module headers.
 * 4. Define command seperator symbol (e.g. `,`) and seperator length (e.g. `sizeof(SEPARATOR) - 1`).
 * 5. Imlement the custom commands.
 * 6. Use `CMD_API_Helper_FindNextArgUInt` to parse the command arguments.
 * 7. Implement the custom commands handler definition header `custom_cli_cmd_handler.h`.
 ***********************************************************************************************************************/

/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "custom_cli_cmd.h"

#if defined(ENABLE_CUSTOM_CMD)
#include <string.h>
#include "oscilloscope_app.h"
#include "cmd_api_helper.h"
#include "debug_api.h"
#include "error_messages.h"

/**********************************************************************************************************************
 * Private definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private typedef
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private constants
 *********************************************************************************************************************/

#if defined(DEBUG_CUSTOM_CMD)
CREATE_MODULE_NAME(CLI_CUSTOM_CMD)
#else
CREATE_MODULE_NAME_EMPTY
#endif /* DEBUG_CUSTOM_CMD */

/**********************************************************************************************************************
 * Private variables
 *********************************************************************************************************************/

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

eErrorCode_t Custom_CLI_CMD_OscilloscopeStart(sMessage_t arguments, sMessage_t *response) {
    if (NULL == response) {
        TRACE_ERR("Invalid data pointer\n");

        return eErrorCode_NULLPTR;
    }

    if (NULL == response->data) {
        TRACE_ERR("Invalid response data pointer\n");

        return eErrorCode_NULLPTR;
    }

    if (!Oscilloscope_APP_Start()) {
        sprintf(response->data, "Failed to start oscilloscope\n");

        return eErrorCode_FAILED;
    }

    snprintf(response->data, response->size, "Operation successful\n");

    return eErrorCode_OK;
}

eErrorCode_t Custom_CLI_CMD_OscilloscopeStop(sMessage_t arguments, sMessage_t *response) {
    if (NULL == response) {
        TRACE_ERR("Invalid data pointer\n");

        return eErrorCode_NULLPTR;
    }

    if (NULL == response->data) {
        TRACE_ERR("Invalid response data pointer\n");

        return eErrorCode_NULLPTR;
    }

    if (!Oscilloscope_APP_Stop()) {
        snprintf(response->data, response->size, "Failed to stop oscilloscope\n");

        return eErrorCode_FAILED;
    }

    snprintf(response->data, response->size, "Operation successful\n");

    return eErrorCode_OK;
}

#endif /* ENABLE_CUSTOM_CMD */
