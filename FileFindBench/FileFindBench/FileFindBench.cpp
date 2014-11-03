#include "stdafx.h"
#include "FileFindBench.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



CWinApp theApp;

#define BASE 1024
#define HALF_BASE BASE/2

#define TRACE_OUT(x) std::endl << L"\t\t" << #x << L" = `" << x << L"` "//awesomely useful macro, included now, just in case I need it later.

#define TRACE_STR(x) << L" " << #x << L" = `" << x << L"`;"

static_assert( sizeof( long long ) == sizeof( std::int64_t ), "bad int size!" );


struct FileFindRecord {
	double time_seconds;
	bool   is_FIND_FIRST_EX_LARGE_FETCH : 1;
	bool   is_BasicInfo                 : 1;
	bool   is_Async                     : 1;
	bool operator<( const FileFindRecord& other ) {
		return time_seconds < other.time_seconds;
		}
	};


//Thank you, Orjan Westin:
std::wstring GetLastErrorStdStr( DWORD error ) {
	if ( error ) {
		LPVOID lpMsgBuf;
		DWORD bufLen = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), reinterpret_cast<LPTSTR>( &lpMsgBuf ), 0, NULL );
		if ( bufLen ) {
			auto lpMsgStr = static_cast<PWSTR>( lpMsgBuf );
			std::wstring result( lpMsgStr, lpMsgStr + bufLen );
			LocalFree( lpMsgBuf );
			return result;
			}
		}
	return std::wstring( );
	}

//http://msdn.microsoft.com/en-us/library/windows/desktop/aa446619(v=vs.85).aspx
BOOL SetPrivilege( HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege ) {
	/*
	  HANDLE hToken,          // access token handle
	  LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
	  BOOL bEnablePrivilege   // to enable or disable privilege
	*/

	TOKEN_PRIVILEGES tp;
	LUID luid;

	if ( !LookupPrivilegeValue( NULL, lpszPrivilege, &luid ) ) {
		printf( "LookupPrivilegeValue error: %u\n", GetLastError( ) );
		return FALSE;
		}

	tp.PrivilegeCount = 1;
	tp.Privileges[ 0 ].Luid = luid;
	if ( bEnablePrivilege ) {
		tp.Privileges[ 0 ].Attributes = SE_PRIVILEGE_ENABLED;
		}
	else {
		tp.Privileges[ 0 ].Attributes = 0;
		}
	// Enable the privilege or disable all privileges.

	if ( !AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), PTOKEN_PRIVILEGES( NULL ), PDWORD( NULL ) ) ) {
		printf( "AdjustTokenPrivileges error: %u\n", GetLastError( ) );
		return FALSE;
		}

	if ( GetLastError( ) == ERROR_NOT_ALL_ASSIGNED ) {
		printf( "The token does not have the specified privilege. \n" );
		return FALSE;
		}
	return TRUE;
	}


void FlushCache( ) {
	//http://msdn.microsoft.com/en-us/library/windows/desktop/aa965240(v=vs.85).aspx
	//"To flush the cache, specify (SIZE_T) -1."
	std::wcout << L"Flushing cache..." << std::endl;
	auto res = SetSystemFileCacheSize( SIZE_T( -1 ), SIZE_T( -1 ), 0 );
	if ( res == 0 ) {
		auto LastError = GetLastError( );
		std::wcerr << L"Last error: " << LastError << std::endl;
		std::wcerr << GetLastErrorStdStr( LastError ) << std::endl;
		}
	}

std::int64_t descendDirectory( _In_ WIN32_FIND_DATA& fData, _In_ const std::wstring& normSzDir, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo, _In_ const bool futures = false ) {
	std::wstring newSzDir = normSzDir;//MUST operate on copy!
	newSzDir.reserve( MAX_PATH );
	newSzDir += L"\\";
	newSzDir += fData.cFileName;
	std::int64_t num = 0;
	if ( futures ) {
		num += stdRecurseFindFutures( newSzDir, isLargeFetch, isBasicInfo );
		}
	else {
		num += stdRecurseFind( newSzDir, isLargeFetch, isBasicInfo );
		}
	return num;
	}

