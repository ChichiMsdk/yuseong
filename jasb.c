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
#include <stdbool.h>

#define YMB [[maybe_unused]]

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <strsafe.h>
	#include <corecrt_io.h>

	#define YO_RDONLY _O_RDONLY
	#define OPEN(a, b) _open(a, b)
	#define CLOSE(a) _close(a)
	#define READ(a, b, c) _read(a, b, c)
	#define STRDUP(str) _strdup(str)

	void ErrorExit(char* lpszMsg, unsigned long dw);

	#define SLASH "\\"
	#define EXTENSION ".exe"
	#define TRACY_PATH "-IC:\\Lib\\tracy-0.11.1\\public -IC:\\Lib\\tracy-0.11.1\\public\\tracy"

	#define GLFW_PATH "-IC:\\Lib\\glfw\\include" 
	#define GLFWLIB_PATH "-LC:\\Lib\\glfw\\lib-vc2022"
	#define GLFWLIB "-lgflw3_mt"

	#define VULKAN_PATH "-IC:/VulkanSDK/1.3.275.0/Include"
	#define VULKANLIB_PATH "-LC:/VulkanSDK/1.3.275.0/Lib"
	#define VULKANLIB "-lvulkan-1"
	#define OPENGLLIB "-lopeng32"
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
	#define STRDUP(str) strdup(str)
	void ErrorExit(char *pMsg) ;

	#define SLASH "/"
	#define EXTENSION ""

	#define TRACY_PATH

	#define GLFW_PATH
	#define GLFWLIB_PATH
	#define GLFWLIB

	#define VULKAN_PATH
	#define VULKANLIB_PATH
	#define VULKANLIB
	#define OPENGLLIB

#endif // WIN32

#define CCJSON_BEGIN "[\n"
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
	char **ppList;
	sFile *pFiles;
	size_t nbElems;
	size_t sizeMax;
}FileList;

typedef struct Command
{
	char *pNAME;
	char *pROOTFOLDER;
	char *pEXTENSION;
	char *pBUILD_DIR;
	char *pOBJ_DIR;
	char *pCC;
	char *pCPP;
	char *pSRC_DIR;
	char *pINCLUDE_DIRS;
	char *pLIB_PATH;
	char *pLIBS;
	char *pCFLAGS;
	char *pDEFINES;
	char *pCPPFLAGS;
}Command;


typedef struct MemChef
{
	void **ppTable;
	size_t nbElems;
	size_t maxSize;
}MemChef;

MemChef gChef = {0};

char *
ChefStrDup(char *pStr)
{
	if (gChef.nbElems >= gChef.maxSize)
	{
		gChef.ppTable = realloc(gChef.ppTable, gChef.maxSize * 2);
		gChef.maxSize *= 2;
	}
	char *pDup = STRDUP(pStr);
	gChef.ppTable[gChef.nbElems] = pDup;
	gChef.nbElems++;
	return pDup;
}

#define CHEF_ALLOC 100

void
ChefInit(void)
{
	gChef.ppTable = malloc(sizeof(void*) * CHEF_ALLOC);
	gChef.nbElems = 0;
	gChef.maxSize = CHEF_ALLOC;
}

void
ChefDestroy(void)
{
	for (size_t i = 0; i < gChef.nbElems; i++)
	{
		printf("Freeing: \"%s\" at %p\n", (char*)gChef.ppTable[i], gChef.ppTable[i]);
		free(gChef.ppTable[i]);
	}
}

void
ChefFree(void *pPtr)
{
	for (size_t i = 0; i < gChef.nbElems; i++)
	{
		if (gChef.ppTable[i] == pPtr)
		{
			free(gChef.ppTable[i]);
			gChef.ppTable[i] = NULL;
		}
	}
	free(pPtr);
}

void *
ChefRealloc(void *pPtr, size_t size)
{
	size_t i = 0;
	for (i = 0; i < gChef.nbElems; i++)
	{
		if (gChef.ppTable[i] == pPtr)
		{
			gChef.ppTable[i] = realloc(gChef.ppTable[i], size);
			break;
		}
	}
	return gChef.ppTable[i];
}

char *
ChefStrAppendSpaceImpl(char *pDst, ...)
{
#define SPACELEN 1

	va_list args;
	va_start(args, pDst);
	char *pArg = NULL;
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		size_t dstSize = strlen(pDst);
		size_t srcSize = strlen(pArg);
		pDst = ChefRealloc(pDst, dstSize + srcSize + 1 + SPACELEN);
		memcpy(&pDst[dstSize], pArg, srcSize);
		pDst[dstSize + srcSize] = ' ';
		pDst[dstSize + srcSize + 1] = 0;
	}
	va_end(args);
	return pDst;
}

