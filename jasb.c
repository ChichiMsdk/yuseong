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

typedef enum yError {
	Y_NO_ERROR = 0x00,

	Y_ERROR_EMPTY = 0x01,
	Y_ERROR_UNKNOWN = 0x02,
	Y_ERROR_EXEC = 0x03,
	Y_ERROR_BUILD = 0x04,
	Y_ERROR_LINK = 0x05,
	Y_ERROR_JSON = 0x06,
	Y_MAX_ERROR
}yError;

char *pErrorMsg[] = {
	"No error.",
	"List is empty.",
	"Unknown error.",
	"Execution error.",
	"Build couldn't finish.",
	"Link couldn't finish.",
	"Json couldn't not be created.",
};

#define GetErrorMsg(a) pErrorMsg[a]

char *gpPrintHelper[] = {
	"{ ",
	"\"directory\": ",
	"\"file\": ",
	"\"output\": ",
	"\"arguments\": [",
	"]}",
};

#define Y_OPENCURLY gpPrintHelper[0]
#define Y_DIRECTORY gpPrintHelper[1]
#define Y_FILE gpPrintHelper[2]
#define Y_OUTPUT gpPrintHelper[3]
#define Y_BEGIN_ARGUMENTS gpPrintHelper[4]
#define Y_CLOSE_ARGUMENTS gpPrintHelper[5]
#define YCOMA ","

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define EXECUTE(x, y) ExecuteImpl(x, y)

int
ExecuteImpl(const char* pCommand, int dry)
{
	int code = 0;
	if (dry == 1)
		printf("%s\n", pCommand);
	else
		code = system(pCommand);
	return code;
}

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
	#define EXTENSION ".exe"
	#define TRACY_PATH "C:\\Lib\\tracy-0.11.1\\public"
	#define TRACYTRACY_PATH "C:\\Lib\\tracy-0.11.1\\public\\tracy"

	#define GLFW_PATH "C:\\Lib\\glfw\\include" 
	#define GLFWLIB_PATH "C:\\Lib\\glfw\\lib-vc2022"
	#define GLFWLIB "gflw3_mt"

	#define VULKAN_PATH "C:/VulkanSDK/1.3.275.0/Include"
	#define VULKANLIB_PATH "C:/VulkanSDK/1.3.275.0/Lib"
	#define VULKANLIB "vulkan-1"
	#define OPENGLLIB "opengl32"

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

	#define YO_RDONLY O_RDONLY
	#define OPEN(a, b) open(a, b)
	#define CLOSE(a) close(a)
	#define READ(a, b, c) read(a, b, c)
	#define STRDUP(str) strdup(str)
	#define MKDIR(str) MkdirImpl(str)
	#define ISDIRECTORY(str) PathIsDirectory(str)
	#define ERROR_EXIT(str) ErrorExit(str)

	#define SLASH "/"
	#define EXTENSION ""

	#define TRACY_PATH "/home/chichi/tracy/public"
	#define TRACYTRACY_PATH "/home/chichi/tracy/public/tracy"

	#define GLFW_PATH ""
	#define GLFWLIB_PATH ""
	#define GLFWLIB "glfw"

	#define VULKAN_PATH ""
	#define VULKANLIB_PATH ""
	#define VULKANLIB "vulkan"
	#define OPENGLLIB "GL"

#endif // WIN32

#define JASBAPPEND 1
#define JASBPREPEND 2
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

/*****************************************************************************/
/*							string memory management						 */
/*****************************************************************************/

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
		/* printf("Freeing: \"%s\" at %p\n", (char*)gChef.ppTable[i], gChef.ppTable[i]); */
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