void trace_fDataBits( _In_ const WIN32_FIND_DATA& fData, _In_ const std::wstring& normSzDir ) {
	if ( !( ( fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) || (fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA) ) ) {
		//TOO MANY DAMN HIDDEN FILES!
		std::wcout << std::endl << L"\tWeird file encountered in " << normSzDir << std::endl << L"\tWeird file attributes:";

		std::wcout << TRACE_OUT( fData.cFileName ) << std::endl;
		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE              ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE              ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED           ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED           ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM    ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM    ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA       ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA       ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE             ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE             ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT       ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT       ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE         ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE         ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL             ) ) {
			std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL             ) );
			}
			//std::wcout << std::endl;
		}
	}

std::int64_t stdRecurseFind( _In_ std::wstring dir, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo ) {

	std::int64_t num = 0;
	dir.reserve( MAX_PATH );
	std::wstring normSzDir(dir);
	normSzDir.reserve( MAX_PATH );
	if ( ( dir.back( ) != L'*' ) && ( dir.at( dir.length( ) - 2 ) != L'\\' ) ) {
		dir += L"\\*";
		}
	else if ( dir.back( ) == L'\\' ) {
		dir += L"*";
		}

	WIN32_FIND_DATA fData;
	HANDLE fDataHand = NULL;

	if ( isLargeFetch ) {
		if ( isBasicInfo ) {
			fDataHand = FindFirstFileExW( dir.c_str( ), FindExInfoBasic,    &fData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH );
			}
		else {
			fDataHand = FindFirstFileExW( dir.c_str( ), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH );
			}
		}
	else {
		if ( isBasicInfo ) {
			fDataHand = FindFirstFileExW( dir.c_str( ), FindExInfoBasic,    &fData, FindExSearchNameMatch, NULL, 0 );
			}
		else {
			fDataHand = FindFirstFileExW( dir.c_str( ), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, 0 );
			}
		}
	if ( fDataHand != INVALID_HANDLE_VALUE ) {
		if ( !( wcscmp( fData.cFileName, L".." ) == 0 ) ) {
			//++num;
			}
		auto res = FindNextFileW( fDataHand, &fData );
		while ( ( fDataHand != INVALID_HANDLE_VALUE ) && ( res != 0 ) ) {
			if ( !std::wcout.good()) {//Slower than it should be.
				auto badBits    = std::wcout.exceptions( );
				auto wasBadBit  = badBits & std::ios_base::badbit;
				auto wasFailBit = badBits & std::ios_base::failbit;
				auto wasEofBit  = badBits & std::ios_base::eofbit;
				std::wcout.clear( );
				std::wcout << L"wcout was in a bad state!" << std::endl;
				std::wcout << TRACE_OUT( wasBadBit );
				std::wcout << TRACE_OUT( wasFailBit );
				std::wcout << TRACE_OUT( wasEofBit );
				std::wcout << std::endl;
				}
			auto scmpVal = wcscmp( fData.cFileName, L".." );
			if ( ( !( scmpVal == 0 ) ) ) {
				++num;
				}
			if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ( scmpVal != 0 ) ) {
				num += descendDirectory( fData, normSzDir, isLargeFetch, isBasicInfo );
				}
			else if ( ( scmpVal != 0 ) && ( fData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL ) ) {
				//++num;
				}
			else if ( ( scmpVal != 0 ) && ( ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL ) ) ) ) {
				
				trace_fDataBits( fData, normSzDir );
				--num;
				}
			res = FindNextFileW( fDataHand, &fData );

			}
		}
	
	FindClose( fDataHand );
	return num;
	}