char *
ChefStrAppendImpl(char *pDst, ...)
{
	va_list args;
	va_start(args, pDst);
	char *pArg = NULL;
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		size_t dstSize = strlen(pDst);
		size_t srcSize = strlen(pArg);
		pDst = ChefRealloc(pDst, dstSize + srcSize + 1);
		memcpy(&pDst[dstSize], pArg, srcSize);
		pDst[dstSize + srcSize] = 0;
	}
	va_end(args);
	return pDst;
}

#define STR(a) ChefStrDup(a)
#define APPEND(a, ...) a = ChefStrAppendImpl(a, __VA_ARGS__, NULL)
#define APPEND_SPACE(a, ...) a = ChefStrAppendSpaceImpl(a, __VA_ARGS__, NULL)

void MakeCleanImpl(void *none, ...);
int IsValidDirImpl(const char *pStr, unsigned long attr, ...);

#define IsValidDir(...) IsValidDirImpl(__VA_ARGS__, NULL)
#define MakeClean(...) MakeCleanImpl(__VA_ARGS__, NULL)

int InitFileList(FileList **ppFileList);
void DestroyFileList(FileList *pFileList);
int WildcardMatch(const char *pStr, const char *pPattern);

/* TODO: Construct absolute path instead of relative */
void FindFiles(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb);

/* TODO: more robust way please, this is clunky */
char **GetFilesDirIter(const char *pBasePath);
int GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb, const char *pBuildFolder);
FileList *GetFileList(const char* pPath, const char* pRegex);
FileList *GetFileListAndObjs(Command* pCmd, const char* pRegex);

#define JSONREGEX "*.o.json"
void PrintFileList(FileList *pList);
int FlushIt(char *pLine, FILE* pFd, size_t size);
int ConstructCompileCommandsJson(FileList *pList, const char *pName);
int ClangCompileCommandsJson(const char *pCompileCommands);

char *
Compile(Command* pCmd, FileList* pList)
{
	if (pList->nbElems <= 0) { fprintf(stderr, "No files to compile found ..\n"); return NULL; }

	char *OUTFLAG;
	char *MODEFLAG;
	char *pObjs;

	if ((strcmp("clang", pCmd->pCC) == 0) || (strcmp("gcc", pCmd->pCC) == 0))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
	}
	Command a = *pCmd;
	for (size_t i = 0; i < pList->nbElems; i++)
	{
		char *fname = pList->pFiles[i].pFullPath;
		char *outname = pList->pFiles[i].pObjName;
		char *pCmd = STR("");
		APPEND_SPACE(pCmd, a.pCC, a.pCFLAGS, a.pDEFINES, MODEFLAG, fname, OUTFLAG, outname);
        /*
		 * char *command = malloc(sizeof(char) * MAX_SIZE_COMMAND);
		 * size_t totalCount = strlen(cc) + strlen(flags) + strlen(fname) + strlen(outname);
		 * snprintf(command, totalCount + 10, "%s %s -c %s -o %s", cc, flags, fname, outname);
         */
		printf("Command: %s\n", pCmd);
		/* system(command); */
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
Link(Command* pCmd, const char *pObj, const char *pOutName)
{
	int error = 0;
	/* char *pCommand = malloc(sizeof(char) * MAX_SIZE_COMMAND); */
	char *pCommand = STR("");
	Command a = *pCmd;

	char *OUTFLAG = NULL;
	char *MODEFLAG = NULL;

	if ((strcmp("clang", pCmd->pCC) == 0) || (strcmp("gcc", pCmd->pCC) == 0))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
	}
	APPEND_SPACE(pCommand, a.pCC, a.pCFLAGS, a.pDEFINES, pObj, OUTFLAG, pOutName);

    /*
	 * size_t totalCount = strlen(pCc) + strlen(pFlags) + strlen(pObj) + strlen(pOutName);
	 * snprintf(pCommand, totalCount + 7, "%s %s %s -o %s", pCc, pFlags, pObj, pOutName);
     */

	printf("Command: %s\n", pCommand);
	/* error = system(pCommand); */

	return error;
}

char *
GetFinalOutputCommand(const char* pOutput, const char* pBuildFolder)
{
	char *pFinalOutputCommand = NULL;
	pFinalOutputCommand = malloc(sizeof(char) * strlen(pOutput) + strlen(pBuildFolder) + 2);
	size_t count = sprintf(pFinalOutputCommand, "%s%s%s", pBuildFolder, SLASH, pOutput);
	pFinalOutputCommand[count--] = 0;
	return pFinalOutputCommand;
}

