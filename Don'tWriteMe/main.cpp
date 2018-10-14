#include <windows.h>
#include <ctime>

const int HOTKEY_NORMAL = 1; // write as-is
const int HOTKEY_IDE = 2; // write ommitting tabs and closing curly brackets
const int HOTKEY_ABORT = 3;

volatile int asciiToVk[128];
volatile bool shouldStop = false;
volatile bool threadRunning = false;
volatile bool ideMode = false;

const int BUFFER_SIZE = 100000;
char buffer[BUFFER_SIZE];

void buildMap();
void sendChar(char c);
DWORD WINAPI sendText(void * szText);
void sendTextThread(char * szText);
void removeTabs(char * szText);
int getClipboardText(char * buffer, int bufferSize);

int main()
{
	buildMap();
	srand(time(0));

	if (!RegisterHotKey(0, HOTKEY_NORMAL, 0, VK_NUMPAD4)) return 0;
	if (!RegisterHotKey(0, HOTKEY_IDE, 0, VK_NUMPAD5)) return 0;
	if (!RegisterHotKey(0, HOTKEY_ABORT, 0, VK_NUMPAD2)) return 0;

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if (msg.message == WM_HOTKEY)
		{
			int hotkey = msg.wParam;
			if (hotkey == HOTKEY_ABORT)
			{
				shouldStop = true;
				continue;
			}

			if (hotkey == HOTKEY_NORMAL) ideMode = false;
			else if (hotkey == HOTKEY_IDE) ideMode = true;
			else continue;

			int len = getClipboardText(buffer, 10000);
			if (len)
			{
				shouldStop = false;
				if (!threadRunning) sendTextThread(buffer);
			}
		}
	}

	return 0;
}

void sendTextThread(char * szText)
{
	CreateThread(0, 0, sendText, szText, 0, 0);
}

int getClipboardText(char * buffer, int bufferSize)
{
	if (!OpenClipboard(nullptr)) return 0;

	HANDLE hData = GetClipboardData(CF_TEXT);
	if (!hData) return 0;

	char * pszText = (char*)GlobalLock(hData);
	if (!pszText) return 0;

	int len = strlen(pszText);
	if (len > bufferSize) len = bufferSize - 1;

	memcpy(buffer, pszText, len);
	buffer[len] = 0;

	GlobalUnlock(hData);
	CloseClipboard();

	return len;
}

void removeTabs(char * szText)
{
	char * idx = szText;
	while (*szText)
	{
		if (*szText == '\t' || *szText == '\r')
		{
			szText++;
			continue;
		}

		*idx = *szText;
		idx++;
		szText++;
	}

	*idx = 0;
}

DWORD WINAPI sendText(void * ptr)
{
	threadRunning = true;
	char * szText = (char*)ptr;

	if (ideMode) removeTabs(szText);

	int enter = 0;

	while (*szText)
	{
		if (shouldStop)
		{
			threadRunning = false;
			return 0;
		}

		if (ideMode)
		{
			if (*szText == '\n')
			{
				if (enter >= 2)
				{
					szText++;
					continue;
				}
				enter++;				
			}
			else enter = 0;

			if (*szText == '\n' && *(szText + 1) == '}')
			{
				keybd_event(VK_DOWN, MapVirtualKey(VK_DOWN, MAPVK_VK_TO_VSC), 0, 0);
				keybd_event(VK_DOWN, MapVirtualKey(VK_DOWN, MAPVK_VK_TO_VSC), KEYEVENTF_KEYUP, 0);
				sendChar('\n');
				enter = 1;
				szText++;
				szText++;
			}
			else sendChar(*szText++);
		}
		else
		{
			sendChar(*szText++);
		}

		Sleep(10 + rand() % 10);
	}
	threadRunning = false;

	return 0;
}

void sendChar(char c)
{
	if (c == 0) return;
	if (c > 127) return;

	int key = asciiToVk[c];
	if (key == 0) return;

	if (key < 0)
	{
		key = ~key;
		keybd_event(VK_SHIFT, MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC), 0, 0);

		keybd_event(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), 0, 0);
		keybd_event(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), KEYEVENTF_KEYUP, 0);

		keybd_event(VK_SHIFT, MapVirtualKey(VK_SHIFT, MAPVK_VK_TO_VSC), KEYEVENTF_KEYUP, 0);
	}
	else
	{
		keybd_event(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), 0, 0);
		keybd_event(key, MapVirtualKey(key, MAPVK_VK_TO_VSC), KEYEVENTF_KEYUP, 0);
	}
}

void buildMap()
{
	int diff = 'A' - 'a';
	for (int i = 'a'; i <= 'z'; i++)
	{
		asciiToVk[i] = 0x41 + i - 'a';
		asciiToVk[i + diff] = ~(0x41 + i - 'a'); // shift must be pressed
	}

	for (int i = '0'; i <= '9'; i++)
	{
		asciiToVk[i] = 0x30 + i - '0';
	}

	asciiToVk[')'] = ~asciiToVk['0'];
	asciiToVk['!'] = ~asciiToVk['1'];
	asciiToVk['@'] = ~asciiToVk['2'];
	asciiToVk['#'] = ~asciiToVk['3'];
	asciiToVk['$'] = ~asciiToVk['4'];
	asciiToVk['%'] = ~asciiToVk['5'];
	asciiToVk['^'] = ~asciiToVk['6'];
	asciiToVk['&'] = ~asciiToVk['7'];
	asciiToVk['*'] = ~asciiToVk['8'];
	asciiToVk['('] = ~asciiToVk['9'];

	asciiToVk['-'] = 0xBD;
	asciiToVk['='] = 0xBB;
	asciiToVk['+'] = ~asciiToVk['='];
	asciiToVk['_'] = ~asciiToVk['-'];

	asciiToVk[' '] = VK_SPACE;

	asciiToVk['`'] = 0xC0;
	asciiToVk['~'] = ~asciiToVk['`'];
	asciiToVk['\t'] = 0x09;
	asciiToVk['\n'] = 0x0D;

	asciiToVk['/'] = 0xBF;
	asciiToVk['?'] = ~asciiToVk['/'];

	asciiToVk['\\'] = 0xDC;
	asciiToVk['|'] = ~asciiToVk['\\'];

	asciiToVk['\''] = 0xDE;
	asciiToVk['\"'] = ~asciiToVk['\''];

	asciiToVk[','] = 0xBC;
	asciiToVk['.'] = 0xBE;
	asciiToVk['<'] = ~asciiToVk[','];
	asciiToVk['>'] = ~asciiToVk['.'];

	asciiToVk['['] = 0xDB;
	asciiToVk[']'] = 0xDD;
	asciiToVk['{'] = ~asciiToVk['['];
	asciiToVk['}'] = ~asciiToVk[']'];

	asciiToVk[';'] = 0xBA;
	asciiToVk[':'] = ~asciiToVk[';'];
}