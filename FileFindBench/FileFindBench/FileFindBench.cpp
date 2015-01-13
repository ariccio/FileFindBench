#include "stdafx.h"
#include "FileFindBench.h"


#ifdef MFC_C_FILE_FIND_TEST
CWinApp theApp;
#endif

#define BASE 1024
#define HALF_BASE BASE/2

//#define TRACE_OUT(x) std::endl << L"\t\t" << #x << L" = `" << x << L"` "//awesomely useful macro, included now, just in case I need it later.

#define TRACE_OUT_C_STYLE( x, fmt_spec ) wprintf( L"\r\n\t\t" L#x L" = `" L#fmt_spec L"` ", ##x )
#define TRACE_OUT_C_STYLE_ENDL( ) wprintf( L"\r\n" )



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
std::wstring GetLastErrorStdStr( _In_ DWORD error ) {
	if ( error ) {
		LPVOID lpMsgBuf;
		DWORD bufLen = FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), reinterpret_cast<LPTSTR>( &lpMsgBuf ), 0, NULL );
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

	if ( !LookupPrivilegeValueW( NULL, lpszPrivilege, &luid ) ) {
		wprintf( L"LookupPrivilegeValue error: %u\n", GetLastError( ) );
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

	if ( !AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( TOKEN_PRIVILEGES ), static_cast<PTOKEN_PRIVILEGES>( NULL ), static_cast<PDWORD>( NULL ) ) ) {
		wprintf( L"AdjustTokenPrivileges error: %u\n", GetLastError( ) );
		return FALSE;
		}

	if ( GetLastError( ) == ERROR_NOT_ALL_ASSIGNED ) {
		wprintf( L"The token does not have the specified privilege. \n" );
		return FALSE;
		}
	return TRUE;
	}

_Success_( return == true )
bool FlushCache( ) {
	//http://msdn.microsoft.com/en-us/library/windows/desktop/aa965240(v=vs.85).aspx
	//"To flush the cache, specify (SIZE_T) -1."
	wprintf( L"Flushing cache...\r\n" );
	const BOOL res = SetSystemFileCacheSize( static_cast<SIZE_T>( -1 ), static_cast<SIZE_T>( -1 ), 0 );
	if ( !res ) {
		const auto LastError = GetLastError( );
		fwprintf( stderr, L"Last error: %lu\r\n", LastError );
		fwprintf( stderr, L"%s\r\n", GetLastErrorStdStr( LastError ).c_str( ) );
		return false;
		}
	return true;
	}

std::int64_t descendDirectory( _In_ WIN32_FIND_DATA& fData, _In_ const std::wstring& normSzDir, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo, _In_ const bool futures = false ) {
	std::wstring newSzDir( normSzDir );//MUST operate on copy!
	newSzDir.reserve( MAX_PATH );
	newSzDir += L"\\";
	newSzDir += fData.cFileName;
	std::int64_t num = 0;
	if ( futures ) {
		num += stdRecurseFindFutures( std::move( newSzDir ), isLargeFetch, isBasicInfo );
		}
	else {
		num += stdRecurseFind( std::move( newSzDir ), isLargeFetch, isBasicInfo );
		}
	return num;
	}

//void trace_wcout_bad( ) {
//	if ( !std::wcout.good( ) ) {//Slower than it should be.
//		auto badBits    = std::wcout.exceptions( );
//		auto wasBadBit  = badBits & std::ios_base::badbit;
//		auto wasFailBit = badBits & std::ios_base::failbit;
//		auto wasEofBit  = badBits & std::ios_base::eofbit;
//		std::wcout.clear( );
//		std::wcout << L"wcout was in a bad state!" << std::endl;
//		std::wcout << TRACE_OUT( wasBadBit );
//		std::wcout << TRACE_OUT( wasFailBit );
//		std::wcout << TRACE_OUT( wasEofBit );
//		std::wcout << std::endl;
//		}
//	}

void trace_fDataBits( _In_ const WIN32_FIND_DATA& fData, _In_ const std::wstring& normSzDir ) {
	if ( !( ( fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA ) ) ) {
		//TOO MANY DAMN HIDDEN FILES!
		wprintf( L"\r\n\tWeird file encountered in %s\r\n\tWeird file attributes:", normSzDir.c_str( ) );

		//std::wcout << TRACE_OUT( fData.cFileName ) << std::endl;
		TRACE_OUT_C_STYLE( fData.cFileName, %s );
		TRACE_OUT_C_STYLE_ENDL( );

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE              ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE              ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED           ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED           ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM    ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM    ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA       ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA       ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE             ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE             ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT       ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT       ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE         ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE         ) );
			}

		if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL             ) ) {
			TRACE_OUT_C_STYLE( ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL ), %lu );
			//std::wcout << TRACE_OUT( ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL             ) );
			}
			//std::wcout << std::endl;
		}
	}

