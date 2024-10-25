/******************************************************************************
 * 						JASB. Just A Simple Builder.                          *
 ******************************************************************************
 * JASB will look for all the ".o.json" files and put them in one             *
 * compile_commands.json at the root directory. It is intended to be used with*
 * a Makefile that will compile it and then use after each build. As sed could*
 * differ from os and shell's this one should actually be portable.           *
 ******************************************************************************/

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <strsafe.h>
	#include <corecrt_io.h>
#elif __linux__
	#include <stdarg.h>
	#include <ftw.h>
	#include <dirent.h>
	#include <errno.h>
	#define MAX_PATH 320
	char pError[1124];
#endif // WIN32

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* NOTE: YES ! All on the heap. */
typedef struct sFile
{
	char *pFileName;
	char *pDirName;

	/* NOTE: I'm insecure */
	int maxSizeForAll;

	/* TODO: make this absolute */
	char *pFullPath;
}sFile;

typedef struct FileList
{
	char **ppList;
	sFile *pFiles;
	int nbElems;
	int sizeMax;
}FileList;

#ifdef _WIN32
void
ErrorExit(LPCTSTR lpszFunction, DWORD dw)
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
	printf("%lu\n", dw);
    ExitProcess(dw); 
}

int
isValidDirWin(const char *str, DWORD dwAttr)
{
	if (strcmp(str, ".") && strcmp(str, ".."))
	{
		if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
			return 1;
	}
	return 0;
}

#elif __linux__
#define isValidDirWin(...) isValidDirWinImpl(__VA_ARGS__, NULL)
int
isValidDirWinImpl(const char *pStr, int attr, ...)
{
	va_list args;
	va_start(args, attr);
	char *pArgStr = NULL;
	while ((pArgStr = va_arg(args, char *)) != NULL)
	{
		if (!strcmp(pArgStr, pStr))
		{
			va_end(args);
			return 0;
		}
	}
	if (strcmp(pStr, ".") && strcmp(pStr, ".."))
	{
		if ((unsigned char)attr == DT_DIR)
			return va_end(args), 1;
	}
	va_end(args);
	return 0;
}
#endif // WIN32

void
findDirWin(const char *path)
{
#ifdef _WIN32
	WIN32_FIND_DATA findData;
	char newPath[MAX_PATH];
	sprintf(newPath, "%s\\*", path);
	/* printf("newPath1: %s\n", newPath); */
	HANDLE hFind = FindFirstFile(newPath, &findData);
	memset(newPath, 0, sizeof(newPath) / sizeof(newPath[0]));

	if (hFind == INVALID_HANDLE_VALUE) { printf("FindFirstDir failed (%lu)\n", GetLastError()); return; } 
	do
	{
		if (isValidDirWin(findData.cFileName, findData.dwFileAttributes))
		{
			/* printf("Dir: %s\n", findData.cFileName); */
			sprintf(newPath, "%s\\%s", path, findData.cFileName);
			printf("Dir: %s\n", newPath);
			findDirWin(newPath);
		}
	}
	while (FindNextFile(hFind, &findData) != 0);
#elif __linux__
#endif
}

void
DestroyFileList(FileList *pFileList)
{
	for (int i = 0; i < pFileList->sizeMax; i++)
	{
		free(pFileList->pFiles[i].pDirName);
		free(pFileList->pFiles[i].pFileName);
		free(pFileList->pFiles[i].pFullPath);
	}
	free(pFileList->pFiles);
	free(pFileList);
}

int
InitFileList(FileList **ppFileList)
{
	int error = 0;
	int randomNbr = 100;

	/* NOTE: Check those allocs ? */
	FileList *pTmp = malloc(sizeof(FileList));
	pTmp->nbElems = 0;
	pTmp->pFiles = malloc(sizeof(sFile) * randomNbr);
	for (int i = 0; i < randomNbr; i++)
	{
		pTmp->pFiles[i].pDirName = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].pFileName = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].pFullPath = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].maxSizeForAll = MAX_PATH;
	}
	pTmp->sizeMax = randomNbr;
	*ppFileList = pTmp;

	return error;
}

int WildcardMatch(const char *pStr, const char *pPattern);

