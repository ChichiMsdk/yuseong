#ifndef YERROR_H
#define YERROR_H

#include <stdint.h>

typedef enum YuResult
{
	YU_SUCCESS = 0x00,
	YU_FAILURE = 0x01,
} YuResult;

typedef enum ErrorType
{
	VK = 0x00,
	YU = 0x01,
	GL = 0x02,
	MAX_TYPE
}ErrorType;

typedef struct ErrorCode
{
	ErrorType		type;
	int64_t			code;
} ErrorCode;

#define RETURN_ERROR_VK(errcode) return(ErrorCode){.type = VK, .code = errcode}
#define RETURN_ERROR_GL(errcode) return(ErrorCode){.type = GL, .code = errcode}
#define RETURN_ERROR_YU(errcode) return(ErrorCode){.type = YU, .code = errcode}
#endif //YERROR_H
