/******************************************************************************
 * 						JASB. Just A Simple Builder.                          *
 ******************************************************************************
 * JASB will look for all the ".o.json" files and put them in one             *
 * compile_commands.json at the root directory. It is intended to be used with*
 * a Makefile that will compile it and then use after each build. As sed could*
 * differ from os and shell's this one should actually be portable.           *
 ******************************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <strsafe.h>
	#include <corecrt_io.h>
	#define YO_RDONLY _O_RDONLY
	#define OPEN(a, b) _open(a, b)
	#define CLOSE(a) _close(a)
	#define READ(a, b, c) _read(a, b, c)
#elif __linux__
	#include <stdarg.h>
	#include <ftw.h>
	#include <dirent.h>
	#include <errno.h>
	#include <unistd.h>
	#define MAX_PATH 320
	char pError[1124];

	#define YO_RDONLY O_RDONLY
	#define OPEN(a, b) open(a, b)
	#define CLOSE(a) close(a)
	#define READ(a, b, c) read(a, b, c)
#endif // WIN32

#define CCJSON_BEGIN "[\n"
#define CCJSON_END "]"
#define STEPS 3000
#define START_ALLOC 10240
#define MAX_SIZE_COMMAND 10000
#define STACK_SIZE 1024

void MakeCleanImpl([[maybe_unused]] void *none, ...);
int isValidDirImpl(const char *pStr, int attr, ...);

#define isValidDir(...) isValidDirImpl(__VA_ARGS__, NULL)
#define MakeClean(...) MakeCleanImpl(__VA_ARGS__, NULL)

/* NOTE: YES ! All on the heap. */
typedef struct sFile
{
	char *pFileName;
	char *pObjName;
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
	size_t nbElems;
	size_t sizeMax;
}FileList;

void
DestroyFileList(FileList *pFileList)
{
	for (size_t i = 0; i < pFileList->sizeMax; i++)
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
void GetFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb);

/* TODO: more robust way please, this is clunky */
char **GetFilesDirIter(const char *pBasePath);

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
		GetFiles(ppDir[i], regex, pFileList->pFiles, &pFileList->nbElems);
	}

	for (int i = 0; i < STACK_SIZE; i++)
		free(ppDir[i]);
	free(ppDir);
	return pFileList;
}

void
PrintFileList(FileList *pList)
{
	for (size_t i = 0; i < pList->nbElems; i++)
	{
		printf("File %zu: %s\n", i, pList->pFiles[i].pFullPath);
	}
}

int
FlushIt(char *pLine, FILE* pFd, size_t size)
{
	int count = fprintf(pFd, "%s", pLine);
	memset(pLine, 0, size);
	return count;
}

int
WildcardMatch(const char *pStr, const char *pPattern)
{
	const char *pStar = NULL;

	while (*pStr)
	{
		if (*pPattern == *pStr)
		{
			pPattern++;
			pStr++;
		}
		else if (*pPattern == '*')
			pStar = pPattern++;
		else if (pStar)
		{
			pPattern = pStar++;
			pStr++;
		}
		else
			return 0;
	}
	while (*pPattern == '*')
		pPattern++;
	return *pPattern == '\0';
}

