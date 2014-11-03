// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma warning(disable:4061) //enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label. The enumerate has no associated handler in a switch statement.
#pragma warning(disable:4062) //The enumerate has no associated handler in a switch statement, and there is no default label.
#pragma warning(disable:4191) //'operator/operation' : unsafe conversion from 'type of expression' to 'type required'
#pragma warning(disable:4265) //'class' : class has virtual functions, but destructor is not virtual
#pragma warning(disable:4350) //An rvalue cannot be bound to a non-const reference. In previous versions of Visual C++, it was possible to bind an rvalue to a non-const reference in a direct initialization. This code now gives a warning.
//#pragma warning(disable:4365) //'action' : conversion from 'type_1' to 'type_2', signed/unsigned mismatch

#ifndef DUMP_MEMUSAGE
#pragma warning(disable:4820) //'bytes' bytes padding added after construct 'member_name'. The type and order of elements caused the compiler to add padding to the end of a struct
#endif

#pragma warning(disable:4917) //'declarator' : a GUID can only be associated with a class, interface or namespace. A user-defined structure other than class, interface, or namespace cannot have a GUID.
#pragma warning(disable:4987) //nonstandard extension used: 'throw (...)'

//noisy
//#pragma warning(disable:4127) //The controlling expression of an if statement or while loop evaluates to a constant.

#pragma warning(disable:4548) //expression before comma has no effect; expected expression with side-effect
#pragma warning(disable:4625) //A copy constructor was not accessible in a base class, therefore not generated for a derived class. Any attempt to copy an object of this type will cause a compiler error. //ANYTHING that inherits from CWND will warn!
#pragma warning(disable:4626) //An assignment operator was not accessible in a base class and was therefore not generated for a derived class. Any attempt to assign objects of this type will cause a compiler error.
//#pragma warning(disable:4755) //Conversion rules for arithmetic operations in the comparison mean that one branch cannot be executed in an inlined function. Cast '(nBaseTypeCharLen + ...)' to 'ULONG64' (or similar type of 8 bytes).
//#pragma warning(disable:4280) //'operator –>' was self recursive through type 'type'. Your code incorrectly allows operator–> to call itself.
#pragma warning(disable:4264) //'virtual_function' : no override available for virtual member function from base 'class'; function is hidden
//#pragma warning(disable:4263) //A class function definition has the same name as a virtual function in a base class but not the same number or type of arguments. This effectively hides the virtual function in the base class.
//#pragma warning(disable:4189) //A variable is declared and initialized but not used.

#pragma warning(disable:4514) //'function' : unreferenced inline function has been removed
#pragma warning(disable:4710) //The given function was selected for inline expansion, but the compiler did not perform the inlining.
#pragma warning(disable:4711) //function 'function' selected for inline expansion. The compiler performed inlining on the given function, although it was not marked for inlining.


#pragma warning(push, 1)

#define _WIN32_WINNT 0x0600

#include <afx.h>
#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#define _AFX_NO_MFC_CONTROLS_IN_DIALOGS         // remove support for MFC controls in dialogs

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#ifndef _DEBUG
#pragma warning(disable:4555) //expression has no effect; expected expression with side-effect //Happens alot with AfxCheckMemory in debug builds.
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <iostream>
#include <ostream>
#include <string>
#include <sstream>
#include <strsafe.h>
#include <vector>
#include <cstdint>
#include <memory>
#include <ios>
#include <exception>
#include <future>
#include <queue>
#pragma warning(pop)

struct minimalFindData {
	minimalFindData( );
	minimalFindData( std::wstring inS, DWORD inA, DWORD inFSH, DWORD inFSL );
	minimalFindData( minimalFindData&& in );
	std::wstring fileName;
	DWORD attributes;
	DWORD fileSizeHigh;
	DWORD fileSizeLow;

	};

struct FILEINFO {
	CString      name;
	DWORD        attributes;
	};

class CFileFindMod : public CFileFind {
	public:
	DWORD GetAttributes( ) const;//brilliant idea I got from WinDirStat!
	};

std::int64_t stdRecurseFind( _In_ std::wstring dir, _In_ bool isLargeFetch, _In_ bool isBasicInfo );
std::int64_t stdRecurseFindFutures( _In_ std::wstring dir, _In_ const bool isLargeFetch, _In_ const bool isBasicInfo );
// TODO: reference additional headers your program requires here
