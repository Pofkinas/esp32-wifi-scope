#ifndef SOURCE_UTILITY_CMD_PARSER_H_
#define SOURCE_UTILITY_CMD_PARSER_H_
/**********************************************************************************************************************
 * Includes
 *********************************************************************************************************************/

#include "framework_config.h"

#if defined(ENABLE_CMD_PARSER)
#include <stddef.h>
#include "message.h"
#include "error_messages.h"

/**********************************************************************************************************************
 * Exported definitions and macros
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported types
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Prototypes of exported functions
 *********************************************************************************************************************/

eErrorCode_t CMD_Parser_ParseToken(char **token, sMessage_t *argument, char *separator, sMessage_t *response);
eErrorCode_t CMD_Parser_FindNextArgUInt(sMessage_t *argument, size_t *return_argument, char *separator, const size_t separator_lenght, sMessage_t *response);
eErrorCode_t CMD_Parser_FindNextArgInt(sMessage_t *argument, int *return_argument, char *separator, const size_t separator_lenght, sMessage_t *response);
eErrorCode_t CMD_Parser_FindNextArgFloat(sMessage_t *argument, float *return_argument, char *separator, const size_t separator_lenght, sMessage_t *response);
eErrorCode_t CMD_Parser_FindNextArgChar(sMessage_t *argument, char *return_argument, char *separator, const size_t separator_lenght, sMessage_t *response);

#endif /* ENABLE_CMD_PARSER */
#endif /* SOURCE_UTILITY_CMD_PARSER_H_ */
