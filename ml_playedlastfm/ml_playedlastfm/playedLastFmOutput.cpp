#include "stdafx.h"
#include "playedLastFmOutput.h"

#include "wa_ipc.h"


playedLastFmOutput::playedLastFmOutput( HWND winampHandle )
	: mIsOpen( false ),
	  mWinampHandle( winampHandle )
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

		return true;
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
		wchar_t iniPath[kPathLen];

		char *dir = (char*)SendMessage( mWinampHandle, WM_WA_IPC, 0, IPC_GETINIDIRECTORY );

		return true;
	}
}

bool playedLastFmOutput::writeMessage( wchar_t* message )
{
	if ( !mIsOpen )
	{
		if ( !openFile() )
		{
			return false;
		}
	}

	// actually write out the message

	return true;
}
