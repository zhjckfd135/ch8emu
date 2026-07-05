#include <stdio.h>
#include "platform_tools.h"

char use_unicode = 0;
char sound_active = 0;
char sound_mute = 0;

#ifdef defined(_WIN32)

#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

#define SAMPLE_RATE 44100

static WORD to_win_color(ConsoleColor color)
{
    switch(color)
    {
        case CONSOLE_BLACK: return 0;
        case CONSOLE_RED: return FOREGROUND_RED;
        case CONSOLE_GREEN: return FOREGROUND_GREEN;
        case CONSOLE_YELLOW: return FOREGROUND_RED | FOREGROUND_GREEN;
        case CONSOLE_BLUE: return FOREGROUND_BLUE;
        case CONSOLE_MAGENTA: return FOREGROUND_RED | FOREGROUND_BLUE;
        case CONSOLE_CYAN: return FOREGROUND_GREEN | FOREGROUND_BLUE;
        case CONSOLE_WHITE: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        default: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}

void print_color_text(const char* message, ConsoleColor color)
{
    HANDLE hc = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hc, to_win_color(color));

    printf("%s\n", message);

    SetConsoleTextAttribute(hc, to_win_color(CONSOLE_WHITE));
}

int is_utf8_console()
{
    return GetConsoleOutputCP() == CP_UTF8;
}

void enable_ansi()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void init_terminal()
{
    enable_ansi();
    SetConsoleOutputCP(CP_UTF8);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);

    use_unicode = is_utf8_console();
}

void delay(int milliseconds)
{
    Sleep(milliseconds);
}

int get_press(int key)
{
    return (GetAsyncKeyState(key) & 0x8000) ? 1 : 0;
}

double now()
{
    static LARGE_INTEGER freq;
    static int init = 0;

    if (!init)
    {
        QueryPerformanceFrequency(&freq);
        init = 1;
    }

    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);

    return (double)t.QuadPart / (double)freq.QuadPart;
}

DWORD WINAPI audio_thread(LPVOID arg)
{
    HWAVEOUT hWaveOut;

    WAVEFORMATEX wf = {0};
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 1;
    wf.nSamplesPerSec = 44100;
    wf.wBitsPerSample = 8;
    wf.nBlockAlign = 1;
    wf.nAvgBytesPerSec = 44100;

    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        return 0;

    static char buffer1[1024];
    static char buffer2[1024];

    int toggle = 0;

    while (1)
    {
        if (sound_mute) continue;

        char* buffer = toggle ? buffer1 : buffer2;
        toggle = !toggle;

        for (int i = 0; i < 1024; i++)
        {
            buffer[i] = sound_active ? ((i & 32) ? 135 : 120) : 128;
        }

        WAVEHDR hdr = {0};
        hdr.lpData = buffer;
        hdr.dwBufferLength = 1024;

        waveOutPrepareHeader(hWaveOut, &hdr, sizeof(hdr));
        waveOutWrite(hWaveOut, &hdr, sizeof(hdr));

        Sleep(5);
    }
}

void init_audio()
{
    CreateThread(NULL, 0, audio_thread, NULL, 0, NULL);
}

#elif defined(__linux__)

#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>

static struct termios orig_termios;
static volatile int key_state[256] = {0};

void print_color_text(const char* message, ConsoleColor color)
{
    const char* col = "\x1b[37m";

    switch(color)
    {

    	case CONSOLE_BLACK:    col = "\x1b[30m"; break;
    	case CONSOLE_RED:      col = "\x1b[31m"; break;
    	case CONSOLE_GREEN:    col = "\x1b[32m"; break;
    	case CONSOLE_YELLOW:   col = "\x1b[33m"; break;
    	case CONSOLE_BLUE:     col = "\x1b[34m"; break;
    	case CONSOLE_MAGENTA:  col = "\x1b[35m"; break;
    	case CONSOLE_CYAN:     col = "\x1b[36m"; break;
    	case CONSOLE_WHITE:    col = "\x1b[37m"; break;
    }

    printf("%s%s\x1b[0m\n", col, message);
}

static void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void* input_thread(void* arg)
{
    char c;

    while (1)
    {
	for (int i = 0; i < 256; i++)
	    key_state[i] = 0;
	
    	if (read(STDIN_FILENO, &c, 1) == 1)
	    key_state[(unsigned char)c] = 1;

	usleep(100);
    }

    return NULL;
}

void init_terminal()
{
    enable_raw_mode();    

    printf("\x1b[?251");
    setvbuf(stdout, NULL, _IONBF, 0);
    
    pthread_t t;
    pthread_create(&t, NULL, input_thread, NULL);
    pthread_detach(t);

    use_unicode = 1;
}

void delay(int milliseconds)
{
    usleep(milliseconds * 1000);
}

int get_press(int key)
{
    return key_state[(unsigned char)key];
}

double now()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void init_audio()
{
}

#endif

void clear_terminal()
{
    printf("\x1b[2J\x1b[H");
}

void reset_screen()
{
    printf("\x1b[H");
}