HANDLE call_find_first_file_ex( _In_ const std::wstring dir, _Out_ WIN32_FIND_DATA& fData, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo ) {
	if ( isLargeFetch ) {
		if ( isBasicInfo ) {
			return FindFirstFileExW( dir.c_str( ), FindExInfoBasic,    &fData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH );
			}
		else {
			return FindFirstFileExW( dir.c_str( ), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH );
			}
		}
	else {
		if ( isBasicInfo ) {
			return FindFirstFileExW( dir.c_str( ), FindExInfoBasic,    &fData, FindExSearchNameMatch, NULL, 0 );
			}
		else {
			return FindFirstFileExW( dir.c_str( ), FindExInfoStandard, &fData, FindExSearchNameMatch, NULL, 0 );
			}
		}
	}


std::int64_t stdRecurseFind( _In_ std::wstring dir, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo ) {

	std::int64_t num = 0;
	dir.reserve( MAX_PATH );
	std::wstring normSzDir(dir);
	assert( dir.size( ) > 2 );
	normSzDir.reserve( MAX_PATH );
	if ( ( dir[ dir.length( ) - 1 ] != L'*' ) && ( dir[ dir.length( ) - 2 ] != L'\\' ) ) {
		dir += L"\\*";
		}
	else if ( dir[ dir.length( ) - 1 ] == L'\\' ) {
		dir += L"*";
		}

	WIN32_FIND_DATA fData;

	HANDLE fDataHand = call_find_first_file_ex( std::move( dir ), fData, isLargeFetch, isBasicInfo );
	if ( fDataHand != INVALID_HANDLE_VALUE ) {
		if ( !( wcscmp( fData.cFileName, L".." ) == 0 ) ) {
			//++num;
			}
		auto res = FindNextFileW( fDataHand, &fData );
		while ( ( fDataHand != INVALID_HANDLE_VALUE ) && ( res != 0 ) ) {
			//trace_wcout_bad( );
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
	std::wstring normSzDir( dir );
	normSzDir.reserve( MAX_PATH );
	assert( dir.size( ) > 2 );
	if ( ( dir[ dir.length( ) - 1 ] != L'*' ) && ( dir[ dir.length( ) - 2 ] != L'\\' ) ) {
		dir += L"\\*";
		}
	else if ( dir[ dir.length( ) - 1 ] == L'\\' ) {
		dir += L"*";
		}
	std::vector<std::future<std::int64_t>> futureDirs;
	futureDirs.reserve( 100 );//pseudo-arbitrary number
	WIN32_FIND_DATA fData;
	HANDLE fDataHand = call_find_first_file_ex( std::move( dir ), fData, isLargeFetch, isBasicInfo );

	if ( fDataHand != INVALID_HANDLE_VALUE ) {
		if ( !( wcscmp( fData.cFileName, L".." ) == 0 ) ) {
			//++num;
			}
		auto res = FindNextFileW( fDataHand, &fData );
		while ( ( fDataHand != INVALID_HANDLE_VALUE ) && ( res != 0 ) ) {
			//trace_wcout_bad( );
			const auto scmpVal = wcscmp( fData.cFileName, L".." );
			if ( ( !( scmpVal == 0 ) ) ) {
				++num;
				}
			if ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ( scmpVal != 0 ) ) {
				futureDirs.emplace_back( std::move( std::async( std::launch::async|std::launch::deferred, descendDirectory, fData, normSzDir, isLargeFetch, isBasicInfo, true ) ) );
				}
			else if ( ( scmpVal != 0 ) && ( ( ( fData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_INTEGRITY_STREAM ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_NO_SCRUB_DATA ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) || ( fData.dwFileAttributes & FILE_ATTRIBUTE_VIRTUAL ) ) ) ) {
				
				trace_fDataBits( fData, normSzDir );
				--num;
				}
			res = FindNextFileW( fDataHand, &fData );
			}
		}
	const auto size_futureDirs = futureDirs.size( );
	for ( size_t i = 0; i < size_futureDirs; ++i ) {
		num += futureDirs[ i ].get( );
		}
	FindClose( fDataHand );
	return num;
	}


void stdWork( _In_ std::wstring arg, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo ) {
	wprintf( L"Working on: `%s`\r\n", arg.c_str( ) );
	std::int64_t numberFiles = 0;
	arg.reserve( MAX_PATH );
	if ( arg.length( ) > 3 ) {
		const auto strCmp = ( arg.compare( 0, 4, arg, 0, 4 ) );
		if ( strCmp != 0 ) {
			arg = L"\\\\?\\" + arg;
			wprintf( L"prefixed `%s`, value now: `%s`\r\n\r\n", arg.c_str( ), arg.c_str( ) );
			}
		}
	else {
		arg = L"\\\\?\\" + arg;
		wprintf( L"prefixed `%s`, value now: `%s`\r\n\r\n", arg.c_str( ), arg.c_str( ) );
		}
	numberFiles = stdRecurseFind( arg, isLargeFetch, isBasicInfo );
	wprintf( L"\r\nNumber of items: %I64d\r\n", numberFiles );
	}

void stdWorkAsync( _In_ std::wstring arg, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo ) {
	wprintf( L"Working on: `%s`\r\n", arg.c_str( ) );
	std::int64_t numberFiles = 0;
	arg.reserve( MAX_PATH );
	if ( arg.length( ) > 3 ) {
		const auto strCmp = ( arg.compare( 0, 4, arg, 0, 4 ) );
		if ( strCmp != 0 ) {
			arg = L"\\\\?\\" + arg;
			wprintf( L"prefixed `%s`, value now: `%s`\r\n\r\n", arg.c_str( ), arg.c_str( ) );
			}
		}
	else {
		arg = L"\\\\?\\" + arg;
		wprintf( L"prefixed `%s`, value now: `%s`\r\n\r\n", arg.c_str( ), arg.c_str( ) );
		}
	numberFiles = stdRecurseFindFutures( std::move( arg ), isLargeFetch, isBasicInfo );
	wprintf( L"\r\nNumber of items: %I64d\r\n", numberFiles );
	}


const DOUBLE getAdjustedTimingFrequency( ) {
	LARGE_INTEGER timingFrequency;
	const BOOL res1 = QueryPerformanceFrequency( &timingFrequency );
	if ( !res1 ) {
		wprintf( L"QueryPerformanceFrequency failed!!!!!! Disregard any timing data!!\r\n" );
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


FileFindRecord iterate( _In_ const std::wstring& arg, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo, _In_ const bool IsAsync ) {
	wprintf( L"--------------------------------------------\r\n" );
	FlushCache( );


	LARGE_INTEGER startTime;
	LARGE_INTEGER endTime;
	
	//std::int64_t fileSizeTotal = 0;
	const auto adjustedTimingFrequency = getAdjustedTimingFrequency( );

	const BOOL res2 = QueryPerformanceCounter( &startTime );
	if ( IsAsync ) {
		stdWorkAsync( arg, isLargeFetch, isBasicInfo );
		}
	else {
		stdWork( arg, isLargeFetch, isBasicInfo );
		}
	const BOOL res3 = QueryPerformanceCounter( &endTime );
	
	if ( ( !res2 ) || ( !res3 ) ) {
		wprintf( L"QueryPerformanceCounter failed!!!!!! Disregard any timing data!!\r\n" );
		}

	const auto totalTime = ( endTime.QuadPart - startTime.QuadPart ) * adjustedTimingFrequency;
	//times.emplace_back( totalTime );
	//ss << L"Time in seconds:  " << totalTime << std::endl;

	FileFindRecord record;
	record.is_Async = IsAsync;
	record.is_BasicInfo = isBasicInfo;
	record.is_FIND_FIRST_EX_LARGE_FETCH = isLargeFetch;
	record.time_seconds = totalTime;
	FlushCache( );
	wprintf( L"\r\n" );
	Sleep( 500 );
	return record;
	}

#ifdef MFC_C_FILE_FIND_TEST
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
#endif




void stats( std::vector<FileFindRecord>& records ) {
	std::vector<FileFindRecord> basic_notlargefetch_notasync;
	std::vector<FileFindRecord> basic_largefetch_notasync;
	std::vector<FileFindRecord> full_notlargefetch_notasync;
	std::vector<FileFindRecord> full_largefetch_notasync;
	

	std::vector<FileFindRecord> basic_notlargefetch_async;
	std::vector<FileFindRecord> basic_largefetch_async;
	std::vector<FileFindRecord> full_largefetch_async;
	std::vector<FileFindRecord> full_notlargefetch_async;
	
	const auto size_records = records.size( );
	for ( size_t i = 0; i < size_records; ++i ) {
		if ( records[ i ].is_BasicInfo ) {
			if ( records[ i ].is_FIND_FIRST_EX_LARGE_FETCH ) {
				if ( records[ i ].is_Async ) {
					basic_largefetch_async.emplace_back( std::move( records[ i ] ) );
					}
				else {
					basic_largefetch_notasync.emplace_back( std::move( records[ i ] ) );
					}
				}
			else {
				if ( records[ i ].is_Async ) {
					basic_notlargefetch_async.emplace_back( std::move( records[ i ] ) );
					}
				else {
					basic_notlargefetch_notasync.emplace_back( std::move( records[ i ] ) );
					}
				}
			}
		else {
			if ( records[ i ].is_FIND_FIRST_EX_LARGE_FETCH ) {
				if ( records[ i ].is_Async ) {
					full_largefetch_async.emplace_back( std::move( records[ i ] ) );
					}
				else {
					full_largefetch_notasync.emplace_back( std::move( records[ i ] ) );
					}
				}
			else {
				if ( records[ i ].is_Async ) {
					full_notlargefetch_async.emplace_back( std::move( records[ i ] ) );
					}
				else {
					full_notlargefetch_notasync.emplace_back( std::move( records[ i ] ) );
					}
				}
			}
		}
	}


int wmain( int argc, _In_reads_( argc ) _Readable_elements_( argc ) WCHAR* argv[ ], WCHAR* envp[ ] ) {
	int nRetCode = 0;
	if ( argc < 2 ) {
		wprintf( L"Need more than 1 argument!\r\n" );
		Sleep( 5000 );
		return ERROR_BAD_ARGUMENTS;
		}

	HMODULE hModule = ::GetModuleHandleW( NULL );

	ULONG_PTR minCache = 0;
	ULONG_PTR maxCache = 0;
	DWORD cacheVars = 0;

	if ( hModule == NULL ) {
		fwprintf( stderr, L"Couldn't get module handle!\r\n" );
		return ERROR_MOD_NOT_FOUND;
		}

#ifdef MFC_C_FILE_FIND_TEST
	if ( !AfxWinInit( hModule, NULL, ::GetCommandLine( ), 0 ) ) {
		fwprintf( stderr, L"Fatal Error: MFC initialization failed!\r\n" );
		return ERROR_APP_INIT_FAILURE;
		}
#endif

	if ( !GetSystemFileCacheSize( &minCache, &maxCache, &cacheVars ) ) {
		fwprintf( stderr, L"Failed to get system file cache size!\r\n" );
		auto lastError = GetLastError( );
		fwprintf( stderr, L"GetLastError: %lu\r\n", lastError );
		fwprintf( stderr, L"%s\r\n", GetLastErrorStdStr( lastError ).c_str( ) );
		return int( lastError );
		}

	TRACE_OUT_C_STYLE( minCache, %llu );
	TRACE_OUT_C_STYLE( maxCache, %llu );
	TRACE_OUT_C_STYLE( cacheVars, %lu );
	TRACE_OUT_C_STYLE_ENDL( );

	const auto Starting_minCache = minCache;
	const auto Starting_maxCache = maxCache;
	const auto Starting_cacheVars = cacheVars;

	HANDLE hToken = NULL;

	const BOOL proc_result = OpenProcessToken( GetCurrentProcess( ), TOKEN_ADJUST_PRIVILEGES, &hToken );
	if ( !proc_result ) {
		fwprintf( stderr, L"OpenProcessToken( GetCurrentProcess( ), TOKEN_ADJUST_PRIVILEGES, &hToken ) failed!!\r\n" );
		return -1;
		}
	const BOOL priv_result = SetPrivilege( hToken, SE_INCREASE_QUOTA_NAME, true );
	if ( !priv_result ) {
		fwprintf( stderr, L"SetPrivilege( hToken, SE_INCREASE_QUOTA_NAME, true ) failed!!\r\n" );
		return -2;

		}
	const BOOL hand_close_result = CloseHandle( hToken );
	if ( !hand_close_result ) {
		fwprintf( stderr, L"CloseHandle( hToken ) failed!!\r\n" );
		return -3;
		}
	//FlushCache( );
	//experimental results indicate that ( 16 * 1024 * 1024 ) is the smallest possible value accepted by SetSystemFileCache.
	const BOOL set_cache_result = SetSystemFileCacheSize( 0, ( 16 * 1024 * 1024 ), FILE_CACHE_MAX_HARD_ENABLE );
	if ( !set_cache_result ) {
		fwprintf( stderr, L"Failed to set system file cache size to 0!\r\n" );
		const auto lastError = GetLastError( );
		fwprintf( stderr, L"GetLastError: %lu\r\n", lastError );
		fwprintf( stderr, L"%s\r\n", GetLastErrorStdStr( lastError ).c_str( ) );
		return set_cache_result;
		}
	const BOOL flush_result = FlushCache( );
	if ( !flush_result ) {
		goto cleanup;
		}

	const BOOL get_cache_size = GetSystemFileCacheSize( &minCache, &maxCache, &cacheVars );

	if ( !get_cache_size ) {
		fwprintf( stderr, L"Failed to get system file cache size!\r\n" );
		const auto lastError = GetLastError( );
		fwprintf( stderr, L"GetLastError: %lu\r\n", lastError );
		fwprintf( stderr, L"%s\r\n", GetLastErrorStdStr( lastError ).c_str( ) );
		goto cleanup;
		}


	TRACE_OUT_C_STYLE( minCache, %llu );
	TRACE_OUT_C_STYLE( maxCache, %llu );
	TRACE_OUT_C_STYLE( cacheVars, %lu );
	TRACE_OUT_C_STYLE_ENDL( );

	//std::wcout << TRACE_OUT( minCache ) << TRACE_OUT( maxCache ) << TRACE_OUT( cacheVars ) << std::endl;


	try {
		std::wstring arg = argv[ 1 ];
		arg.reserve( MAX_PATH );
		//std::wstringstream ss;
		std::vector<FileFindRecord> records;
		records.reserve( 17 );
		if ( hModule != NULL ) {

			//wrapCFileFind( arg.c_str( ), ss );
			for ( int i = 0; i < 2; ++i ) {
				TRACE_OUT_C_STYLE( i, %i );
				TRACE_OUT_C_STYLE_ENDL( );
				//std::wcout << TRACE_OUT( i ) << std::endl;


				//records.emplace_back( iterate( arg, false, true, true ) );
				//records.emplace_back( iterate( arg, false, true, true ) );
				//records.emplace_back( iterate( arg, false, true, true ) );
				//records.emplace_back( iterate( arg, false, true, true ) );

				//----

				//records.emplace_back( iterate( arg, true, true, false )  );
				//records.emplace_back( iterate( arg, false, true, false ) );

				//records.emplace_back( iterate( arg, true, false, false ) );
				//records.emplace_back( iterate( arg, false, false, false ));

				records.emplace_back( iterate( arg, true, true, true ) );
				records.emplace_back( iterate( arg, false, true, true ) );

				records.emplace_back( iterate( arg, true, false, true ) );
				records.emplace_back( iterate( arg, false, false, true ) );
				}


			//wrapCFileFind( arg.c_str( ), ss );
			wprintf( L"---------------------\r\n" );
			const auto size_records = records.size( );
			if ( size_records > 1 ) {
				std::sort( &( records[ 0 ] ), &( records[ size_records - 1 ] ) );
				}
			for ( size_t i = 0; i < size_records; ++i ) {
				wprintf( L"%s\r\n", formatFileFindRecord( records[ i ] ).c_str( ) );
				}
			//stats( records );
			}
		else {
			nRetCode = 1;
			}
		}
	catch ( std::exception& e ) {
		fprintf( stderr, "%s\r\n", e.what( ) );
		goto cleanup;
		}
cleanup://If we've made any changes, revert them
	if ( !SetSystemFileCacheSize( Starting_minCache, Starting_maxCache, 0 ) ) {
		fwprintf( stderr, L"Error resetting cache size!\r\n" );
		const auto lastError = GetLastError( );
		fwprintf( stderr, L"GetLastError: %lu\r\n", lastError );
		wprintf( L"%s\r\n", GetLastErrorStdStr( lastError ).c_str( ) );
		return int( lastError );
		}

	if ( !GetSystemFileCacheSize( &minCache, &maxCache, &cacheVars ) ) {
		fwprintf( stderr, L"Failed to get system file cache size!\r\n" );
		const auto lastError = GetLastError( );
		fwprintf( stderr, L"GetLastError: %lu\r\n", lastError );
		fwprintf( stderr, L"%s\r\n", GetLastErrorStdStr( lastError ).c_str( ) );
		return int( lastError );
		}

	TRACE_OUT_C_STYLE( minCache, %llu );
	TRACE_OUT_C_STYLE( maxCache, %llu );
	TRACE_OUT_C_STYLE( cacheVars, %lu );
	TRACE_OUT_C_STYLE_ENDL( );

	//stdRecurseFindFutures( L"", false, false );
	return nRetCode;
	}
