#pragma once

#include "Windows.h"
#include "stdio.h"

class playedLastFmOutput
{
public:
	playedLastFmOutput( HWND winampHandle );
	~playedLastFmOutput(void);

	bool closeFile();
	bool writeMessage( wchar_t* message );

	static const wchar_t* kLogFilename;

private:
	bool openFile();

	bool  mIsOpen;
	FILE* mOutputFile;

	static const size_t kPathLen = 512;
	wchar_t      mLogPath[kPathLen];
	bool         mFilenameValid;
};