/* TODO: Construct absolute path instead of relative */
void
GetFilesWin(const char *pPath, const char *pRegex, sFile *pFiles, int *pNb)
{
#ifdef WIN32
	WIN32_FIND_DATA findFileData;
	char pSearchPath[MAX_PATH];
	sprintf(pSearchPath, "%s\\%s", pPath, pRegex);
	HANDLE hFind = FindFirstFile(pSearchPath, &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetLastError();
		if (dw != ERROR_FILE_NOT_FOUND)
			ErrorExit(pSearchPath, dw);
		return ; 
	}
	strncpy(pFiles[*pNb].pDirName, pPath, MAX_PATH);
	do
	{
		strncpy(pFiles[*pNb].pFileName, findFileData.cFileName, MAX_PATH);
		sprintf(pFiles[*pNb].pFullPath, "%s\\%s", pPath, findFileData.cFileName);
		(*pNb)++;
		/* printf("%s\n", fullPath); //findFileData.cFileName); */
	}
	while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
#elif __linux__
	DIR *pdStream;
	struct dirent *pdEntry;
	char pFullPath[1024];
	char pSearchPath[1024];
	int index = 0;
	int j = 0;

	sprintf(pSearchPath, "%s/.", pPath);
	sprintf(pError, "Couldn't open '%s'", pSearchPath);
	if ((pdStream = opendir(pSearchPath)) == NULL) { perror(pError); return ; }
	strncpy(pFiles[*pNb].pDirName, pPath, MAX_PATH);
	while ((pdEntry = readdir(pdStream)) != NULL)
	{
		if (pdEntry->d_type == DT_REG)
		{
			if (WildcardMatch(pdEntry->d_name, pRegex))
			{
				int lenName = strlen(pdEntry->d_name);
				sprintf(pFiles[*pNb].pObjName, "%s/", pBuildFolder);
				int lenObj = strlen(pFiles[*pNb].pObjName);
				for (int i = 0; i < lenName; i++)
				{
					if (pdEntry->d_name[i] == '.')
					{
						pFiles[*pNb].pObjName[i + lenObj] = pdEntry->d_name[i];
						pFiles[*pNb].pObjName[++i + lenObj] = 'o';
						pFiles[*pNb].pObjName[++i + lenObj] = 0;
						break;
					}
					pFiles[*pNb].pObjName[i + lenObj] = pdEntry->d_name[i];
				}
				strncpy(pFiles[*pNb].pFileName, pdEntry->d_name, MAX_PATH);
				sprintf(pFiles[*pNb].pFullPath, "%s/%s", pPath, pdEntry->d_name);
				(*pNb)++;
			}
		}
	}
	closedir(pdStream);
#endif
}

#define STACK_SIZE 1024

/* TODO: more robust way please, this is clunky */
char ** 
GetFilesDirIter(const char *pBasePath) 
{
#ifdef _WIN32
	char **ppDir = malloc(sizeof(char *) * STACK_SIZE);
	for (int i = 0; i < STACK_SIZE; i++)
	{
		ppDir[i] = malloc(sizeof(char) * MAX_PATH);
		memset(ppDir[i], 0, MAX_PATH);
	}
	int index = 0;

	strncpy(ppDir[index], pBasePath, MAX_PATH);
	char fullPath[1024];
	WIN32_FIND_DATA fileInfo;
	int j = 0;
	do
	{
		char *currentPath = ppDir[index];
		char searchPath[1024];

		sprintf(searchPath, "%s\\*", currentPath);
		HANDLE handle = FindFirstFile(searchPath, &fileInfo);
		if (handle == INVALID_HANDLE_VALUE)
			fprintf(stderr, "Unable to open directory: Error(%lu)\n", GetLastError());
		do
		{
			if (isValidDirWin(fileInfo.cFileName, fileInfo.dwFileAttributes))
			{
				j++;
				memset(fullPath, 0, sizeof(fullPath) / sizeof(fullPath[0]));
				sprintf(fullPath, "%s\\%s", currentPath, fileInfo.cFileName);

				if (j < STACK_SIZE) strncpy(ppDir[j], fullPath, MAX_PATH);
				else fprintf(stderr, "Stack overflow: too many directories to handle.\n");
			}
			if (FindNextFile(handle, &fileInfo) == 0) { break; }
		} while (1);

		FindClose(handle);
		index++;

	} while (ppDir[index][0]);
	return ppDir;
#elif __linux__
	DIR *pdStream;
	struct dirent *pdEntry;
	char pFullPath[1024];
	char pSearchPath[1024];
	int index = 0;
	int j = 0;

	char **ppDir = malloc(sizeof(char *) * STACK_SIZE);
	for (int i = 0; i < STACK_SIZE; i++)
	{
		ppDir[i] = malloc(sizeof(char) * MAX_PATH);
		memset(ppDir[i], 0, MAX_PATH);
	}
	strncpy(ppDir[index], pBasePath, MAX_PATH);

	do
	{
		char *pCurrentPath = ppDir[index];
		sprintf(pSearchPath, "%s/.", pCurrentPath);
		sprintf(pError, "Couldn't open '%s'", pSearchPath);
		if ((pdStream = opendir(pSearchPath)) == NULL) { perror(pError); return NULL; }
		while ((pdEntry = readdir(pdStream)) != NULL)
		{
			if (isValidDirWin(pdEntry->d_name, pdEntry->d_type, ".git"))
			{
				j++;
				memset(pFullPath, 0, sizeof(pFullPath) / sizeof(pFullPath[0]));
				sprintf(pFullPath, "%s/%s", pCurrentPath, pdEntry->d_name);
				/* printf("pFullPath[%d] at index[%d]: %s\n", j, index, pFullPath); */
				if (j < STACK_SIZE) strncpy(ppDir[j], pFullPath, MAX_PATH);
				else
				{
					fprintf(stderr, "Stack overflow:"
							"too many directories to handle:"
							" %s\n", pFullPath);
					goto panic;
				}
			}
		}
		closedir(pdStream);
		index++;
	} while (ppDir[index][0]);
	return ppDir;
panic:
	closedir(pdStream);
	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return NULL;
#endif // WIN32
}

