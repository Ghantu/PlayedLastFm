#pragma once

#include "windows.h"

class playedLastFmOutput
{
public:
	playedLastFmOutput( HWND winampHandle );
	~playedLastFmOutput(void);

	bool writeMessage( wchar_t* message );

private:
	bool closeFile();
	bool openFile();

	bool mIsOpen;
	HWND mWinampHandle;
};