int
Build(Command* pCmd, FileList* pListCfiles)
{
	char *pFinalOutputCommand = NULL;
	char *pOutput = STR("");
	APPEND(pOutput, pCmd->pBUILD_DIR, SLASH, pCmd->pNAME, pCmd->pEXTENSION);
	char *pOutObjs = NULL;

	pOutObjs = Compile(pCmd, pListCfiles);
	/* pFinalOutputCommand = GetFinalOutputCommand(pOutput, pBuildFolder); */

	int error = Link(pCmd, pOutObjs, pOutput);
	/* free(pFinalOutputCommand); */
	if (strlen(pOutObjs) > 0)
		free(pOutObjs);
	return error;
}
/*
 * TODO: 
 * - Multithreading -> CrossPlatform = boring for this lots of lines
 * - Incremental builds -> Most of the time useless / problematic
 */

bool gbVulkan = FALSE;
bool gbOpenGL = FALSE;
bool gbD3D11 = FALSE;
bool gbD3D12 = FALSE;
bool gbGLFW3 = FALSE;
bool gbTracy = FALSE;
bool gbRelease = FALSE;
bool gbDebug = TRUE;
bool gbTest = FALSE;
bool gbAsan = FALSE;

int
main(int argc, char **ppArgv)
{
#if 0
	char test[1000000];
	printf("Wrote: %d\n", sprintf(test, "TEST:|%s%s%s%s|", "", "SALUT", "", "YO"));
	printf("%s", test);
	return 1;
#endif

	if (argc == 2)
	{
		if (strcmp(ppArgv[1], "-h"))
			fprintf(stderr, "Usage: jasb\n");
		return 1;
	}
	gbVulkan = FALSE; gbOpenGL = FALSE; gbD3D11 = FALSE; gbD3D12 = FALSE; gbGLFW3 = FALSE;
	gbTracy = FALSE;
	gbRelease = FALSE; gbDebug = TRUE; gbTest = FALSE;
	gbAsan = FALSE;

	ChefInit();
	Command cmd = {
		.pNAME = STR("yuseong"),
		.pROOTFOLDER = STR("."),
		.pEXTENSION = STR(EXTENSION),
		.pBUILD_DIR = STR("build"),
		.pOBJ_DIR = STR("build" SLASH "obj"),
		.pCC = STR("clang"),
		.pCPP = STR("clang++"),
		.pSRC_DIR = STR("src"),
		.pINCLUDE_DIRS = STR("-Isrc -Isrc/core"),
		.pCFLAGS = STR("-Wall -Wextra -Werror"),
		.pLIB_PATH = STR(""),
        /*
		 * pNAME;
		 * pROOTFOLDER;
		 * pEXTENSION;
		 * pBUILD_DIR;
		 * pOBJ_DIR;
		 * pCC;
		 * pCPP;
		 * pSRC_DIR;
		 * pINCLUDE_DIRS;
		 * pLIB_PATH;
		 * pLIBS;
		 * pCFLAGS;
		 * pCPPFLAGS;
		 * pDEFINES;
         */
	};

#ifdef _WIN32 
	cmd.pDEFINES = STR("-DPLATFORM_WINDOWS");
	cmd.pLIBS = STR("-lshell32 -lgdi32 -lwinmm -luser32");
	APPEND(cmd.pCFLAGS, " -g3");
#elif __linux__
	cmd.pDEFINES = STR("-DPLATFORM_LINUX");
	cmd.pLIBS = STR("");
	APPEND(cmd.pCFLAGS, " -ggdb3");
#else
	cmd.pDEFINES = STR("");
	cmd.pLIBS = STR("");
#endif

	cmd.pCPPFLAGS = STR("");
	APPEND(cmd.pCFLAGS, " -fno-inline", " -fno-omit-frame-pointer");
	APPEND(cmd.pCFLAGS, " -Wno-missing-field-initializers", " -Wno-unused-but-set-variable");
	APPEND(cmd.pCFLAGS, " -Wno-uninitialized");
	APPEND(cmd.pCFLAGS, " -Wvarargs");
	if (gbDebug)
	{
		APPEND(cmd.pDEFINES, " -D_DEBUG", " -DDEBUG", " -DYUDEBUG");
		APPEND(cmd.pCFLAGS, " -O0");
	}
	else if (gbRelease)
	{
		APPEND(cmd.pDEFINES, " -D_RELEASE", " -DRELEASE", " -DYURELEASE");
		APPEND(cmd.pCFLAGS, " -O3");
	}

	if (gbTest) APPEND(cmd.pDEFINES, " ", "-DTESTING");

	if (gbVulkan)
	{
		APPEND(cmd.pLIB_PATH, " ", VULKANLIB_PATH);
		APPEND(cmd.pINCLUDE_DIRS, " ", VULKANLIB_PATH);
		APPEND(cmd.pLIBS, " ", VULKANLIB);
	}
	else if (gbOpenGL)
	{
		APPEND(cmd.pLIBS, " ", OPENGLLIB);
	}
#ifdef _WIN32
	else if (gbD3D11)
	{
		APPEND(cmd.pLIBS, " ", "-ld3dcompiler -ld3d11 -ldxgi -ldxguid");
	}
	else if (gbD3D12)
	{

	}
#endif
	if (gbGLFW3)
	{
		APPEND(cmd.pINCLUDE_DIRS, " ", GLFW_PATH);
		APPEND(cmd.pLIB_PATH, " ", GLFWLIB_PATH);
		APPEND(cmd.pLIBS, " ", GLFWLIB);
	}
	if (gbTracy)
	{
		APPEND(cmd.pINCLUDE_DIRS, " ", TRACY_PATH);
		APPEND(cmd.pDEFINES, " ", "-DTRACY_ENABLE");
		/* CPPFLAGS =-Wno-format */
		/* Also, compile cpp flags with different struct ? */
	}
	if (gbAsan)
	{
		APPEND(cmd.pCFLAGS, " ", "-fsanitize=address");
	}

	APPEND(cmd.pINCLUDE_DIRS, " -Isrc/renderer/opengl");
	char *pCompileCommands = STR("compile_commands.json");

	FileList *pListCfiles = {0};

	pListCfiles = GetFileListAndObjs(&cmd, "*.c");
	if (!pListCfiles) { fprintf(stderr, "Something happened in c\n"); return 1; }

	FileList *pListCppFiles = {0};

	pListCppFiles = GetFileListAndObjs(&cmd, "*.cpp");
	if (!pListCppFiles) { fprintf(stderr, "Something happened in cpp\n"); return 1; }

	if (Build(&cmd, pListCfiles) == 1)
	{ fprintf(stderr, "Something happened in cpp\n"); return 1; }

    /*
	 * if (ClangCompileCommandsJson(pCompileCommands))
	 * 	goto exiting;
     */

	/* DestroyFileList(pListJson); */
	ChefDestroy();
	printf("number = %zu\n", gChef.nbElems);
	return 0;

exiting:
	fprintf(stderr, "Error");
	/* perror(pListJson->pFiles->pFullPath); */
	/* DestroyFileList(pListJson); */

	printf("number = %zu\n", gChef.nbElems);
	ChefDestroy();
	return 1;
}

