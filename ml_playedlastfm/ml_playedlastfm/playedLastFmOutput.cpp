#include "stdafx.h"
#include "playedLastFmOutput.h"

#include "wa_ipc.h"

#include <string>

const wchar_t* playedLastFmOutput::kLogFilename = L"\\Plugins\\ml_playedlastfm.log";

playedLastFmOutput::playedLastFmOutput( HWND winampHandle )
	: mIsOpen( false ),
	mOutputFile( NULL )
{
	size_t convertedChars = 0;

	char *dir = (char*)SendMessage( winampHandle, WM_WA_IPC, 0, IPC_GETINIDIRECTORY );
	if ( dir != NULL )
	{
		size_t origsize = strlen( dir ) + 1;
		if ( origsize <= kPathLen )
		{
			mbstowcs_s( &convertedChars, mLogPath, origsize, dir, _TRUNCATE );
			wcscat_s( mLogPath, kPathLen, kLogFilename );
			mFilenameValid = true;
		}
		else
		{
			// Not enough buffer to hold the path... ?!
			mFilenameValid = false;
		}
	}
	else
	{
		// IPC_GETINIDIRECTORY failed?!
		mFilenameValid = false;
	}
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
		if ( mFilenameValid )
		{
			errno_t fileOpenError;
			if ( ( fileOpenError = _wfopen_s( &mOutputFile, mLogPath, L"at" ) ) == 0 )
			{
				mIsOpen = true;
				return true;
			}
			else
			{
				// Failed to open file
				return false;
			}
		}
		else
		{
			// failed to establish the filename and path in constructor
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

	fwprintf( mOutputFile, message );
	fwprintf( mOutputFile, L"\n" );

	return true;
}