int
ConstructCompileCommandsJson(FileList *pList, const char *pName)
{
	FILE *pFd1 = fopen(pName, "w");
	if (!pFd1)
	{
		fprintf(stderr, "Cp2\n");
		perror(pName);
		DestroyFileList(pList);
		return 1;
	}
	fprintf(pFd1, "%s", CCJSON_BEGIN);
	size_t totalSize = 0;
	size_t count = 0;
	char *pLine = calloc(START_ALLOC, sizeof(char));
	int fd = 0;
	size_t i = 0;
	size_t lastOne = pList->nbElems - 1; 
	while (i < pList->nbElems)
	{
		fd = OPEN(pList->pFiles[i].pFullPath, YO_RDONLY);
		if (!fd)
			goto exiting;
		while (1)
		{
			count = READ(fd, pLine, STEPS);
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
		CLOSE(fd);
		i++;
	}
	fprintf(pFd1, "%s", CCJSON_END);
	fclose(pFd1);
	return 0;
exiting:
	CLOSE(fd);
	fclose(pFd1);
	return 1;
}

char *
Compile(const char *cc, const char *flags, FileList *pList)
{
	if (pList->nbElems <= 0)
	{
		fprintf(stderr, "No files to compile found ..\n");
		return NULL;
	}
	char *pObjs;
	for (size_t i = 0; i < pList->nbElems; i++)
	{
		char *fname = pList->pFiles[i].pFullPath;
		char *outname = pList->pFiles[i].pObjName;
		char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
		size_t totalCount = strlen(cc) + strlen(flags) + strlen(fname) + strlen(outname);
		snprintf(command, totalCount + 10, "%s %s -c %s -o %s", cc, flags, fname, outname);
		printf("Command: %s\n", command);
		/* system(command); */
		free(command);
	}

	size_t count = 0;
	size_t total = 0;

	for (size_t i = 0; i < pList->nbElems; i++)
		total += strlen(pList->pFiles[i].pObjName);

	pObjs = malloc(sizeof(char) * total + (pList->nbElems) + 1);
	for (size_t i = 0; i < pList->nbElems; i++)
		count += sprintf(pObjs + count, "%s ", pList->pFiles[i].pObjName);
	pObjs[--count] = 0;
	if (!pObjs)
	{
		free(pObjs);
		return "";
	}
	return pObjs;
}

int
Link(const char *pCc, const char *pFlags, const char *pObj, const char *pOutname)
{
	char *pCommand = malloc(sizeof(char) * MAX_SIZE_COMMAND);
	size_t totalCount = strlen(pCc) + strlen(pFlags) + strlen(pObj) + strlen(pOutname);
	snprintf(pCommand, totalCount + 7, "%s %s %s -o %s", pCc, pFlags, pObj, pOutname);
	printf("Command: %s\n", pCommand);
	int error = system(pCommand);
	free(pCommand);
	return error;
}

int
Build(const char* pCompiler, const char* pCflags, FileList* pListCfiles, char* pObjs)
{
	char *pFinal = NULL;
	char *pOutput = "test";
	char *pBuildFolder = "build";
	char *pLibs = "";

	pObjs = Compile(pCompiler, pCflags, pListCfiles);
	pFinal = malloc(sizeof(char) * strlen(pOutput) + strlen(pBuildFolder) + 2);
	size_t count = sprintf(pFinal, "%s/%s", pBuildFolder, pOutput);
	pFinal[count--] = 0;
	int error = Link(pCompiler, pLibs, pObjs, pFinal);
	free(pFinal);
	return error;
}

#define JSONREGEX "*.o.json"

int
CreateCompileCommandsJson(const char *pCompileCommands)
{
	FileList *pListJson = GetFileList(".", JSONREGEX);
	if (!pListJson)
		return 1;
	return ConstructCompileCommandsJson(pListJson, pCompileCommands);
}

int GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb, const char *pBuildFolder);


int
main(int argc, char **ppArgv)
{
	if (argc == 2)
	{
		if (strcmp(ppArgv[1], "-h"))
			fprintf(stderr, "Usage: jasb\n");
		return 1;
	}
	char *pExtension = ".exe";
	char *pBuildFolder = "build";
	char *pRegex = ppArgv[1];
	char *pFolder = ".";
	char *pAsan = "";
	char *pCompileCommands = "compile_commands.json";
	char *pObjs = NULL;

	FileList *pListCfiles = GetFileList(pFolder, "*.c");
	FileList *pListCppFiles = GetFileList(pFolder, "*.cpp");
	PrintFileList(pListCfiles);
	PrintFileList(pListCppFiles);
	return 1;
	if (CreateCompileCommandsJson(pCompileCommands))
		goto exiting;

    /*
	 * if (strlen(pObjs) > 0)
	 * 	free(pObjs);
	 * free(pObjs2);
     */
	/* DestroyFileList(pListJson); */
	return 0;

exiting:
	fprintf(stderr, "Error");
	/* perror(pListJson->pFiles->pFullPath); */
	/* DestroyFileList(pListJson); */
	return 1;
}

/*****************************************************************************/
/*WINDOWS*/
/*****************************************************************************/
#ifdef WIN32

char ** 
GetFilesDirIter(const char *pBasePath) 
{
	char **ppDir = malloc(sizeof(char *) * STACK_SIZE);
	for (int i = 0; i < STACK_SIZE; i++)
	{
		ppDir[i] = malloc(sizeof(char) * MAX_PATH);
		memset(ppDir[i], 0, MAX_PATH);
	}
	int index = 0;

	strncpy(ppDir[index], basePath, MAX_PATH);
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
			if (isValidDir(fileInfo.cFileName, fileInfo.dwFileAttributes))
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
}

void
GetFiles(const char *pPath, const char *pRegex, sFile *pFiles, int *pNb, const char *pBuildFolder)
{
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
}

