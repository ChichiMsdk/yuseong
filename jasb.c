/******************************************************************************
 * 						JASB. Just A Simple Builder.                          *
 ******************************************************************************
 * JASB will look for all the ".o.json" files and put them in one             *
 * compile_commands.json at the root directory. It is intended to be used with*
 * a Makefile that will compile it and then use after each build. As sed could*
 * differ from os and shell's this one should actually be portable.           *
 ******************************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define YMB [[maybe_unused]]

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>
#include <corecrt_io.h>
#include <shlwapi.h>

void ErrorExit(char* lpszMsg, unsigned long dw);
bool MkdirImpl(char* pStr);

#pragma comment(lib, "shlwapi.lib")
#define YO_RDONLY _O_RDONLY
#define OPEN(a, b) _open(a, b)
#define CLOSE(a) _close(a)
#define READ(a, b, c) _read(a, b, c)
#define STRDUP(str) _strdup(str)
#define MKDIR(str) MkdirImpl(str)
#define ISDIRECTORY(str) PathIsDirectory(str)
#define ERROR_EXIT(str) ErrorExit(str, GetLastError())

#define SLASH "\\"

void PerrorLog(char *pMsg, char *pFile, int line);
#define PERROR_LOG(str) PerrorLog(str, __FILE__, __LINE__)
bool DoesExist(const char *pPath);

#elif __linux__
#include <stdarg.h>
#include <ftw.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_PATH 320
char pError[1124];


void ErrorExit(char *pMsg);
bool PathIsDirectory(const char *pPath);

void PerrorLog(char *pMsg, const char *pFile, int line);
#define PERROR_LOG(str) PerrorLog(str, __FILE__, __LINE__)

#define YO_RDONLY O_RDONLY
#define OPEN(a, b) open(a, b)
#define CLOSE(a) close(a)
#define READ(a, b, c) read(a, b, c)
#define STRDUP(str) strdup(str)
#define MKDIR(str) MkdirImpl(str)
#define ISDIRECTORY(str) PathIsDirectory(str)
#define ERROR_EXIT(str) ErrorExit(str)

#define SLASH "/"

#endif // WIN32

#define JASBAPPEND 1
#define JASBPREPEND 2
#define CCJSON_BEGIN "["
#define CCJSON_END "]"
#define STEPS 3000
#define START_ALLOC 10240
#define MAX_SIZE_COMMAND 10000
#define STACK_SIZE 1024

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
    sFile *pFiles;
    size_t nbElems;
    size_t sizeMax;
}FileList;

void MakeCleanImpl(void *none, ...);
int IsValidDirImpl(const char *pStr, unsigned long attr, ...);

#define IsValidDir(...) IsValidDirImpl(__VA_ARGS__, NULL)
#define MakeClean(...) MakeCleanImpl(__VA_ARGS__, NULL)

FileList* InitFileList(void);
void DestroyFileList(FileList *pFileList);
int WildcardMatch(const char *pStr, const char *pPattern);

/* TODO: Construct absolute path instead of relative */
void FindFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb);

/* TODO: more robust way please, this is clunky */
char **GetFilesDirIter(const char *pBasePath);
FileList *GetFileList(const char* pPath, const char* pRegex);

#define JSONREGEX "*.o.json"
void PrintFileList(FileList *pList);
void AddToBuffer(char *pLine, char* buffer, int* count);
int ConstructCompileCommandsJson(FileList *pList, const char *pName);
int ClangCompileCommandsJson(const char *pCompileCommands);


