//
// WndLib
// Copyright (c) 1994-2012 Mark H. P. Lord. All rights reserved.
//
// See LICENSE.txt for license.
//

#ifndef WNDLIB_VERINFO_H
#define WNDLIB_VERINFO_H

#include "WndLib.h"

namespace WndLib
{
	// Reads version information from an executable or DLL.
	//
	// Example Usage:
	//
	//  	VerInfo vi;
	//  	if (! vi.Read(TEXT("c:\\windows\\system32\\comdlg32.dll")))
	//  		printf("Can't read version information.\n");
	//  	else
	//  	{
	//  		const int *version = vi.GetVersion();
	//  		printf("Version: %d.%d.%d.%d\n", version[0], version[1], version[2], version[3]);
	//  		printf("Title: %s\n", vi.GetTitle());
	//  		printf("Copyright: %s\n", vi.GetCopyright());
	//  		printf("Comments: %s\n", vi.GetComments());
	//  	}
	// 
	class WNDLIB_EXPORT VerInfo
	{
	public:

		VerInfo();

		// Read version information from the specified file. Returns false
		// on error.
		bool Read(const TCHAR *filename);

		// The same as calling Read with the path to the current module.
		bool ReadCurrentModule();

		// Returns a pointer to an array of 4 ints.
		const int *GetVersion() const
		{
			return _version;
		}

		// Return the module's title.
		const TCHAR *GetTitle() const
		{
			return _title;
		}

		// Return the module's copyright.
		const TCHAR *GetCopyright() const
		{
			return _copyright;
		}

		// Return the module's comments.
		const TCHAR *GetComments() const
		{
			return _comments;
		}

	private:

		int _version[4];
		TCHAR _title[1024];
		TCHAR _copyright[1024];
		TCHAR _comments[1024];
	};
}

#endif