#define SPACELEN 1
char *
ChefStrAppendSpaceImpl(char *pDst, ...)
{
	va_list args;
	va_start(args, pDst);
	char *pArg = NULL;
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		if (strlen(pArg) <= 0)
			continue;
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

char *
ChefStrAppendWithFlagsImpl(char *pDst, char *pFlag, ...)
{
	if (!pFlag)
		return NULL;
	va_list args;
	char *pArg = NULL;
	va_start(args, pFlag);
	size_t flagSize = strlen(pFlag);
	/* size_t dstSize = strlen(pDst); */
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		if (strlen(pArg) <= 0)
			continue;
		size_t srcSize = strlen(pArg);
		size_t dstSize = strlen(pDst);
		pDst = ChefRealloc(pDst, dstSize + srcSize + 1 + flagSize + SPACELEN);
		memcpy(&pDst[dstSize], pFlag, flagSize);
		memcpy(&pDst[dstSize + flagSize], pArg, srcSize);
		pDst[dstSize + srcSize + flagSize] = ' ';
		pDst[dstSize + srcSize + flagSize + 1] = 0;
	}
	va_end(args);
	return pDst;
}

char *
ChefStrPrependWithFlagsImpl(char *pDst, char *pFlag, ...)
{
	if (!pFlag)
		return NULL;
	va_list args;
	char *pArg = NULL;
	va_start(args, pFlag);
	size_t flagSize = strlen(pFlag);
	while ((pArg = va_arg(args, char *)) != NULL)
	{
		if (strlen(pArg) <= 0)
			continue;
		size_t srcSize = strlen(pArg);
		size_t dstSize = strlen(pDst);
		char *tmp = malloc(dstSize + 1);

		pDst = ChefRealloc(pDst, dstSize + srcSize + 1 + flagSize + SPACELEN);

		memcpy(&pDst[dstSize], pFlag, flagSize);
		memcpy(&pDst[dstSize + flagSize], pArg, srcSize);

		pDst[dstSize + srcSize + flagSize] = ' ';
		pDst[dstSize + srcSize + flagSize + 1] = 0;

		free(tmp);
	}
	va_end(args);
	return pDst;
}

char *
ChefStrSurroundImpl(char *pDst, char *pSurround)
{
	size_t dstSize = strlen(pDst);
	size_t surroundSize = strlen(pSurround);
	char *tmp = ChefStrDup(pDst);
	tmp = ChefRealloc(tmp, dstSize + (surroundSize * 2) + 1);
	memcpy(tmp, pSurround, surroundSize);
	memcpy(&tmp[surroundSize], pDst, dstSize);
	memcpy(&tmp[surroundSize + dstSize], pSurround, surroundSize);
	tmp[(surroundSize * 2) + dstSize] = 0;
	return tmp;
}

#define STR(a) ChefStrDup(a)
#define SELF_APPEND(a, ...) a = ChefStrAppendImpl(a, __VA_ARGS__, NULL)
#define SELF_APPEND_SPACE(a, ...) a = ChefStrAppendSpaceImpl(a, __VA_ARGS__, NULL)

#define APPEND(a, ...) ChefStrAppendImpl(a, __VA_ARGS__, NULL)
#define APPEND_SPACE(a, ...) ChefStrAppendSpaceImpl(a, __VA_ARGS__, NULL)

#define PUTINQUOTE(a) ChefStrSurroundImpl(a, "\"")

#define SELF_PREPEND_WITH_FLAGS(dst, flags, ...) dst = ChefStrPrependWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)
#define SELF_APPEND_WITH_FLAGS(dst, flags, ...) dst = ChefStrAppendWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)

#define PREPEND_WITH_FLAGS(dst, flags, ...) ChefStrPrependWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)
#define APPEND_WITH_FLAGS(dst, flags, ...) ChefStrAppendWithFlagsImpl(dst, flags, __VA_ARGS__, NULL)

#define ADD_FLAGS(dst, flags, ...) \
	do { \
		if (strcmp(flags, "-lib") == 0) { \
			SELF_APPEND_WITH_FLAGS(dst, flags, __VA_ARGS__); \
		} \
		if (strcmp(flags, "-l") == 0) { \
			SELF_PREPEND_WITH_FLAGS(dst, flags, __VA_ARGS__); \
		} \
	} while(0);


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