std::int64_t stdRecurseFindFutures( _In_ std::wstring dir, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo ) {

	std::int64_t num = 0;
	dir.reserve( MAX_PATH );
	std::wstring normSzDir(dir);
	normSzDir.reserve( MAX_PATH );
	if ( ( dir.back( ) != L'*' ) && ( dir.at( dir.length( ) - 2 ) != L'\\' ) ) {
		dir += L"\\*";
		}
	else if ( dir.back( ) == L'\\' ) {
		dir += L"*";
		}
	std::vector<std::future<std::int64_t>> futureDirs;
	futureDirs.reserve( 100 );//pseudo-arbitrary number
	WIN32_FIND_DATA fData;
	HANDLE fDataHand = NULL;

	if ( isLargeFetch ) {
		if ( isBasicInfo ) {
			fDataHand = FindFirstFileExW( dir.c_str( ), FindExInfoBasic,    &fData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH );
			}
		else {
			fDataHand = FindFirstFileExW( dir.c_str( ), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH );
			}
		}
	else {
		if ( isBasicInfo ) {
			fDataHand = FindFirstFileExW( dir.c_str( ), FindExInfoBasic,    &fData, FindExSearchNameMatch, NULL, 0 );
			}
		else {
			fDataHand = FindFirstFileExW( dir.c_str( ), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, 0 );
			}
		}
	if ( fDataHand != INVALID_HANDLE_VALUE ) {
		if ( !( wcscmp( fData.cFileName, L".." ) == 0 ) ) {
			//++num;
			}
		auto res = FindNextFileW( fDataHand, &fData );
		while ( ( fDataHand != INVALID_HANDLE_VALUE ) && ( res != 0 ) ) {
			if ( !std::wcout.good()) {//Slower than it should be.
				auto badBits    = std::wcout.exceptions( );
				auto wasBadBit  = badBits & std::ios_base::badbit;
				auto wasFailBit = badBits & std::ios_base::failbit;
				auto wasEofBit  = badBits & std::ios_base::eofbit;
				std::wcout.clear( );
				std::wcout << L"wcout was in a bad state!" << std::endl;
				std::wcout << TRACE_OUT( wasBadBit );
				std::wcout << TRACE_OUT( wasFailBit );
				std::wcout << TRACE_OUT( wasEofBit );
				std::wcout << std::endl;
				}
			auto scmpVal = wcscmp( fData.cFileName, L".." );
			if ( ( !( scmpVal == 0 ) ) ) {
				++num;
				}
			if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ( scmpVal != 0 ) ) {
				futureDirs.emplace_back( std::async( std::launch::async|std::launch::deferred, descendDirectory, fData, normSzDir, isLargeFetch, isBasicInfo, true ) );
				}
			else if ( ( scmpVal != 0 ) && ( fData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL ) ) {
				//++num;
				}
			else if ( ( scmpVal != 0 ) && ( ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL ) ) ) ) {
				
				trace_fDataBits( fData, normSzDir );
				--num;
				}
			res = FindNextFileW( fDataHand, &fData );
			}
		}
	for ( auto& a : futureDirs ) {
		num += a.get( );
		}
	FindClose( fDataHand );
	return num;
	}


void stdWork( _In_ std::wstring arg, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo ) {
	std::wcout << L"Working on: `" << arg << L"`" << std::endl;
	std::int64_t numberFiles = 0;
	arg.reserve( MAX_PATH );
	if ( arg.length( ) > 3 ) {
		auto strCmp = ( arg.compare( 0, 4, arg, 0, 4 ) );
		if ( strCmp != 0 ) {
			arg = L"\\\\?\\" + arg;
			std::wcout << L"prefixed `" << arg << L"`, value now: `" << arg << L"`" << std::endl << std::endl;
			}
		}
	else {
		arg = L"\\\\?\\" + arg;
		std::wcout << L"prefixed `" << arg << L"`, value now: `" << arg << L"`" << std::endl << std::endl;
		}
	numberFiles = stdRecurseFind( arg, isLargeFetch, isBasicInfo );
	std::wcout << std::endl << L"Number of items: " << numberFiles << std::endl;
	}

