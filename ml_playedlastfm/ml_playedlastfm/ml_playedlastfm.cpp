// This is the main DLL file.

#include "stdafx.h"

#include "Windows.h"
#include "Wininet.h"
#include "Strsafe.h"

#include "wa_ipc.h"
#include "ml.h"

#include "tinyxml2.h"

#include "ml_playedlastfm.h"

#include "playedLastFmOutput.h"

winampMediaLibraryPlugin PlayedLastFm =
{
	MLHDR_VER,
	PLUGIN_NAME,
	init, // return 0 on success, non-zero for failure (quit WON'T be called)
	quit,
	MessageProc,
	//all the following data is filled in by the library
	0, // hwndWinampParent
	0, // hwndLibraryParent -- send this any of the WM_ML_IPC messages
	0, // hDllInstance
};

// event functions follow

// Reads config values from ini file
int init()
{
	MessageBox( PlayedLastFm.hwndWinampParent, L"PlayedLastFm!", L"", MB_OK );
	quitThread = false;
	output = new playedLastFmOutput( PlayedLastFm.hwndWinampParent );
	threadHandle = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size
		PlayedLastFmThread,     // thread function name
		NULL,                   // argument to thread function
		0,                      // use default creation flags
		&threadId);             // returns the thread identifier

	return 0;
}

// Writes values back to ini file
void quit()
{
	output->writeMessage( L"Setting quitThread true." );
	quitThread = true;
	output->writeMessage( L"Waiting for thread." );
	DWORD waitReturn = WaitForSingleObject( threadHandle, 1000 );
	switch ( waitReturn )
	{
	case WAIT_FAILED:
		output->writeMessage( L"Failed waiting for thread." );
		break;
	case WAIT_OBJECT_0:
		output->writeMessage( L"The state of the specified object is signaled." );
		CloseHandle( threadHandle );
		break;
	case WAIT_TIMEOUT:
		output->writeMessage( L"The time-out interval elapsed, and the object's state is nonsignaled." );
		break;
	default:
		output->writeMessage( L"Unknown return code." );
		break;
	}
	output->writeMessage( L"Returning from quit().\n" );
	delete output;
}

// Receives messages from Winamp -- this is the primary way to do IO
// return NONZERO if you accept this message as yours, otherwise 0 to pass it
// to other plugins
INT_PTR MessageProc( int message_type, INT_PTR param1, INT_PTR param2, INT_PTR param3 )
{
	wchar_t msg[1024];
	wsprintf( msg, L"MessageProc called with message type %x, param1 %x, param2 %x, param3 %x",
		message_type,
		param1,
		param2,
		param3
		);
	//output->writeMessage( msg );
	return NULL;
}

// This is an export function called by winamp which returns this plugin info.
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
extern "C" __declspec(dllexport) winampMediaLibraryPlugin *winampGetMediaLibraryPlugin()
{
	return &PlayedLastFm;
}

// plugin-specific functions follow

DWORD WINAPI PlayedLastFmThread( LPVOID lpParam )
{
	const size_t kPathLen = 512;
	wchar_t iniPath[kPathLen];
	getInitFileName( iniPath, kPathLen );
	output->writeMessage( iniPath );
	GetPrivateProfileString( L"playedlastfm", L"Username", NULL, lastFmUsername, 256, iniPath );
	output->writeMessage( lastFmUsername );
	lastSyncTime = GetPrivateProfileInt( L"playedlastfm", L"lastsync", 0, iniPath );
	wchar_t msg[256];
	wsprintf( msg, L"Last sync time is %d", lastSyncTime );
	output->writeMessage( msg );
	syncInterval = SYNC_INTERVAL;

	while ( !quitThread )
	{
		currentSyncTime = getCurrentTime();
		if ( currentSyncTime > lastSyncTime + syncInterval )
		{
			performLastFmSync();
			lastSyncTime = currentSyncTime;
		}
		Sleep( 100 ); // one tenth of a second -- want to respond quickly to quitThread becoming true
	}

	output->writeMessage( iniPath );
	//WritePrivateProfileString( L"playedlastfm", L"Username", lastFmUsername, iniPath );
	output->writeMessage( lastFmUsername );
	wchar_t syncTimeStr[64];
	wsprintf( syncTimeStr, L"%d", lastSyncTime );
	//WritePrivateProfileString( L"playedlastfm", L"lastsync", syncTimeStr, iniPath );
	output->writeMessage( syncTimeStr );

	return 0;
}

void getInitFileName( wchar_t* filename, size_t numChars )
{
	char *dir = (char*)SendMessage( PlayedLastFm.hwndWinampParent, WM_WA_IPC, 0, IPC_GETINIDIRECTORY );
	// Convert to a wchar_t*
	size_t origsize = strlen( dir ) + 1;
	if ( origsize <= numChars )
	{
		size_t convertedChars = 0;
		mbstowcs_s( &convertedChars, filename, origsize, dir, _TRUNCATE );
		wcscat_s( filename, numChars, INIFILENAME );
	}
}

// Converts a Windows system time into a Unix epoch time stored in a time_t
time_t getCurrentTime()
{
	SYSTEMTIME currentSystemTime;
	FILETIME   currentFileTime;
	GetSystemTime( &currentSystemTime );
	SystemTimeToFileTime( &currentSystemTime, &currentFileTime );
	ULARGE_INTEGER windowsTicks;
	windowsTicks.LowPart = currentFileTime.dwLowDateTime;
	windowsTicks.HighPart = currentFileTime.dwHighDateTime;

	return ( windowsTicks.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH );
}