bool gbDebug = true;
bool gbRelease = false;

bool gbTracy = false;
bool gbAsan = true;
bool gbTest = false;

bool gbVulkan = true;
bool gbOpenGL = true;
bool gbD3D11 = false;
bool gbD3D12 = false;

#ifdef __linux__
bool gbGLFW3 = true;
#elif _WIN32
bool gbGLFW3 = false;
#endif

Command
CommandInit(void)
{	
	Command cmd = {
		.pROOTFOLDER = STR("src"),
		.pNAME = STR("yuseong"),
		.pEXTENSION = STR(EXTENSION),
		.pBUILD_DIR = STR("build"),
		.pOBJ_DIR = STR("build" SLASH "obj"),
		.pCC = STR("clang"),
		.pCPP = STR("clang++"),
		.pSRC_DIR = STR("src"),
		.pCFLAGS = STR(""),
		.pLIB_PATH = STR(""),
		.pINCLUDE_DIRS = STR(""),
		.pLIBS = STR(""),
		.pDEFINES = STR(""),
	};
	char *INCLUDEFLAG = NULL;
	char *LIBFLAG = NULL;
	char *LIBDEPEND = NULL;
	char *OPTIFLAGS = NULL;
	char *DEBUGMODEFLAGS = NULL;
	char *DEFINEFLAG = NULL;

#ifdef _WIN32 
	if ((strcmp(cmd.pCC, "clang") == 0 || (strcmp(cmd.pCC, "gcc") == 0)))
	{
		OPTIFLAGS = STR("-O3");
		DEBUGMODEFLAGS = STR("-O0");
		DEFINEFLAG = STR("-D");
		INCLUDEFLAG = STR("-I");
		LIBFLAG = STR("-L");
		LIBDEPEND = STR("-l");
		SELF_APPEND(cmd.pCFLAGS, "-Wall -Wextra -Werror");
		SELF_APPEND(cmd.pCFLAGS, " -g3");
	}
	else
	{
		OPTIFLAGS = STR("/O2");
		DEBUGMODEFLAGS = STR("/O");
		DEFINEFLAG = STR("/D");
		INCLUDEFLAG = STR("/I");
		LIBFLAG = STR("/L");
		LIBDEPEND = STR(".lib");
		SELF_APPEND(cmd.pCFLAGS, "/Wall");
		SELF_APPEND(cmd.pCFLAGS, " /Zi");
	}
	SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "PLATFORM_WINDOWS");
	ADD_FLAGS(cmd.pLIBS, LIBDEPEND, "shlwapi", "shell32", "gdi32", "winmm", "user32");

#elif __linux__
	INCLUDEFLAG = STR("-I");
	LIBFLAG = STR("-L");
	LIBDEPEND = STR("-l");
	DEFINEFLAG = STR("-D");
	OPTIFLAGS = STR("-O3");
	DEBUGMODEFLAGS = STR("-O0");
	cmd.pDEFINES = STR("-DPLATFORM_LINUX -DYGLFW3 ");
	SELF_APPEND(cmd.pCFLAGS, "-ggdb3");
	SELF_APPEND(cmd.pLIBS, "-lwayland-client -lxkbcommon -lm ");
#else
	cmd.pDEFINES = STR("");
	cmd.pLIBS = STR("");
