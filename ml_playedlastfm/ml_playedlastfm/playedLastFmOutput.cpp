#include "stdafx.h"
#include "playedLastFmOutput.h"

#include "wa_ipc.h"

#include <string>

const wchar_t* playedLastFmOutput::kLogFilename = L"\\Plugins\\ml_playedlastfm.log";

playedLastFmOutput::playedLastFmOutput( HWND winampHandle )
	: mIsOpen( false ),
	mWinampHandle( winampHandle ),
	mOutputFile( NULL )
{
}


playedLastFmOutput::~playedLastFmOutput(void)
{
	if ( mIsOpen )
	{
		if ( !closeFile() )
		{
			// handle some kind of can't-close-file error
		}
	}
}

bool playedLastFmOutput::closeFile()
{
	if ( mIsOpen )
	{
		// close the file
		errno_t error;
		if ( ( error = fclose( mOutputFile ) ) == 0 )
		{
			mOutputFile = NULL;
			mIsOpen = false;
			return true;
		}
		else
		{
			// couldn't close the file?
			return false;
		}
	}
	else
	{
		return true;
	}
}

bool playedLastFmOutput::openFile()
{
	if ( mIsOpen )
	{
		return true;
	}
	else
	{
		const size_t kPathLen = 512;
		wchar_t logPath[kPathLen];
		size_t convertedChars = 0;
		errno_t fileOpenError;

		char *dir = (char*)SendMessage( mWinampHandle, WM_WA_IPC, 0, IPC_GETINIDIRECTORY );
		if ( dir != NULL )
		{

			size_t origsize = strlen( dir ) + 1;
			if ( origsize <= kPathLen )
			{
				mbstowcs_s( &convertedChars, logPath, origsize, dir, _TRUNCATE );
				wcscat_s( logPath, kPathLen, kLogFilename );
				if ( ( fileOpenError = _wfopen_s( &mOutputFile, logPath, L"at" ) ) == 0 )
				{
					mIsOpen = true;
					return true;
				}
				else
				{
					// Failed to open file
					MessageBox( mWinampHandle, L"_wfopen_s failed, trying _wfopen", L"", MB_OK );
					if ( ( mOutputFile = _wfopen( logPath, L"at" ) ) != NULL )
					{
						mIsOpen = true;
						return true;
					}

					return false;
				}
			}
			else
			{
				// Not enough buffer to hold the path... ?!
				return false;
			}
		}
		else
		{
			// IPC_GETINIDIRECTORY failed?!
			return false;
		}
	}
}

bool playedLastFmOutput::writeMessage( wchar_t* message )
{
	if ( !mIsOpen && !openFile() )
	{
		return false;
	}

	// actually write out the message
	std::wstring ws( message );
	std::string msg( ws.begin(), ws.end() );
	fwrite( msg.c_str(), sizeof( char ), strlen( msg.c_str() ), mOutputFile );
	fwrite( "\n", sizeof( char ), strlen( "\n" ), mOutputFile );

	return true;
}