void performLastFmSync()
{
	//1380998600
	//1380998800

	int numTracks = 0;

	// request 1 track in timespan
	// use that result to get number of pages
	if ( getNumTracks( &numTracks ) )
	{
		numPages = ( numTracks / TRACKS_PER_PAGE ) + ( numTracks % TRACKS_PER_PAGE > 0 ? 1 : 0 );

		wchar_t syncMessage[1024];
		wsprintf( syncMessage, L"Found %d tracks, on %d pages.", numTracks, numPages );
		output->writeMessage( syncMessage );

		// for each page i from n...i...1
		for ( int page = numPages; numPages > 0; --numPages )
		{
			//   check if we should quit
			if ( quitThread )
			{
				wsprintf( syncMessage, L"Quit was set before processing page %d of %d!", page, numPages );
				output->writeMessage( syncMessage );

				break;
			}

			wsprintf( syncMessage, L"Processing page %d of %d.", page, numPages );
			output->writeMessage( syncMessage );

			//   request page i
			//   for each track in page
			if ( getTracksPage( page ) )
			{
				//     query from media library 
				//     update timestamp
				//     ++numplays
				//     if last track in page
				//       update lastSyncTime
			}
		}
	}
}

bool getNumTracks( int* numTracks )
{
	bool returnVal = false;
	HINTERNET hSession = InternetOpen( USER_AGENT, 0, NULL, NULL, 0 );

	if ( hSession != NULL )
	{
		wchar_t req[1024];
		wchar_t currentSyncString[256];
		StringCbPrintfW( req, 1024, REQUEST_STRING, lastFmUsername, API_KEY, 1, lastSyncTime ); // get first page of results
		// It didn't like adding currentSyncTime to the printf for some reason,
		// so we'll cat currentSyncString onto the end of req
		StringCbPrintfW( currentSyncString, 256, L"%ld", currentSyncTime ); 
		wcscat_s( req, currentSyncString );
		HINTERNET hOpenUrl = InternetOpenUrl( hSession, req, NULL, 0, 1, 1 );

		if ( hOpenUrl == NULL )
		{
			output->writeMessage( L"Could not access URL!" );
		}
		else
		{
			output->writeMessage( L"Accessed URL!" );

			DWORD totalBytes = 0;
			FILE * tempFile;

			if ( ( tempFile = _wfopen( TEMP_FILE_PATH, L"wb" ) ) )
			{
				const int kBufSize = 256 * 1000;
				char buffer[kBufSize];
				DWORD bytesRead = 0;

				while ( InternetReadFile( hOpenUrl, buffer, kBufSize, &bytesRead )  )
				{
					if ( bytesRead == 0 )
					{
						output->writeMessage( L"Reached end of file!" );
						break;
					}

					fwrite( buffer, sizeof( char ), bytesRead, tempFile );
					totalBytes += bytesRead;
					wchar_t writeMessage[256];
					wsprintf( writeMessage, L"Wrote %d bytes to temp file", bytesRead );
					output->writeMessage( writeMessage );

				}

				fclose( tempFile );

				wchar_t closeMessage[256];
				wsprintf( closeMessage, L"Closed temp file, wrote %d bytes", totalBytes );
				output->writeMessage( closeMessage );

				if ( parseTempFile( numTracks ) )
				{
					returnVal = true;
				}
			}
			else
			{
				output->writeMessage( L"Couldn't open temp file!" );
			}

			InternetCloseHandle( hOpenUrl );
		}
		InternetCloseHandle( hSession );
	}

	return returnVal;
}

bool parseTempFile( int* numTracks )
{
	bool retVal = false;
	wchar_t parseMsg[256];

	// Re-open file now that we know the size
	tinyxml2::XMLDocument doc;
	doc.LoadFile( "C:\\temp\\played.xml" );

	int parseError = doc.ErrorID();
	if ( parseError != 0 )
	{
		tinyxml2::XMLElement* lfmElement = doc.FirstChildElement();

		if ( lfmElement )
		{
			tinyxml2::XMLElement* recentTracksElement = lfmElement->FirstChildElement();
			if ( recentTracksElement )
			{
				tinyxml2::XMLError playsError = recentTracksElement->QueryIntAttribute( "total", numTracks );

				if ( playsError == tinyxml2::XML_WRONG_ATTRIBUTE_TYPE )
				{
					output->writeMessage( L"Wrong attribute type!" );
				}
				else if ( playsError == tinyxml2::XML_NO_ATTRIBUTE )
				{
					output->writeMessage( L"No attribute!" );
				}
				else if ( playsError == tinyxml2::XML_NO_ERROR )
				{
					wsprintf( parseMsg, L"total plays: %d", *numTracks );
					output->writeMessage( parseMsg );
					retVal = true;
				}
				else
				{
					output->writeMessage( L"Other error!" );
				}
			}
			else
			{
				output->writeMessage( L"Couldn't find recent tracks element" );
			}
		}
		else
		{
			output->writeMessage( L"Couldn't find lfm element" );
		}
	}
	else
	{
		wsprintf( parseMsg, L"Parse error: %d", parseError );
		output->writeMessage( parseMsg );
	}

	return retVal;
}

bool getTracksPage( int numTracks )
{
	return false;
}

/*
IPC_INETAVAILABLE

IPC_GETHTTPGETTER

IPC_GETHTTPGETTERW

IPC_GET_EXTENDED_FILE_INFO

IPC_SET_EXTENDED_FILE_INFO
IPC_WRITE_EXTENDED_FILE_INFO
*/