#endif

	SELF_PREPEND_WITH_FLAGS(cmd.pINCLUDE_DIRS, INCLUDEFLAG, "src", "src/core", "src/renderer/opengl");
	cmd.pCPPFLAGS = STR("");

	if ((strcmp(cmd.pCC, "clang") == 0 || (strcmp(cmd.pCC, "gcc") == 0)))
	{
		SELF_APPEND(cmd.pCFLAGS, " -fno-inline", " -fno-omit-frame-pointer");
		SELF_APPEND(cmd.pCFLAGS, " -Wno-missing-field-initializers", " -Wno-unused-but-set-variable");
		SELF_APPEND(cmd.pCFLAGS, " -Wno-uninitialized");
		SELF_APPEND(cmd.pCFLAGS, " -Wvarargs");
	}
	if (gbDebug)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "_DEBUG", "DEBUG", "YUDEBUG ");
		SELF_APPEND(cmd.pCFLAGS, " ", DEBUGMODEFLAGS);
	}
	else if (gbRelease)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "_RELEASE", "RELEASE", "YURELEASE ");
		SELF_APPEND(cmd.pCFLAGS, " ", OPTIFLAGS);
	}

	if (gbTest) SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "TESTING ");

	if (gbVulkan)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pLIB_PATH, LIBFLAG, VULKANLIB_PATH);
		SELF_PREPEND_WITH_FLAGS(cmd.pINCLUDE_DIRS, INCLUDEFLAG, VULKAN_PATH);
		ADD_FLAGS(cmd.pLIBS, LIBDEPEND, VULKANLIB);
	}
	if (gbOpenGL)
	{
		ADD_FLAGS(cmd.pLIBS, LIBDEPEND, OPENGLLIB);
	}
#ifdef _WIN32
	else if (gbD3D11) { ADD_FLAGS(cmd.pLIBS, LIBDEPEND, "d3dcompiler", "d3d11", "dxgi", "dxguid"); }
	else if (gbD3D12) { }
#endif
	if (gbGLFW3)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pINCLUDE_DIRS, INCLUDEFLAG, GLFW_PATH);
		SELF_PREPEND_WITH_FLAGS(cmd.pLIB_PATH, LIBFLAG, GLFWLIB_PATH);
		ADD_FLAGS(cmd.pLIBS, LIBDEPEND, GLFWLIB);
	}
	if (gbTracy)
	{
		SELF_PREPEND_WITH_FLAGS(cmd.pDEFINES, DEFINEFLAG, "TRACY_ENABLE");
		/* CPPFLAGS =-Wno-format */
		/* Also, compile cpp flags with different struct ? */
	}
	if (gbAsan)
	{
		SELF_APPEND(cmd.pCFLAGS, " ", "-fsanitize=address");
	}
	SELF_PREPEND_WITH_FLAGS(cmd.pINCLUDE_DIRS, INCLUDEFLAG, TRACY_PATH, TRACYTRACY_PATH);
	return cmd;
}

bool
StrIsEqual(const char* s1, const char* s2)
{
	if (strcmp(s1, s2) == 0)
		return true;
	else
		return false;
}

bool
ArgsCheck(int argc, char** ppArgv)
{
	if (StrIsEqual(ppArgv[1], "-h"))
	{
		fprintf(stderr, "Usage: jasb\n");
		return false;
	}
	int i = -1;
	while (++i < argc)
	{
		if (StrIsEqual(ppArgv[i], "vk") || StrIsEqual(ppArgv[i], "vulkan") || StrIsEqual(ppArgv[i], "VULKAN")
				|| StrIsEqual(ppArgv[i], "Vulkan"))
		{
			gbVulkan = true;
			fprintf(stderr, "Vulkan backend chosen\n");
			continue;
		}
		if (StrIsEqual(ppArgv[i], "TRACY") || StrIsEqual(ppArgv[i], "tracy"))
		{
			gbTracy = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "gl") || StrIsEqual(ppArgv[i], "opengl") || StrIsEqual(ppArgv[i], "OpenGL"))
		{
			gbOpenGL = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "D3D11"))
		{
			gbD3D11 = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "D3D12"))
		{
			gbD3D12 = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "GLFW3"))
		{
			gbGLFW3 = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "TEST"))
		{
			gbTest = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "DEBUG"))
		{
			gbDebug = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "RELEASE"))
		{
			gbRelease = true;
			continue;
		}
		if (StrIsEqual(ppArgv[i], "ASAN"))
		{
			gbAsan = true;
			continue;
		}
	}
	return true;
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
ClangCompileCommandsJson(const char *pCompileCommands)
{
	FileList *pListJson = GetFileList(".", JSONREGEX);
	PrintFileList(pListJson);
	if (!pListJson)
		return 1;
	return ConstructCompileCommandsJson(pListJson, pCompileCommands);
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
		free(pFileList->pFiles[i].pObjName);
	}
	free(pFileList->pFiles);
	free(pFileList);
}

