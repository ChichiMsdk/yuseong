#ifdef WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <strsafe.h>
#elif __linux__
	#include <stdarg.h>
	#include <ftw.h>
	#include <dirent.h>
	#include <errno.h>
	#define MAX_PATH 320
	char pError[1124];
#endif // WIN32
	
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CC clang
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
	int nbElems;
	int sizeMax;
}FileList;

char *
Compile2(const char *cc, const char *flags, FileList *pList)
{
	if (pList->nbElems <= 0)
	{
		fprintf(stderr, "No files to compile found ..\n");
		return NULL;
	}
	char *pObjs;
	for (int i = 0; i < pList->nbElems; i++)
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

	for (int i = 0; i < pList->nbElems; i++)
		total += strlen(pList->pFiles[i].pObjName);

	pObjs = malloc(sizeof(char) * total + (pList->nbElems) + 1);
	for (int i = 0; i < pList->nbElems; i++)
		count += sprintf(pObjs + count, "%s ", pList->pFiles[i].pObjName);
    /*
	 * printf("Count: %zu\n", count);
	 * printf("%zu + %d = %zu\n", total, pList->nbElems, total + pList->nbElems);
     */
	pObjs[--count] = 0;
	if (!pObjs)
	{
		free(pObjs);
		return "";
	}
	return pObjs;
}

void
Compile(const char *cc, const char *flags, const char *fname, const char *outname)
{
	char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
	size_t totalCount = strlen(cc) + strlen(flags) + strlen(fname) + strlen(outname);
	snprintf(command, totalCount + 10, "%s %s -c %s -o %s", cc, flags, fname, outname);
	printf("Command: %s\n", command);
	system(command);
	free(command);
}

void
Link(const char *cc, const char *flags, const char *obj, const char *outname)
{
	char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
	size_t totalCount = strlen(cc) + strlen(flags) + strlen(obj) + strlen(outname);
	snprintf(command, totalCount + 7, "%s %s %s -o %s", cc, flags, obj, outname);
	printf("Command: %s\n", command);
	/* system(command); */
	free(command);
}

void
DestroyFileList(FileList *pFileList)
{
	for (int i = 0; i < pFileList->sizeMax; i++)
	{
		free(pFileList->pFiles[i].pDirName);
		free(pFileList->pFiles[i].pFileName);
		free(pFileList->pFiles[i].pObjName);
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
		pTmp->pFiles[i].pObjName = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].pFullPath = malloc(sizeof(char) * MAX_PATH);
		pTmp->pFiles[i].maxSizeForAll = MAX_PATH;
	}
	pTmp->sizeMax = randomNbr;

	*ppFileList = pTmp;

	return error;
}

int WildcardMatch(const char *pStr, const char *pPattern);

/* TODO: Construct absolute path instead of relative */
void GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, int *pNb, const char *pBuildFolder);

/* TODO: more robust way please, this is clunky */
char **GetFilesDirIter(const char *pBasePath);

/*
 * TODO:
 * - Add option: iterative or not
 * - Add va_args to specify multiple files manually
 */
FileList *
GetFileList(const char *path, const char *regex, const char *pBuildFolder)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
	char **ppDir = GetFilesDirIter(path); // free this somewhere
	for (int i = 0; ppDir[i][0]; i++)
	{
		GetFilesAndObj(ppDir[i], regex, pFileList->pFiles, &pFileList->nbElems, pBuildFolder);
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
		printf("Obj %d: %s\n", i, pList->pFiles[i].pObjName);
	}
}

/* TODO: Specify a "cleaning" directory */
void
MakeCleanImpl([[maybe_unused]] void *none, ...)
{
	va_list args;
	va_start(args, none);

	char *str;
	while ((str = va_arg(args, char *)) != NULL)
	{
		if (str)
		{
#ifdef WIN32
			WIN32_FIND_DATA findFileData;
			HANDLE hFind = FindFirstFile(str, &findFileData);
			if (hFind == INVALID_HANDLE_VALUE)
			{
				DWORD dw = GetLastError();
				if (dw != ERROR_FILE_NOT_FOUND)
					ErrorExit(str, dw);
				return ; 
			}
			do{
				printf("Removing %s..\n", findFileData.cFileName);
				remove(findFileData.cFileName);
			}
			while (FindNextFile(hFind, &findFileData) != 0);
#elif __linux__
#endif // WIN32
		}
	}
	va_end(args);
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
 * NOTE: Yes I malloc and no we don't care
 * I should probably check them tho..
 */

/*
 * TODO: 
 * - Add parsing C files for flags.. (like pragma lib)
 */
int
main(int argc, char **ppArgv)
{
	/* findDirWin("."); */

	char *pFolder = "."; char *pBuildFolder = "build";
	char *pName = "nomake"; char *pExtension = ".exe";
	char *pOutput = "test.exe"; char *pCompiler = "clang";
	char *pCompiler2 = "c++"; char *pIncludeDirs = "src";
	char *pLibs = "-g3 -luser32"; char *pLibPath = "";
	char *pCFlags = "-g3 -Wall -Wextra -Werror";
	char *pMacros = "PLATFORM_WINDOWS";
	char *pAsan = ""; 
	char *pObjs = NULL; char *pObjs2 = NULL; char *pFinal = NULL;

	char *str = "src/main.c";
	char *pattern = "*.c";
	char **ppDir = NULL;

	FileList *CFiles = GetFileList(pFolder, "*.c", pBuildFolder);
	FileList *CPPFiles = GetFileList(pFolder, "*.cpp", pBuildFolder);

	pObjs = Compile2(pCompiler, pCFlags, CFiles);
	pFinal = malloc(sizeof(char) * strlen(pOutput) + strlen(pBuildFolder) + 2);
	size_t count = sprintf(pFinal, "%s/%s", pBuildFolder, pOutput);
	pFinal[count--] = 0;
	Link(pCompiler, pLibs, pObjs, pFinal);
	/* MakeClean("*.rdi", "*lib", "*.pdb", "*.exe", "*.exp", "*.o"); */

	PrintFileList(CFiles);
	PrintFileList(CPPFiles);

	free(pFinal);
	if (strlen(pObjs) > 0)
		free(pObjs);
	free(pObjs2);
	DestroyFileList(CFiles);
	DestroyFileList(CPPFiles);

	return 0;
}

/*****************************************************************************/
/*WINDOWS*/

/*****************************************************************************/
/*LINUX*/
