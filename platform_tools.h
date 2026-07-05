#pragma once

typedef enum
{
    CONSOLE_BLACK = 0,
    CONSOLE_RED,
    CONSOLE_GREEN,
    CONSOLE_YELLOW,
    CONSOLE_BLUE,
    CONSOLE_MAGENTA,
    CONSOLE_CYAN,
    CONSOLE_WHITE
} ConsoleColor;

extern char use_unicode;
extern char sound_active;
extern char sound_mute;

void print_color_text(const char* message, ConsoleColor color);
void reset_screen();
void init_terminal();
void reset_input();
void clear_terminal();
void delay(int milliseconds);
int get_press(int key);
double now();
void init_audio();
