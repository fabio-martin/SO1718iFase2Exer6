#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include "../Include/PrintUtils.h"
#include "../Include/BmpUtils.h"

static PCTCH rot_to_str(ROTATION_enum_t rot) {
	LPCSTR rot_strs[] = { "HOR", "VER", "90", "180", "270" };
	return rot_strs[rot];
}

// Struct to hold final result
typedef struct {
	PTCHAR data;				// Result (pathnames with files separated by character '\n')
	DWORD len;					// Actual length of result
	DWORD capacity;				// Capacity of result
} RES, *PRES;

static VOID ResAdd(PRES res, PTCHAR str) {
	DWORD filepathLen = _tcscnlen(str, MAX_PATH) + 1; // +1 for _T('\n') 
	if (filepathLen + res->len > res->capacity) {
		// Expand filenames buffer if there are no enough space
		res->capacity = res->capacity + MAX_PATH;
		res->data = (PCHAR)realloc(res->data, res->capacity * sizeof(TCHAR));
	}
	PTCHAR cur = res->data + res->len;
	res->len += filepathLen;
	_stprintf_s(cur, filepathLen + 1, _T("%s\n"), (PCTSTR)str); // fileLen + 1 for null str terminator
}

typedef struct {
	// Global context
	ROTATION_enum_t oper;		// Operation type
	FILETIME localTime;			// Current SYSTEM_TIME converted to FILETIME used to eliminate repeated rotation process
} OPER_CTX, *POPER_CTX;

static BOOL file_exists(LPCTSTR filePath) {
	//This will get the file attributes bitlist of the file
	DWORD fileAtt = GetFileAttributes(filePath);

	//If an error occurred it will equal to INVALID_FILE_ATTRIBUTES
	if (fileAtt == INVALID_FILE_ATTRIBUTES)
		//So lets throw an exception when an error has occurred
		return FALSE;

	//If the path referers to a directory it should also not exists.
	return ((fileAtt & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

static LPVOID ProcessBmps(PCTSTR filepath, PCTSTR pathname_out, LPVOID ctx) {
	POPER_CTX globalCtx = (POPER_CTX)ctx;
	TCHAR pathname[MAX_PATH];	// auxiliary buffer
	// the pathname is needed to define new filename based on filename match string and operation type
	PCTCH last_folder = _tcsrchr(filepath, '/');
	assert(last_folder != NULL && last_folder - filepath < MAX_PATH);
	// Consider '/' character
	last_folder += 1;
	_memccpy(pathname, filepath, last_folder - filepath, last_folder - filepath);
	pathname[last_folder - filepath] = _T('\0');

	_tprintf(_T("BMPUtils_singlethread.ProcessBmps: \n"
		"\tOper = %s\n"
		"\tFilepath = \"%s\"\n"
		"\tPath out = \"%s\"\n"), rot_to_str(globalCtx->oper), filepath, pathname_out);

	WIN32_FIND_DATA fileData;
	HANDLE fileIt = FindFirstFile(filepath, &fileData);
	if (fileIt == INVALID_HANDLE_VALUE) return NULL;

	// Initiate result
	RES res = {NULL, 0,0};

	// Process directory entries
	do {
		if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			// Particular case that process all bmp files from a root directory: ".../"
			// Not processing "." and ".." files!
			if (_tcscmp(fileData.cFileName, _T(".")) && _tcscmp(fileData.cFileName, _T(".."))) {
				TCHAR buffer[MAX_PATH];		// auxiliary buffer
				_stprintf_s(buffer, _T("%s%s/"), pathname, fileData.cFileName);
				// Recursively process child directory
				LPVOID r = ProcessBmps(buffer, pathname_out, ctx);
				ResAdd(&res, (PTCHAR)r);
			}
		}
		else {
			// The file iterated may is newer than start of operation, so do not process this file. This file is an output of an earlier processed file. 
			// It may happen when output directory is the same that input directory.
			if (CompareFileTime(&globalCtx->localTime, &fileData.ftCreationTime) == 1) {
				// Process file archive
				// Create new filename based on original filename and operation
				TCHAR buffer_in[MAX_PATH];
				TCHAR buffer_out[MAX_PATH];
				LPCSTR rot_str = rot_to_str(globalCtx->oper);
				_stprintf_s(buffer_in, _T("%s%s"), pathname, fileData.cFileName);
				_stprintf_s(buffer_out, _T("%s%s_%s"), pathname_out, rot_str, fileData.cFileName);
				// Eliminate duplicate work if output file already exists.
				if (file_exists(buffer_out) == FALSE) {
					bmp_rot(buffer_in, buffer_out, globalCtx->oper);

					// Accumulate filename
					ResAdd(&res, (PTCHAR)buffer_out);
				}
			}
		}
	} while (FindNextFile(fileIt, &fileData) == TRUE);

	FindClose(fileIt);

	return res.data;
}

LPVOID BMPUtilsProcessRotationSequential(PCTSTR oper_arg, PCTSTR filepathIn, PCTSTR pathnameOut) {
	ROTATION_enum_t rot;
	if (_tcscmp(oper_arg, "90") == 0)
		rot = ROTATE_90;
	else if (_tcscmp(oper_arg, "180") == 0)
		rot = ROTATE_180;
	else if (_tcscmp(oper_arg, "270") == 0)
		rot = ROTATE_270;
	else if (_tcscmp(oper_arg, "HOR") == 0)
		rot = ROTATE_HORIZONTAL;
	else if (_tcscmp(oper_arg, "VER") == 0)
		rot = ROTATE_VERTICAL;

	OPER_CTX ctx;
	ctx.oper = rot;
	SYSTEMTIME time;
	GetSystemTime(&time);
	DWORD bres = SystemTimeToFileTime(&time, &ctx.localTime);
	assert(bres);

	return ProcessBmps(filepathIn, pathnameOut, &ctx);
}
