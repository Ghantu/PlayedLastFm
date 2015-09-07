#ifndef ml_playedlastfm_h
#define ml_playedlastfm_h

// ml_playedlastfm.h

#include <windows.h>

// plugin name/title
#define PLUGIN_NAME "Played Last via Last.fm"

// User-Agent string sent to Last.fm server
#define USER_AGENT L"Winamp PlayedLastFm"

// API Key for Last.fm API access
#define API_KEY L"a12776f4ca2ca23f426a16d1d44d0908"
#define REQUEST_STRING L"http://ws.audioscrobbler.com/2.0/?method=user.getrecenttracks&user=%s&api_key=%s&limit=%d&page=%d&from=%ld&to="
#define INIFILENAME L"\\Plugins\\ml_playedlastfm.ini"
#define WINDOWS_TICK 10000000
#define SEC_TO_UNIX_EPOCH 11644473600LL
#define SYNC_INTERVAL 60 * 60
#define TEMP_FILE_PATH L"C:\\temp\\played.xml"
#define TRACKS_PER_PAGE 200

class playedLastFmOutput;

typedef struct 
{
	char* artistName;
	char* trackName;
	char* albumName;
	int dateUts;
} TrackInfo;

int          init( void );
void         quit( void );
INT_PTR      MessageProc( int message_type, INT_PTR param1, INT_PTR param2, INT_PTR param3 );
void         performLastFmSync();
void         getInitFileName( wchar_t* filename, size_t numChars );
DWORD WINAPI PlayedLastFmThread( LPVOID lpParam );
time_t       getCurrentTime();
bool         parseTempFile( int* numTracks );
bool         parseTempFile( TrackInfo* trackInfo );
bool         queryLastFm( int limit, int page );

time_t  lastSyncTime;
time_t  currentSyncTime;
time_t  syncInterval;
wchar_t lastFmUsername[256];
HANDLE  threadHandle;
DWORD   threadId;
bool    quitThread;
int     numPages;

playedLastFmOutput* output;

#endif // ml_playedlastfm_h