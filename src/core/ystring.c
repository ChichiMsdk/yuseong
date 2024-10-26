#include "ystring.h"

#include <stdlib.h>

#include "core/logger.h"

/**
 * @brief Simple wrapper around atoi that returns -1 in
 * case no number was found
 *
 * @param pStr the string to convert
 * @return converted string as integer or -1 if no int were found
 */
YND int
yAtoi(const char *pStr)
{
	if (pStr[0] > '9' || pStr[0] < '0')
		return -1;
	return atoi(pStr);
}
