#include "mydefines.h"

#include "core/filesystem.h"

#include <stdio.h>
#include <string.h>

int
OsFopen(FILE** pFile, const char* pFilePath, const char* pMode)
{
	errno_t errcode = fopen_s(pFile, pFilePath, pMode);
	return errcode;
}

int
OsFread(void* pBuffer, FILE* pStream, size_t elementSize, size_t elementCount, size_t bufferSize)
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