void stdWorkAsync( _In_ std::wstring arg, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo ) {
	std::wcout << L"Working on: `" << arg << L"`" << std::endl;
	std::int64_t numberFiles = 0;
	arg.reserve( MAX_PATH );
	if ( arg.length( ) > 3 ) {
		auto strCmp = ( arg.compare( 0, 4, arg, 0, 4 ) );
		if ( strCmp != 0 ) {
			arg = L"\\\\?\\" + arg;
			std::wcout << L"prefixed `" << arg << L"`, value now: `" << arg << L"`" << std::endl << std::endl;
			}
		}
	else {
		arg = L"\\\\?\\" + arg;
		std::wcout << L"prefixed `" << arg << L"`, value now: `" << arg << L"`" << std::endl << std::endl;
		}
	numberFiles = stdRecurseFindFutures( arg, isLargeFetch, isBasicInfo );
	std::wcout << std::endl << L"Number of items: " << numberFiles << std::endl;
	}


const DOUBLE getAdjustedTimingFrequency( ) {
	LARGE_INTEGER timingFrequency;
	BOOL res1 = QueryPerformanceFrequency( &timingFrequency );
	if ( !res1 ) {
		std::wcout << L"QueryPerformanceFrequency failed!!!!!! Disregard any timing data!!" << std::endl;
		}
	const DOUBLE adjustedTimingFrequency = ( DOUBLE( 1.00 ) / DOUBLE( timingFrequency.QuadPart ) );
	return adjustedTimingFrequency;
	}

std::wstring formatFileFindRecord( _In_ const FileFindRecord& record ) {
	std::wstring retStr;
	if ( record.is_FIND_FIRST_EX_LARGE_FETCH) {
		retStr += L"Iteration WITH    FIND_FIRST_EX_LARGE_FETCH, ";
		}
	else {
		retStr += L"Iteration without FIND_FIRST_EX_LARGE_FETCH, ";
		}
	if ( record.is_BasicInfo ) {
		retStr += L" WITH    isBasicInfo, ";
		}
	else {
		retStr += L" without isBasicInfo, ";
		}
	retStr += L"IsAsync?: ";
	retStr += ( record.is_Async ? L"1" : L"0" );
	retStr += L", ";
	retStr += L"Time in seconds:  ";
	retStr += std::to_wstring( record.time_seconds );
	return retStr;
	}


FileFindRecord iterate( _In_ const std::wstring& arg, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo, _Inout_ std::wstringstream& ss, _In_ const bool IsAsync ) {
	std::wcout << L"--------------------------------------------" << std::endl;
	FlushCache( );
	//if ( isLargeFetch ) {
	//	ss << L"Iteration WITH    FIND_FIRST_EX_LARGE_FETCH, ";
	//	}
	//else {
	//	ss << L"Iteration without FIND_FIRST_EX_LARGE_FETCH, ";
	//	}
	//if ( isBasicInfo ) {
	//	ss << L" WITH    isBasicInfo, ";
	//	}
	//else {
	//	ss << L" without isBasicInfo, ";
	//	}
	//ss << L"IsAsync?: " << IsAsync << L", ";


	LARGE_INTEGER startTime;
	LARGE_INTEGER endTime;
	
	//std::int64_t fileSizeTotal = 0;
	auto adjustedTimingFrequency = getAdjustedTimingFrequency( );

	BOOL res2 = QueryPerformanceCounter( &startTime );
	if ( IsAsync ) {
		stdWorkAsync( arg, isLargeFetch, isBasicInfo );
		}
	else {
		stdWork( arg, isLargeFetch, isBasicInfo );
		}
	BOOL res3 = QueryPerformanceCounter( &endTime );
	
	if ( ( !res2 ) || ( !res3 ) ) {
		std::wcout << L"QueryPerformanceCounter Failed!!!!!! Disregard any timing data!!" << std::endl;
		}

	auto totalTime = ( endTime.QuadPart - startTime.QuadPart ) * adjustedTimingFrequency;
	//times.emplace_back( totalTime );
	//ss << L"Time in seconds:  " << totalTime << std::endl;

	FileFindRecord record;
	record.is_Async = IsAsync;
	record.is_BasicInfo = isBasicInfo;
	record.is_FIND_FIRST_EX_LARGE_FETCH = isLargeFetch;
	record.time_seconds = totalTime;
	FlushCache( );
	std::wcout << std::endl;
	Sleep( 500 );
	return record;
	}

