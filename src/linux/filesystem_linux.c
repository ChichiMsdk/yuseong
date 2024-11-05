#include "mydefines.h"

#ifdef YPLATFORM_LINUX

#include "core/filesystem.h"
#include "core/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
	while (count < bufferSize)
	{
		count += fread(pBuffer, elementSize, elementCount, pStream);
		if (ferror(pStream))
		{
			YERROR2("Write error");
			clearerr(pStream);
			exit(1);
		}
	}
	return count;
}

int
OsFclose(FILE* pStream)
{
	return fclose(pStream);
}

int
OsStrError(char *pBuffer, size_t bufSize, int errnum)
{
	return strerror_r(errnum, pBuffer, bufSize);
}

#endif // YPLATFORM_LINUX