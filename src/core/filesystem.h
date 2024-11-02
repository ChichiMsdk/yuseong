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

int OsFread(
		void*								pBuffer,
		FILE*								pStream,
		size_t								elementSize,
		size_t								elementCount,
		size_t								bufferSize);

int OsFclose(
		FILE*								pStream);

int OsStrError(
		char*								pBuffer,
		size_t								bufSize,
		int									errnum);

#endif // FILESYSTEM_H