int
ClangCompileCommandsJson(const char *pCompileCommands)
{
	FileList *pListJson = GetFileList(".", JSONREGEX);
	PrintFileList(pListJson);
	if (!pListJson)
		return 1;
	return ConstructCompileCommandsJson(pListJson, pCompileCommands);
}

/*
 * TODO:
 * - Add option: iterative or not
 * - Add va_args to specify multiple files manually
 */
FileList *
GetFileList(const char* pPath, const char* pRegex)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
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

FileList *
GetFileListAndObjs(Command* pCmd, const char* pRegex)
{
	FileList *pFileList = {0};
	if ((InitFileList(&pFileList) != 0))
		return NULL;
	char **ppDir = GetFilesDirIter(pCmd->pROOTFOLDER); // free this somewhere
	for (int i = 0; ppDir[i][0]; i++)
	{
		GetFilesAndObj(ppDir[i], pRegex, pFileList->pFiles, &pFileList->nbElems, pCmd->pBUILD_DIR);
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
			if (IsValidDir(fileInfo.cFileName, fileInfo.dwFileAttributes) == TRUE)
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
GetFilesAndObj(const char *pPath, const char *pRegex, sFile *pFiles, size_t *pNb, const char *pBuildFolder)
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
		return 1; 
	}
	strncpy(pFiles[*pNb].pDirName, pPath, MAX_PATH);
	do
	{
		int lenName = strlen(findFileData.cFileName);
		sprintf(pFiles[*pNb].pObjName, "%s%s", pBuildFolder, SLASH);
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
		sprintf(pFiles[*pNb].pFullPath, "%s%s%s", pPath, SLASH, findFileData.cFileName);
		(*pNb)++;
	}
	while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
	return 0;
}

void
findDirWin([[maybe_unused]] const char *path)
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
			return FALSE;
		}
	}

	if (strcmp(pStr, ".") && strcmp(pStr, ".."))
	{
		if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
			return TRUE;
	}
	return FALSE;
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
			if (isValidDir(pdEntry->d_name, pdEntry->d_type, ".git") == TRUE)
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
isValidDirImpl(const char *pStr, unsigned long attr, ...)
{
	va_list args;
	va_start(args, attr);
	char *pArgStr = NULL;
	while ((pArgStr = va_arg(args, char *)) != NULL)
	{
		if (!strcmp(pArgStr, pStr))
		{
			va_end(args);
			return FALSE;
		}
	}
	va_end(args);
	if (strcmp(pStr, ".") && strcmp(pStr, ".."))
	{
		if ((unsigned char)attr == DT_DIR)
			return TRUE;
	}
	return FALSE;
}

void
ErrorExit(char *pMsg) 
{
	perror(pMsg);
	exit(1);
}
#endif