bool
StrIsEqual(const char* s1, const char* s2)
{
    if (strcmp(s1, s2) == 0)
	return true;
    else
	return false;
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
/*
 * TODO:
 * - Add option: iterative or not
 * - Add va_args to specify multiple files manually
 */


FileList *
GetFileList(const char* pPath, const char* pRegex)
{
    FileList *pFileList = InitFileList();

    char **ppDir = GetFilesDirIter(pPath);
    for (int i = 0; ppDir[i][0]; i++)
    {
	FindFiles(ppDir[i], pRegex, pFileList->pFiles, &pFileList->nbElems);
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

FILE*
c_Fopen(const char *path, char* option)
{
    FILE* pFile = fopen(path, option);
    if (!pFile)
    {
	perror(path);
	pFile = stderr;
    }
    return pFile;
}

int
c_Fclose(FILE* stream)
{
    if (stream == stderr)
	return 0;
    return fclose(stream);
}

void
AddToBuffer(char *pLine, char* buffer, int* count)
{
    buffer += *count;
    int outputCount = sprintf(buffer, "\n%s", pLine);
    memset(pLine, 0, outputCount);

    /* NOTE: Jumps the just added '\n' */
    char *temp = buffer + 1;
    for (int i = 0; i < outputCount; i++)
    {
	if (temp[i] == '\r' || temp[i] == '\n')
	{
	    temp[i] = 0;
	    outputCount--;
	}
    }
    *count += outputCount;
}

int
ConstructCompileCommandsJson(FileList *pList, const char *pName)
{
    FILE *pFd1 = c_Fopen(pName, "w");

    bool error = true;
    int bufferCount = 0;
    int charRead = 0;

    char pLine[START_ALLOC];
    char *buffer = calloc(START_ALLOC * (pList->nbElems + 3), sizeof(char));

    bufferCount += sprintf(buffer, "%s", CCJSON_BEGIN);
    for (size_t i = 0; i < pList->nbElems; i++)
    {
	int fd = OPEN(pList->pFiles[i].pFullPath, YO_RDONLY);
	if (!fd) { error = false; goto exiting;} 
	do
	{
	    charRead = READ(fd, pLine, STEPS);
	} while(charRead > 0);
	AddToBuffer(pLine, buffer, &bufferCount);
	CLOSE(fd);
    }

    /* NOTE: Removes the last ',' */
    sprintf(buffer + bufferCount - 1, "\n%s", CCJSON_END);
    fprintf(pFd1, "%s", buffer);

    free(buffer);
exiting:
    c_Fclose(pFd1);
    return error;
}

FileList *gpListJson;

int
ClangCompileCommandsJson(const char *pCompileCommands)
{
    gpListJson = GetFileList(".", JSONREGEX);
    if (!gpListJson) return false;
    return ConstructCompileCommandsJson(gpListJson, pCompileCommands);
}

static sFile stubFile = {
    .pFileName = "stub.c",
    .pObjName = "stub.o",
    .pDirName = "C:/",
    .maxSizeForAll = MAX_PATH,
    .pFullPath = "C:/stub",
};

static FileList stubList = {
    .pFiles = &stubFile,
    .nbElems = 1,
    .sizeMax = 1,
};

FileList*
InitFileList(void)
{
    int randomNbr = 100;

    FileList *pTmp = calloc(1, sizeof(FileList));
    if (!pTmp)
	return &stubList;

    pTmp->pFiles = malloc(sizeof(sFile) * randomNbr);
    if (!pTmp->pFiles)
	return free(pTmp), &stubList;

    /* TODO: Better design around the allocation's failure ! */
    for (int i = 0; i < randomNbr; i++)
    {
	pTmp->pFiles[i].pDirName = malloc(sizeof(char) * MAX_PATH);
	pTmp->pFiles[i].pFileName = malloc(sizeof(char) * MAX_PATH);
	pTmp->pFiles[i].pObjName = malloc(sizeof(char) * MAX_PATH);
	pTmp->pFiles[i].pFullPath = malloc(sizeof(char) * MAX_PATH);
	pTmp->pFiles[i].maxSizeForAll = MAX_PATH;
    }
    pTmp->sizeMax = randomNbr;
    return pTmp;
}

void
DestroyFileList(FileList *pFileList)
{
    /* TODO: Better design around the allocation's failure ! */
    if (StrIsEqual(pFileList->pFiles[0].pFullPath, "C:/stub"))
	    return ;
    for (size_t i = 0; i < pFileList->sizeMax; i++)
    {
	free(pFileList->pFiles[i].pDirName);
	free(pFileList->pFiles[i].pFileName);
	free(pFileList->pFiles[i].pFullPath);
	free(pFileList->pFiles[i].pObjName);
    }
    free(pFileList->pFiles);
    free(pFileList);
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

    strncpy(ppDir[index], pBasePath, MAX_PATH);
    char fullPath[1024];
    WIN32_FIND_DATA fileInfo;
    int j = 0;
    do
    {
	char *currentPath = ppDir[index];
	char searchPath[1024];

	sprintf(searchPath, "%s%s*", currentPath, SLASH);
	HANDLE handle = FindFirstFile(searchPath, &fileInfo);
	if (handle == INVALID_HANDLE_VALUE)
	    fprintf(stderr, "Unable to open directory: Error(%lu)\n", GetLastError());
	do
	{
	    if (IsValidDir(fileInfo.cFileName, fileInfo.dwFileAttributes) == true)
	    {
		j++;
		memset(fullPath, 0, sizeof(fullPath) / sizeof(fullPath[0]));
		sprintf(fullPath, "%s%s%s", currentPath, SLASH, fileInfo.cFileName);

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
FindFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb)
{
    WIN32_FIND_DATA findFileData;
    char pSearchPath[MAX_PATH];
    sprintf(pSearchPath, "%s%s%s", pPath, SLASH, pRegex);
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
	sprintf(pFiles[*pNb].pFullPath, "%s%s%s", pPath, SLASH, findFileData.cFileName);
	(*pNb)++;
    }
    while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

int
IsValidDirImpl(const char *pStr, unsigned long dwAttr, ...)
{
    va_list args;
    va_start(args, dwAttr);
    char *pArgStr = NULL;
    while ((pArgStr = va_arg(args, char *)) != NULL)
    {
	if (!strcmp(pArgStr, pStr))
	{
	    va_end(args);
	    return false;
	}
    }

    /* NOTE: Excluded directories */
    if (!StrIsEqual(pStr, ".") && !StrIsEqual(pStr, ".."))
    {
	if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
	    return true;
    }
    return false;
}

bool
MkdirImpl(char *pStr)
{
    return CreateDirectory(pStr, NULL);
}

void
findDirWin(const char *path)
{
    WIN32_FIND_DATA findData;
    char newPath[MAX_PATH];
    sprintf(newPath, "%s%s*", path, SLASH);
    HANDLE hFind = FindFirstFile(newPath, &findData);
    memset(newPath, 0, sizeof(newPath) / sizeof(newPath[0]));

    if (hFind == INVALID_HANDLE_VALUE) { printf("FindFirstDir failed (%lu)\n", GetLastError()); return; } 
    do
    {
	if (IsValidDir(findData.cFileName, findData.dwFileAttributes))
	{
	    /* printf("Dir: %s\n", findData.cFileName); */
	    sprintf(newPath, "%s%s%s", path, SLASH, findData.cFileName);
	    printf("Dir: %s\n", newPath);
	    findDirWin(newPath);
	}
    }
    while (FindNextFile(hFind, &findData) != 0);
}

void
ErrorExit(char* lpszFunction, unsigned long dw) 
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
	    0, NULL);

    lpDisplayBuf = (LPVOID)LocalAlloc(
	    LMEM_ZEROINIT, 
	    (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 

    StringCchPrintf(
	    (LPTSTR)lpDisplayBuf,
	    LocalSize(lpDisplayBuf) / sizeof(TCHAR),
	    TEXT("%s failed with error %d: %s"),
	    lpszFunction, dw, lpMsgBuf); 

    fprintf(stderr, "Error: %s\n", (LPCTSTR)lpDisplayBuf);

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
	    if (IsValidDir(pdEntry->d_name, pdEntry->d_type, ".git") == true)
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
FindFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb)
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
IsValidDirImpl(const char *pStr, unsigned long attr, ...)
{
    va_list args;
    va_start(args, attr);
    char *pArgStr = NULL;
    while ((pArgStr = va_arg(args, char *)) != NULL)
    {
	if (!strcmp(pArgStr, pStr))
	{
	    va_end(args);
	    return false;
	}
    }
    va_end(args);
    if (strcmp(pStr, ".") && strcmp(pStr, ".."))
    {
	if ((unsigned char)attr == DT_DIR)
	    return true;
    }
    return false;
}

bool
PathIsDirectory(const char *pPath)
{
    struct stat sb;
    int result = stat(pPath, &sb);
    if (result < 0)
    {
	PERROR_LOG("PathIsDirectoy()");
	return false;
    }
    if (S_ISDIR(sb.st_mode))
	return true;
    return false;
}

bool
MkdirImpl(char *pStr)
{
    if (mkdir(pStr, 0700) == 0)
	return true;
    return false;
}

void
ErrorExit(char *pMsg) 
{
    perror(pMsg);
    exit(1);
}
void
PerrorLog(char* pStr, const char* pFile, int line)
{
    char buff[1024];
    sprintf(buff, "%s in %s:%d", pStr, pFile, line);
}
#endif
/*
 * TODO: 
 * - Multithreading -> CrossPlatform = boring for this lots of lines
 * - Incremental builds -> Most of the time useless / problematic
 */
int
main(YMB int argc, YMB char **ppArgv)
{
    const char* pCompileCommands = "compile_commands.json";
    if (ClangCompileCommandsJson(pCompileCommands) == false)
	fprintf(stderr, "Error");

    DestroyFileList(gpListJson);
    return 0;
}
