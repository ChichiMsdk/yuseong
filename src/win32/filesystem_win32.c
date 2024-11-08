#include "mydefines.h"

#ifdef YPLATFORM_WINDOWS
#include "core/filesystem.h"
#include "core/logger.h"

#include <stdio.h>
#include <string.h>

int
OsFopen(FILE** pFile, const char* pFilePath, const char* pMode)
{
	errno_t errcode = fopen_s(pFile, pFilePath, pMode);
	if (errcode != 0)
		YERROR("Path: %s", pFilePath);
	return errcode;
}

size_t
OsFread(void* pBuffer, size_t bufferSize, size_t elementSize, size_t elementCount, FILE* pStream)
{
	errno_t errcode = fread_s(pBuffer, bufferSize, elementSize, elementCount, pStream);
	return errcode;
}

int
OsFclose(FILE* pStream)
{
	return fclose(pStream);
}

int
OsStrError(char *pBuffer, size_t bufSize, int errnum)
{
	return strerror_s(pBuffer, bufSize, errnum);
}
#endif // YPLATFORM_WINDOWS