int
GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, int *pNb, const char *pBuildFolder)
{
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
		int lenName = strlen(findFileData.cFileName);
		sprintf(pFiles[*pNb].pObjName, "%s\\", pBuildFolder);
		int lenObj = strlen(pFiles[*pNb].pObjName);
		for (int i = 0; i < lenName; i++)
		{
			if (findFileData.cFileName[i] == '.')
			{
				pFiles[*pNb].pObjName[i + lenObj] = findFileData.cFileName[i];
				pFiles[*pNb].pObjName[++i + lenObj] = 'o';
				pFiles[*pNb].pObjName[++i + lenObj] = 0;
				break;
			}
			pFiles[*pNb].pObjName[i + lenObj] = findFileData.cFileName[i];
		}
		strncpy(pFiles[*pNb].pFileName, findFileData.cFileName, MAX_PATH);
		sprintf(pFiles[*pNb].pFullPath, "%s\\%s", pPath, findFileData.cFileName);
		(*pNb)++;
		/* printf("%s\n", fullPath); //findFileData.cFileName); */
	}
	while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}

void
findDirWin([[maybe_unused]] const char *path)
{
	WIN32_FIND_DATA findData;
	char newPath[MAX_PATH];
	sprintf(newPath, "%s\\*", path);
	/* printf("newPath1: %s\n", newPath); */
	HANDLE hFind = FindFirstFile(newPath, &findData);
	memset(newPath, 0, sizeof(newPath) / sizeof(newPath[0]));

	if (hFind == INVALID_HANDLE_VALUE) { printf("FindFirstDir failed (%lu)\n", GetLastError()); return; } 
	do
	{
		if (isValidDir(findData.cFileName, findData.dwFileAttributes))
		{
			/* printf("Dir: %s\n", findData.cFileName); */
			sprintf(newPath, "%s\\%s", path, findData.cFileName);
			printf("Dir: %s\n", newPath);
			findDirWin(newPath);
		}
	}
	while (FindNextFile(hFind, &findData) != 0);
}

int
isValidDir(const char *str, DWORD dwAttr)
{
	if (strcmp(str, ".") && strcmp(str, ".."))
	{
		if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
			return 1;
	}
	return 0;
}

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
    ExitProcess(dw); 
}

#endif

/*****************************************************************************/
/*LINUX*/
/*****************************************************************************/
#ifdef __linux__

char ** 
GetFilesDirIter(const char *pBasePath) 
{
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
			if (isValidDir(pdEntry->d_name, pdEntry->d_type, ".git"))
			{
				j++;
				memset(pFullPath, 0, sizeof(pFullPath) / sizeof(pFullPath[0]));
				sprintf(pFullPath, "%s/%s", pCurrentPath, pdEntry->d_name);
				if (j < STACK_SIZE) 
					strncpy(ppDir[j], pFullPath, MAX_PATH);
				else
				{
					fprintf(stderr, "Stack overflow:" "too many directories to handle:" " %s\n", pFullPath);
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
}

void
GetFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb)
{
	DIR *pdStream;
	struct dirent *pdEntry;
	char pSearchPath[1024];

	sprintf(pSearchPath, "%s/.", pPath);
	sprintf(pError, "Couldn't open '%s'", pSearchPath);
	if ((pdStream = opendir(pSearchPath)) == NULL) 
	{ 
		perror(pError); 
		return ; 
	}
	strncpy(pFiles[*pNb].pDirName, pPath, MAX_PATH);
	while ((pdEntry = readdir(pdStream)) != NULL)
	{
		if (pdEntry->d_type == DT_REG)
		{
			if (WildcardMatch(pdEntry->d_name, pRegex))
			{
				strncpy(pFiles[*pNb].pFileName, pdEntry->d_name, MAX_PATH);
				sprintf(pFiles[*pNb].pFullPath, "%s/%s", pPath, pdEntry->d_name);
				(*pNb)++;
			}
		}
	}
	closedir(pdStream);
}

int
GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb, const char *pBuildFolder)
{
	DIR *pdStream;
	struct dirent *pdEntry;
	char pSearchPath[1024];

	sprintf(pSearchPath, "%s/.", pPath);
	sprintf(pError, "Couldn't open '%s'", pSearchPath);
	if ((pdStream = opendir(pSearchPath)) == NULL) 
	{ 
		perror(pError); 
		return 1; 
	}
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
	return 0;
}

int
isValidDirImpl(const char *pStr, int attr, ...)
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

void
ErrorExit(char *pMsg) 
{
	perror(pMsg);
	exit(1);
}
#endif