DWORD CFileFindMod::GetAttributes( ) const {
	ASSERT( m_hContext != NULL );
	ASSERT_VALID( this );

	if ( m_pFoundInfo != NULL ) {
		return ( static_cast< LPWIN32_FIND_DATA >( m_pFoundInfo ) )->dwFileAttributes;
		}
	else {
		return INVALID_FILE_ATTRIBUTES;
		}
	}


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
		b = finder.FindNextFile();
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

void wrapCFileFind( _In_ const CString dir, std::wstringstream& ss ) {
	//work with api monitor shows MFC uses FindFirstFileW, which then internally calls NtQueryDirectoryFile with FileBothDirectoryInformation and ReturnSingleEntry==TRUE 
	std::wcout << L"--------------------------------------------" << std::endl;
	std::wcout << L"MFC iteration" << std::endl;
	LARGE_INTEGER startTime;
	LARGE_INTEGER endTime;
	auto adjustedTimingFrequency = getAdjustedTimingFrequency( );

	BOOL res2 = QueryPerformanceCounter( &startTime );

	auto num = doCFileFind( dir );

	BOOL res3 = QueryPerformanceCounter( &endTime );
	
	auto totalTime = ( endTime.QuadPart - startTime.QuadPart ) * adjustedTimingFrequency;

	if ( ( !res2 ) || ( !res3 ) ) {
		std::wcout << L"QueryPerformanceCounter Failed!!!!!! Disregard any timing data!!" << std::endl;
		}

	ss << L"Time in seconds:  " << totalTime << std::endl;

	std::wcout << L"Number of items: " << num << std::endl;
	ss << L"MFC iteration,                                                      Time in seconds:  " << totalTime << std::endl;
//  ss << L"Iteration without FIND_FIRST_EX_LARGE_FETCH, ";
//                                                ss << L" WITH    isBasicInfo, ";
	}