/*****************************************************************************/
								/*WINDOWS*/
/*****************************************************************************/
#ifdef WIN32

/*
 * BOOL
 * DirectoryExists(LPCTSTR szPath)
 * {
 *   DWORD dwAttrib = GetFileAttributes(szPath);
 * 
 *   return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
 * }
 */
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

bool
MkdirImpl(char *pStr)
{
	return CreateDirectory(pStr, NULL);
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
#endif

void
PerrorLog(char *pMsg, char *pFile, int line) 
{
	char pStr[2048];
	if (strlen(pFile) >= (2048 - strlen(" at :%d:") + strlen(pMsg)))
		perror(pMsg);
	else
	{
		sprintf(&pStr[strlen(pMsg)], "%s at %s:%d: ", pMsg, pFile, line);
		perror(pMsg);
	}
	perror(pMsg);
}

yError
CompileCmdJson(char **ppCmds, size_t nbCmds)
{
	YMB char *pName = "compile_commands.json";
    /*
	 * FILE *pFd = fopen(pName, "w");
	 * if (!pFd)
	 * {
	 * 	PERROR_LOG("CompileCmdJson");
	 * 	return Y_ERROR_JSON;
	 * }
	 * fprintf(pFd, "%s", CCJSON_BEGIN);
     */
	printf("%s", CCJSON_BEGIN);
	YMB size_t totalSize = 0;
	YMB size_t count = 0;
	size_t i = 0;
	size_t lastOne = nbCmds - 1; 
	char *pEndComa = ",\n";
	while (i < nbCmds)
	{
		printf("[%zu]: %s\n", i, ppCmds[i]);
		
		/* printf("%s%s%s,", Y_OPENCURLY, Y_DIRECTORY, PUTINQUOTE(YCWD), Y_FILE, PUTINQUOTE(ppCmds[i])) */
		if (i == lastOne)
			pEndComa = "\n";
		i++;
	}
	printf("%s", CCJSON_END);
    /*
	 * fprintf(pFd, "%s", CCJSON_END);
	 * fclose(pFd);
     */
	return Y_NO_ERROR;
}

yError
Compile(Command* pCmd, FileList* pList, char **ppOut)
{
	yError result = Y_NO_ERROR;
	if (pList->nbElems <= 0) { fprintf(stderr, "No files to compile found ..\n"); result = Y_ERROR_EMPTY; return result; }

	char *OUTFLAG;
	char *MODEFLAG;

	if ((strcmp("clang", pCmd->pCC) == 0) || (strcmp("gcc", pCmd->pCC) == 0))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
	}
	Command a = *pCmd;
	char **ppJson = malloc(sizeof(char *) * pList->nbElems);

	for (size_t i = 0; i < pList->nbElems; i++)
	{
		char *fname = pList->pFiles[i].pFullPath;
		char *outname = pList->pFiles[i].pObjName;
		ppJson[i] = STR("");
		SELF_APPEND_SPACE(ppJson[i], a.pCC, a.pCFLAGS, a.pDEFINES, a.pINCLUDE_DIRS, MODEFLAG, fname, OUTFLAG, outname);
		result = EXECUTE(ppJson[i], true);
		if (result != 0)
		{
			free(ppJson);
			return Y_ERROR_BUILD;
		}
	}
	/* CompileCmdJson(ppJson, pList->nbElems); */
	free(ppJson);

	// NOTE: Making the path .o files for linker
	size_t count = 0;
	size_t total = 0;

	for (size_t i = 0; i < pList->nbElems; i++)
		total += strlen(pList->pFiles[i].pObjName);

	char *pTemp = malloc(sizeof(char) * total + (pList->nbElems) + 1);
	for (size_t i = 0; i < pList->nbElems; i++)
		count += sprintf(pTemp + count, "%s ", pList->pFiles[i].pObjName);
	if (count > 0)
		count--;
	pTemp[count] = 0;
	if (!pTemp)
	{
		free(pTemp);
		result = Y_ERROR_EMPTY;
	}
	*ppOut = pTemp;
	return result;
}

