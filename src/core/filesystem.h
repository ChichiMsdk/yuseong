#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>

/**
 * Return 0 for success
 */
int OsFopen(
		FILE**								pFile,
		const char*							pFilePath,
		const char*							pMode);

size_t OsFread(
		void*								pBuffer,
		size_t								bufferSize,
		size_t								elementSize,
		size_t								elementCount,
		FILE*								pStream);

int OsFclose(
		FILE*								pStream);

int OsStrError(
		char*								pBuffer,
		size_t								bufSize,
		int									errnum);

#endif // FILESYSTEM_H