int _tmain( int argc, _In_reads_( argc ) TCHAR* argv[ ], TCHAR* envp[ ] ) {
	int nRetCode = 0;
	if ( argc < 2 ) {
		std::wcerr << L"Need more than 1 argument!" << std::endl;
		Sleep( 5000 );
		return -1;
		}

	HMODULE hModule = ::GetModuleHandle( NULL );

	ULONG_PTR minCache;
	ULONG_PTR maxCache;
	DWORD cacheVars;

	if ( !GetSystemFileCacheSize( &minCache, &maxCache, &cacheVars ) ) {
		std::wcerr << L"Failed to get system file cache size!" << std::endl;
		auto lastError = GetLastError( );
		std::wcerr << L"GetLastError: " << lastError << std::endl;
		std::wcerr << GetLastErrorStdStr( lastError ) << std::endl;
		return int( lastError );
		}
	std::wcout << TRACE_OUT( minCache ) << TRACE_OUT( maxCache ) << TRACE_OUT( cacheVars ) << std::endl;
	const auto Starting_minCache = minCache;
	const auto Starting_maxCache = maxCache;
	const auto Starting_cacheVars = cacheVars;

	HANDLE hToken = NULL;

	OpenProcessToken( GetCurrentProcess( ), TOKEN_ADJUST_PRIVILEGES, &hToken );
	SetPrivilege( hToken, SE_INCREASE_QUOTA_NAME, true );
	CloseHandle( hToken );
	FlushCache( );
	//experimental results indicate that ( 16 * 1024 * 1024 ) is the smallest possible value accepted by SetSystemFileCache.
	auto res = SetSystemFileCacheSize( 0, ( 16 * 1024 * 1024 ), FILE_CACHE_MAX_HARD_ENABLE );
	if ( res == 0 ) {
		std::wcerr << L"Failed to set system file cache size to 0!" << std::endl;
		auto lastError = GetLastError( );
		std::wcerr << L"GetLastError: " << lastError << std::endl;
		std::wcerr << GetLastErrorStdStr( lastError ) << std::endl;
		return res;
		}
	FlushCache( );

	if ( !GetSystemFileCacheSize( &minCache, &maxCache, &cacheVars ) ) {
		std::wcerr << L"Failed to get system file cache size!" << std::endl;
		auto lastError = GetLastError( );
		std::wcerr << L"GetLastError: " << lastError << std::endl;
		std::wcerr << GetLastErrorStdStr( lastError ) << std::endl;
		return int( lastError );
		}
	std::wcout << TRACE_OUT( minCache ) << TRACE_OUT( maxCache ) << TRACE_OUT( cacheVars ) << std::endl;


	try {
		std::wstring arg = argv[ 1 ];
		arg.reserve( MAX_PATH );
		std::wstringstream ss;
		std::vector<FileFindRecord> records;
		if ( hModule != NULL ) {
			if ( !AfxWinInit( hModule, NULL, ::GetCommandLine( ), 0 ) ) {
				_tprintf( _T( "Fatal Error: MFC initialization failed\n" ) );
				nRetCode = 1;
				}
			else {
				//wrapCFileFind( arg.c_str( ), ss );
				for ( int i = 0; i < 2; ++i ) {
					std::wcout << TRACE_OUT( i ) << std::endl;

					records.emplace_back( iterate( arg, true, true, ss, false )  );
					records.emplace_back( iterate( arg, false, true, ss, false ) );

					records.emplace_back( iterate( arg, true, false, ss, false ) );
					records.emplace_back( iterate( arg, false, false, ss, false ));

					records.emplace_back( iterate( arg, true, true, ss, true ) );
					records.emplace_back( iterate( arg, false, true, ss, true ) );

					records.emplace_back( iterate( arg, true, false, ss, true ) );
					records.emplace_back( iterate( arg, false, false, ss, true ) );
					}
				}
			//wrapCFileFind( arg.c_str( ), ss );
			std::wcout << ss.str( ) << std::endl;
			std::wcout << L"---------------------" << std::endl;
			std::sort( records.begin( ), records.end( ) );
			for ( const auto& record : records ) {
				std::wcout << formatFileFindRecord( record ) << std::endl;
				}


			}
		else {
			nRetCode = 1;
			}
		}
	catch ( std::exception& e ) {
		std::cout << e.what( ) << std::endl;
		goto cleanup;
		}
cleanup://If we've made any changes, revert them
	if ( !SetSystemFileCacheSize( Starting_minCache, Starting_maxCache, 0 ) ) {
		std::wcout << L"Error resetting cache size!" << std::endl;
		auto LastError = GetLastError( );
		std::wcerr << L"Last error: " << LastError << std::endl;
		std::wcerr << GetLastErrorStdStr( LastError ) << std::endl;
		return int( LastError );
		}

	if ( !GetSystemFileCacheSize( &minCache, &maxCache, &cacheVars ) ) {
		std::wcerr << L"Failed to get system file cache size!" << std::endl;
		auto lastError = GetLastError( );
		std::wcerr << L"GetLastError: " << lastError << std::endl;
		std::wcerr << GetLastErrorStdStr( lastError ) << std::endl;
		return int( lastError );
		}
	std::wcout << TRACE_OUT( minCache ) << TRACE_OUT( maxCache ) << TRACE_OUT( cacheVars ) << std::endl;


	return nRetCode;
	}