yError
Link(Command* pCmd, const char *pObj, const char *pOutName)
{
	yError result = Y_NO_ERROR;
	char *pCommand = STR("");
	Command a = *pCmd;

	char *OUTFLAG = NULL;
	char *MODEFLAG = NULL;

	if ((strcmp("clang", pCmd->pCC) == 0) || (strcmp("gcc", pCmd->pCC) == 0))
	{
		OUTFLAG = STR("-o");
		MODEFLAG = STR("-c");
	}
	SELF_APPEND_SPACE(pCommand, a.pCC, a.pCFLAGS, a.pDEFINES, pObj, OUTFLAG, pOutName, pCmd->pLIB_PATH, pCmd->pLIBS);
	result = EXECUTE(pCommand, 1);
	if (result != 0)
		return Y_ERROR_LINK;
	return result;
}

yError
Build(Command* pCmd, FileList* pListCfiles)
{
	char *pOutput = STR("");
	SELF_APPEND(pOutput, pCmd->pBUILD_DIR, SLASH, pCmd->pNAME, pCmd->pEXTENSION);
	char *pOutObjs = NULL;
	yError result = Y_NO_ERROR;

	result = Compile(pCmd, pListCfiles, &pOutObjs);
	if (result != Y_NO_ERROR)
		return result;
	result = Link(pCmd, pOutObjs, pOutput);
	if (result != Y_NO_ERROR)
		return result;
	free(pOutObjs);
	return result;
}

bool
DoesExist(const char *pPath)
{
	struct stat sb;
	int result = stat(pPath, &sb);
	if (result < 0)
	{
		PERROR_LOG("DoesExist()");
		return false;
	}
	return true;
}

/*
 * TODO: 
 * - Multithreading -> CrossPlatform = boring for this lots of lines
 * - Incremental builds -> Most of the time useless / problematic
 */
int
main(int argc, char **ppArgv)
{
	if (argc >= 2)
	{
		if (ArgsCheck(argc, ppArgv) == false)
			return 1;
	}
	ChefInit();
	Command cmd = CommandInit();
	if (ISDIRECTORY(cmd.pOBJ_DIR) == false || DoesExist(cmd.pOBJ_DIR) == false)
	{
		if (MKDIR(cmd.pBUILD_DIR) == false )
			ERROR_EXIT("Mkdir: ");
		if (MKDIR(cmd.pOBJ_DIR) == false )
			ERROR_EXIT("Mkdir: ");
	}

	YMB char *pCompileCommands = STR("compile_commands.json");

	FileList *pListCfiles = GetFileListAndObjs(&cmd, "*.c");
	if (!pListCfiles) { fprintf(stderr, "Something happened in c\n"); return 1; }

	FileList *pListCppFiles = GetFileListAndObjs(&cmd, "*.cpp");
	if (!pListCppFiles) { fprintf(stderr, "Something happened in cpp\n"); return 1; }
	yError result = Build(&cmd, pListCfiles);
	if ( result != Y_NO_ERROR)
	{ fprintf(stderr, "%s\n", GetErrorMsg(result)); return 1; }

    /*
	 * if (ClangCompileCommandsJson(pCompileCommands))
	 * 	goto exiting;
     */

	DestroyFileList(pListCfiles);
	DestroyFileList(pListCppFiles);
	ChefDestroy();
	printf("number = %zu\n", gChef.nbElems);
	return 0;

/* exiting: */
	fprintf(stderr, "Error");
	/* perror(pListJson->pFiles->pFullPath); */
	/* DestroyFileList(pListJson); */

	printf("number = %zu\n", gChef.nbElems);
	ChefDestroy();
	return 1;
}
