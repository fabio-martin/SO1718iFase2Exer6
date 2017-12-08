#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include <process.h>
#include "../Include/PrintUtils.h"
#include "../Include/BmpUtils.h"

LPVOID BMPUtilsProcessRotationSequential(PCTSTR oper_arg, PCTSTR filepathIn, PCTSTR pathnameOut);

DWORD _tmain(DWORD argc, PTCHAR argv[]) {

	if (argc < 3) {
		_tprintf(_T("Use: %s <operation> <filepath> <pathname>\n"
					"\toperation: VER | HOR | 90 | 180 | 270\n"
					"\tfilepath: pathname/filename. Filename may contain \'?\' and \'*\' wildcards\n"
			        "\tpathname: pathname to write processed files\n"
					), argv[0]);
		exit(0);
	}

	PCTSTR oper_arg = argv[1];
	PCTSTR filepath = argv[2];
	PCTSTR pathname = argv[3];

	PTCHAR files = (PTCHAR)BMPUtilsProcessRotationSequential(oper_arg, filepath, pathname);
	_tprintf("Main: Files created: \n%s", files);
	free(files);

	PRESS_TO_FINISH("");

	return 0;

}

