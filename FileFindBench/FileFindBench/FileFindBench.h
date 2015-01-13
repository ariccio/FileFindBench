#pragma once

#include "stdafx.h"
#include "resource.h"

__int64 stdRecurseFind( _In_ std::wstring dir, _In_ bool isLargeFetch, _In_ bool isBasicInfo );

__int64 stdRecurseFindFutures( _In_ std::wstring dir, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo );

const DOUBLE getAdjustedTimingFrequency( );
// TODO: reference additional headers your program requires here


struct minimalFindData {
	minimalFindData( );
	minimalFindData( std::wstring inS, DWORD inA, DWORD inFSH, DWORD inFSL );
	minimalFindData( minimalFindData&& in );
	std::wstring fileName;
	DWORD attributes;
	DWORD fileSizeHigh;
	DWORD fileSizeLow;

	};

#ifdef MFC_C_FILE_FIND_TEST
struct FILEINFO {
	CString      name;
	DWORD        attributes;
	};

class CFileFindMod : public CFileFind {
	public:
	DWORD GetAttributes( ) const;//brilliant idea I got from WinDirStat!
	};


std::int64_t doCFileFind( _In_ CString dir ) { //CString is native to MFC, avoids many conversions to<>from std::wstring
	std::int64_t num = 0;
	CFileFindMod finder;
	if ( dir.Right( 1 ) != _T( '\\' ) ) {
		dir += L"\\*.*";
		}
	else {
		dir += L"*.*";//Yeah, if you're wondering, `*.*` works for files WITHOUT extensions.
		}
	auto b = finder.FindFile( dir );
	while ( b ) {
		b = finder.FindNextFileW( );
		if ( finder.IsDots( ) ) {
			continue;
			}
		else if ( finder.IsDirectory( ) ) {
			++num;
			num += doCFileFind( finder.GetFilePath( ) );
			}
		else {
			FILEINFO fi;
			fi.attributes = finder.GetAttributes( );
			if ( ( fi.attributes & FILE_ATTRIBUTE_DEVICE ) || ( fi.attributes & FILE_ATTRIBUTE_INTEGRITY_STREAM ) || ( fi.attributes & FILE_ATTRIBUTE_HIDDEN ) || ( fi.attributes & FILE_ATTRIBUTE_NO_SCRUB_DATA ) || ( fi.attributes & FILE_ATTRIBUTE_OFFLINE ) || ( fi.attributes & FILE_ATTRIBUTE_REPARSE_POINT ) || ( fi.attributes & FILE_ATTRIBUTE_SPARSE_FILE ) || ( fi.attributes & FILE_ATTRIBUTE_VIRTUAL ) ) {
				--num;
				}
			++num;
			}

		}
	return num;
	}

void wrapCFileFind( _In_ const CString dir ) {
	//work with api monitor shows MFC uses FindFirstFileW, which then internally calls NtQueryDirectoryFile with FileBothDirectoryInformation and ReturnSingleEntry==TRUE 
	wprintf( L"--------------------------------------------\r\n" );
	wprintf( L"MFC iteration\r\n" );
	LARGE_INTEGER startTime;
	LARGE_INTEGER endTime;
	const auto adjustedTimingFrequency = getAdjustedTimingFrequency( );

	const BOOL res2 = QueryPerformanceCounter( &startTime );

	std::int64_t num = doCFileFind( dir );

	const BOOL res3 = QueryPerformanceCounter( &endTime );
	
	const double totalTime = ( endTime.QuadPart - startTime.QuadPart ) * adjustedTimingFrequency;

	if ( ( !res2 ) || ( !res3 ) ) {
		wprintf( L"QueryPerformanceCounter Failed!!!!!! Disregard any timing data!!\r\n" );
		}

	wprintf( L"Time in seconds:  %f\r\n", totalTime );

	wprintf( L"Number of items: %I64d\r\n", num );
	wprintf( L"MFC iteration,                                                      Time in seconds:  %f\r\n", totalTime );
//  ss << L"Iteration without FIND_FIRST_EX_LARGE_FETCH, ";
//                                                ss << L" WITH    isBasicInfo, ";
	}
#endif
