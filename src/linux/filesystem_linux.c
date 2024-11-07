#include "mydefines.h"

#ifdef YPLATFORM_LINUX

#include "core/filesystem.h"
#include "core/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

int
OsFopen(FILE** pFile, const char* pFilePath, const char* pMode)
{
	(*pFile) = fopen(pFilePath, pMode);
	if (!pFile)
		return 1;
	return 0;
}
/**
 * Reads up to bufferSize element in pBuffer and returns the amount read
 */
size_t
OsFread(void* pBuffer, size_t bufferSize, size_t elementSize, size_t elementCount, FILE* pStream)
{
	/* TODO: Change error handling here */
	size_t count = 0;
	size_t returnValue = 0;
	while (1)
	{
		returnValue = fread(pBuffer, elementSize, elementCount, pStream);
		count += returnValue;
		if (returnValue < bufferSize)
			break;
		if (ferror(pStream))
		{
			YERROR2("Write error");
			clearerr(pStream);
			exit(1);
		}
		else if (feof(pStream))
			return count;
	}
	return count;
}

int
OsFclose(FILE* pStream)
{
	return fclose(pStream);
}

int
OsStrError(YMB char *pBuffer,YMB  size_t bufSize,YMB  int errnum)
{
	return 1;
}

#endif // YPLATFORM_LINUX