/*
 * TODO:
 * - Add option: iterative or not
 * - Add va_args to specify multiple files manually
 */
FileList *
GetFileList(const char *path, const char *regex)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
	char **ppDir = GetFilesDirIter(path); // free this somewhere
	for (int i = 0; ppDir[i][0]; i++)
	{
		GetFilesWin(ppDir[i], regex, pFileList->pFiles, &pFileList->nbElems);
	}

	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return pFileList;
}

void
PrintFileList(FileList *pList)
{
	for (int i = 0; i < pList->nbElems; i++)
	{
		printf("File %d: %s\n", i, pList->pFiles[i].pFullPath);
	}
}

#define CCJSON_BEGIN "[\n"
#define CCJSON_END "]"
#define STEPS 3000
#define START_ALLOC 10240

int
FlushIt(char *pLine, FILE* pFd, size_t size)
{
	int count = fprintf(pFd, "%s", pLine);
	memset(pLine, 0, size);
	return count;
}

int
main(int argc, char **ppArgv)
{
	if (argc != 2)
	{
		fprintf(stderr, "Usage: jasb <regex>\n");
		return 1;
	}
	char *pFolder = ".";
	char *pRegex = ppArgv[1];
	char *pCompileCommands = "compile_commands.json";

	FileList *CFiles = GetFileList(pFolder, pRegex);
	/* PrintFileList(CFiles); */
	FILE *pFd1 = fopen(pCompileCommands, "w");
	if (!pFd1)
	{
		fprintf(stderr, "Cp2\n");
		perror(pCompileCommands);
		DestroyFileList(CFiles);
		return 1;
	}
	fprintf(pFd1, "%s", CCJSON_BEGIN);
	size_t totalSize = 0;
	size_t count = 0;
	char *pLine = calloc(START_ALLOC, sizeof(char));
	size_t i = 0;
	int fd = 0;
	size_t lastOne = CFiles->nbElems - 1; 
	while (i < CFiles->nbElems)
	{
		fd = _open(CFiles->pFiles[i].pFullPath, _O_RDONLY);
		if (!fd)
			goto exiting;
		while (1)
		{
			count = _read(fd, pLine, STEPS);
			totalSize+=count;
			if (totalSize >= (START_ALLOC - STEPS))
			{
				fprintf(stderr, "Sorry line was too big. I didn't write code to realloc\n");
				errno = EOVERFLOW;
				goto exiting;
			}
			if (count < STEPS)
			{
				if (i == lastOne)
				{
					fwrite(pLine, sizeof(char), totalSize - 2, pFd1);
					fwrite("\n", sizeof(char), strlen("\n"), pFd1);
					break;
				}
				FlushIt(pLine, pFd1, totalSize);
				totalSize = 0;
				break;
			}
			if (count < 0)
				goto exiting;
		}
		_close(fd);
		i++;
	}
	fprintf(pFd1, "%s", CCJSON_END);
	fclose(pFd1);
	DestroyFileList(CFiles);
	return 0;

exiting:
	perror(CFiles->pFiles->pFullPath);
	DestroyFileList(CFiles);
	_close(fd);
	fclose(pFd1);
	return 1;
}

