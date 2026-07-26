#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "platform_tools.h"

//## DEFINES ##

#define CH8_RAM_SIZE 4096
#define CH8_V_SIZE 16
#define CH8_STACK_SIZE 16
#define CH8_START_ADDRESS 0x200
#define CH8_SCREEN_WIDTH 64
#define CH8_SCREEN_HEIGHT 32
#define CH8_FONTSET_SIZE 16
#define CH8_KEYMAP_SIZE 16
#define CH8_DEBUG_LINES 16
#define CH8_DEBUG_LINE_LEN 64
#define CH8_LINE_LEN (CH8_SCREEN_WIDTH * CH8_MAX_FILL_LEN + 3 + CH8_DEBUG_LINE_LEN + 1)
#define CH8_VF CH8_V_SIZE-1
#define CH8_UNICODE_FILL "█"
#define CH8_ASCII_FILL "#"
#define CH8_ASCII_FREE ' '
#define CH8_MAX_FILL_LEN 4
#define CH8_CPU_HZ 700.0
#define CH8_TIMER_HZ 60.0
#define CH8_FRAME_HZ 60.0
#define CH8_IPF 11 // For now, it will be a constant, but eventually, we'll need to add a flag to change this value.

//## CONSTS ##

const uint8_t CH8_FONTSET[CH8_FONTSET_SIZE][5] =
{
	{0xF0,0x90,0x90,0x90,0xF0}, //0
	{0x20,0x60,0x20,0x20,0x70}, //1
	{0xF0,0x10,0xF0,0x80,0xF0}, //2
	{0xF0,0x10,0xF0,0x10,0xF0}, //3
	{0x90,0x90,0xF0,0x10,0x10}, //4
	{0xF0,0x80,0xF0,0x10,0xF0}, //5
	{0xF0,0x80,0xF0,0x90,0xF0}, //6
	{0xF0,0x10,0x20,0x40,0x40}, //7
	{0xF0,0x90,0xF0,0x90,0xF0}, //8
	{0xF0,0x90,0xF0,0x10,0xF0}, //9
	{0xF0,0x90,0xF0,0x90,0x90}, //A
	{0xE0,0x90,0xE0,0x90,0xE0}, //B
	{0xF0,0x80,0x80,0x80,0xF0}, //C
	{0xE0,0x90,0x90,0x90,0xE0}, //D
	{0xF0,0x80,0xF0,0x80,0xF0}, //E
	{0xF0,0x80,0xF0,0x80,0x80}  //F
};

const int CH8_KEYMAP[CH8_KEYMAP_SIZE] =
{
    'x', '1', '2', '3', 'q', 'w', 'e', 'a', 's', 'd', 'z', 'c', '4', 'r', 'f', 'v'
};

//## TYPEDEFS ##

typedef struct
{
    const char* rom_path;
    char help;
    char debug;
    char print_hex_rom;
} Chip8Config;

typedef struct
{
    uint8_t display[CH8_SCREEN_WIDTH * CH8_SCREEN_HEIGHT];
    uint16_t keys;
    bool wait_for_vblank;    
    int waiting_key;
    // Let it stay this way for now, but these two variables should not be there in the future.

    uint8_t ram[CH8_RAM_SIZE];
    uint8_t V[CH8_V_SIZE];

    uint16_t I;
    uint16_t PC;
    uint8_t SP;

    uint8_t delay_timer;
    uint8_t sound_timer;

    uint16_t stack[CH8_STACK_SIZE];
} Chip8;

//## UTILS ##

void error_exit(const char* message)
{
    print_color_text(message, CONSOLE_RED);
    exit(1);
}

void print_help()
{
    printf(
        "      _     _____                      \n"
        "     | |   |  _  |                     \n"
        "  ___| |__  \\ V /  ___ _ __ ___  _   _ \n"
        " / __| '_ \\ / _ \\ / _ \\ '_ ` _ \\| | | |\n"
        "| (__| | | | |_| |  __/ | | | | | |_| |\n"
        " \\___|_| |_|\\___/\\___|_| |_| |_|\\__,_|\n"
        " by zhjckfd\n"
        "\n"
        " Usage:\n"
        "    [Path to ROM]             Run ROM\n"
        "    -h                        Show arguments list\n"
        "    -d                        Debug mod\n"
        "    -p                        Print Hex ROM\n"
        "    --ascii                   Set ASCII display mode\n"
        "    --mute                    Off audio\n"
    );
    exit(0);
}

void print_hex_array(uint8_t* bytes, long len)
{
    for (int i = 0; i < len; i++)
        printf("%02X ", (uint8_t)bytes[i]);
}

void build_debug_lines(char (*lines)[CH8_DEBUG_LINE_LEN], Chip8* ch8)
{
    uint16_t opcode = (ch8->ram[ch8->PC] << 8) | ch8->ram[ch8->PC + 1];

    snprintf(lines[0], CH8_DEBUG_LINE_LEN, "       DEBUG MODE       ", ch8->PC, opcode);

    snprintf(lines[1], CH8_DEBUG_LINE_LEN, "PC: %04X OP: %04X%*s", ch8->PC, opcode, 8, "");
    snprintf(lines[2], CH8_DEBUG_LINE_LEN, "I : %04X SP: %02X%*s", ch8->I, ch8->SP, 8, "");
    snprintf(lines[3], CH8_DEBUG_LINE_LEN, "DT: %02X ST: %02X%*s", ch8->delay_timer, ch8->sound_timer, 10, "");
    snprintf(lines[4], CH8_DEBUG_LINE_LEN, "KEYS: %04X%*s", ch8->keys, 13, "");

    for (int i = 5; i < CH8_DEBUG_LINES; i++)
        memset(lines[i], 0, CH8_DEBUG_LINE_LEN);

    int l = 4;
    for (int i = 0; i < 16; i += 4)
    {
        snprintf(lines[l], CH8_DEBUG_LINE_LEN,
            "V%X:%02X V%X:%02X V%X:%02X V%X:%02X",
            i,   ch8->V[i],
            i+1, ch8->V[i+1],
            i+2, ch8->V[i+2],
            i+3, ch8->V[i+3]
        );
        l++;
    }
}

void print_display(Chip8* ch8, char debug_mode)
{
    reset_screen();

    static char buffer[CH8_LINE_LEN * CH8_SCREEN_HEIGHT + 1];
    char* p = buffer;

    char lines[CH8_DEBUG_LINES][CH8_DEBUG_LINE_LEN];
    build_debug_lines(lines, ch8);

    for (int y = 0; y < CH8_SCREEN_HEIGHT; y++)
    {

        for (int x = 0; x < CH8_SCREEN_WIDTH; x++)
        {
            uint8_t pixel = ch8->display[y * CH8_SCREEN_WIDTH + x];
            const char *fill = use_unicode ? CH8_UNICODE_FILL : CH8_ASCII_FILL;

            if (pixel)
            {
                size_t len = (use_unicode ? 3 : 1);

                if (p + len < buffer + sizeof(buffer))
                {
                    memcpy(p, fill, len);
                    p += len;
                }
            }
            else
            {
                *p++ = CH8_ASCII_FREE;
            }
        }

        if (debug_mode)
        {
            if (y < CH8_DEBUG_LINES)
                p += sprintf(p, " | %s", lines[y]);
            else
                p += sprintf(p, " |");
        }

        *p++ = '\n';
    }

    *p = '\0';

    fputs(buffer, stdout);
    fflush(stdout);
}

//## ROM ##

bool file_to_bytes(const char* path, uint8_t** romBytes, long* romSize)
{
    FILE* fileptr;
    fileptr = fopen(path, "rb");
    if (!fileptr)
        return false;

    fseek(fileptr, 0, SEEK_END);
    *romSize = ftell(fileptr);
    rewind(fileptr);

    *romBytes = (uint8_t*)malloc(*romSize * sizeof(uint8_t));
    if (!*romBytes)
    {
        fclose(fileptr);
        return false;
    }
    fread(*romBytes, 1, *romSize, fileptr);
    fclose(fileptr);

    return true;
}

//## CHIP8 CORE ##

Chip8 init_chip8()
{
    Chip8 ch8 =  {0};

    ch8.keys = 0;
    ch8.waiting_key = -1;

    ch8.PC = CH8_START_ADDRESS;
    ch8.SP = 0;
    ch8.I = 0;

    ch8.delay_timer = 0;
    ch8.sound_timer = 0;

    return ch8;
}

bool load_rom_to_ram(Chip8* ch8, const uint8_t* bytes, const long size)
{
    for (long i = 0; i < size; i++)
    {
        if (i + CH8_START_ADDRESS >= CH8_RAM_SIZE)
            return false;

        ch8->ram[i + CH8_START_ADDRESS] = bytes[i];
    }

    return true;
}

void load_fontset_to_ram(Chip8* ch8)
{
    for (long i = 0; i < CH8_FONTSET_SIZE * 5; i++)
    {
        ch8->ram[i] = CH8_FONTSET[i / 5][i % 5];
    }
}

void unknown_opcode_error(Chip8* ch8, uint16_t opcode)
{
    char buf[128];

    snprintf(buf, sizeof(buf),
        "Unknown opcode: 0x%04X at PC=0x%04X",
        opcode,
        ch8->PC);

    error_exit(buf);
}

void display_draw(Chip8* ch8, uint16_t opcode)
{
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t n = (opcode & 0x000F);

    uint8_t vx = ch8->V[x] % CH8_SCREEN_WIDTH;
    uint8_t vy = ch8->V[y] % CH8_SCREEN_HEIGHT;

    ch8->V[0xF] = 0;

    for (int i = 0; i < n; i++)
    {
        if (vy + i >= CH8_SCREEN_HEIGHT)
            break;

        uint8_t sprite_byte = ch8->ram[ch8->I + i];

        for (int j = 0; j < 8; j++)
        {
            if (vx + j >= CH8_SCREEN_WIDTH)
                break;

            if (sprite_byte & (0x80 >> j))
            {
                uint16_t px = vx + j;
                uint16_t py = vy + i;

                if (ch8->display[py * CH8_SCREEN_WIDTH + px] == 1)
                    ch8->V[0xF] = 1;

                ch8->display[py * CH8_SCREEN_WIDTH + px] ^= 1;
            }
        }
    }
}

void opcode_runtime(Chip8* ch8, uint16_t opcode)
{
    uint8_t k, x, y;
    switch (opcode & 0xF000)
    {
    case 0x0000:
        if (opcode == 0x00E0) // CLS
        {
            memset(ch8->display, 0, CH8_SCREEN_WIDTH * CH8_SCREEN_HEIGHT * sizeof(uint8_t));
        }
        else if (opcode == 0x00EE) // RET
        {
            if (ch8->SP == 0) error_exit("RET called with empty stack");
            ch8->PC = ch8->stack[--ch8->SP];
        }
        else // SYS addr
        {
            //ch8->PC = (opcode & 0x0FFF);
        }
        break;
    case 0x1000: // JP addr
        ch8->PC = (opcode & 0x0FFF);
        break;
    case 0x2000: // CALL addr
        if (ch8->SP >= CH8_STACK_SIZE) error_exit("SP overflow in CALL opcode");
        ch8->stack[ch8->SP] = ch8->PC;
        ch8->SP++;
        ch8->PC = (opcode & 0x0FFF);
        break;
    case 0x3000: // SE Vx, byte
        k = (opcode & 0x00FF);
        x = (opcode & 0x0F00) >> 8;
        if (ch8->V[x] == k) ch8->PC += 2;
        break;
    case 0x4000: // SNE Vx, byte
        k = (opcode & 0x00FF);
        x = (opcode & 0x0F00) >> 8;
        if (ch8->V[x] != k) ch8->PC += 2;
        break;
    case 0x5000: // SE Vx, Vy
        if ((opcode & 0x000F) != 0) break;
        x = (opcode & 0x0F00) >> 8;
        y = (opcode & 0x00F0) >> 4;
        if (ch8->V[x] == ch8->V[y]) ch8->PC += 2;
        break;
    case 0x6000: // LD Vx, byte
        k = (opcode & 0x00FF);
        x = (opcode & 0x0F00) >> 8;
        ch8->V[x] = k;
        break;
    case 0x7000: // ADD Vx, byte
        k = (opcode & 0x00FF);
        x = (opcode & 0x0F00) >> 8;
        ch8->V[x] += k;
        break;
    case 0x8000:
        x = (opcode & 0x0F00) >> 8;
        y = (opcode & 0x00F0) >> 4;

        switch (opcode & 0xF00F)
        {
        case 0x8000: // LD Vx, Vy
            ch8->V[x] = ch8->V[y];
            break;
        case 0x8001: // OR Vx, Vy
            ch8->V[x] = ch8->V[x] | ch8->V[y];
            ch8->V[CH8_VF] = 0;
            break;
        case 0x8002: // AND Vx, Vy
            ch8->V[x] = ch8->V[x] & ch8->V[y];
            ch8->V[CH8_VF] = 0;
            break;
        case 0x8003: // XOR Vx, Vy
            ch8->V[x] = ch8->V[x] ^ ch8->V[y];
            ch8->V[CH8_VF] = 0;
            break;
        case 0x8004: // ADD Vx, Vy
        {
            uint16_t sum = ch8->V[x] + ch8->V[y];
            ch8->V[x] = sum & 0xFF;
            ch8->V[CH8_VF] = (sum > 0xFF);
            break;
        }
        case 0x8005: // SUB Vx, Vy
        {
            uint8_t vx = ch8->V[x];
            uint8_t vy = ch8->V[y];
            
            ch8->V[x] = vx - vy;
            ch8->V[CH8_VF] = (vx >= vy);
            break;
        }
        case 0x8006: // SHR Vx or SHR Vx, Vy
        {
            //uint8_t lsb = ch8->V[x] & 1;
            //ch8->V[x] = ch8->V[x] >> 1;
            //ch8->V[CH8_VF] = lsb;
            uint8_t vy = ch8->V[y];

            ch8->V[x] = vy >> 1;
            ch8->V[CH8_VF] = vy & 1;
            break;
        }
        case 0x8007: // SUBN Vx, Vy
        {
            uint8_t vx = ch8->V[x];
            uint8_t vy = ch8->V[y];
            
            ch8->V[x] = vy - vx;
            ch8->V[CH8_VF] = (vy >= vx);
            break;
        }
        case 0x800E: // SHL Vx or SHR Vx, Vy
        {
            //uint8_t msb = ch8->V[x] >> 7;
            //ch8->V[x] = ch8->V[x] << 1;
            //ch8->V[CH8_VF] = msb;
            uint8_t vy = ch8->V[y];

            ch8->V[x] = vy << 1;
            ch8->V[CH8_VF] = vy >> 7;
            break;
        }
        default:
            unknown_opcode_error(ch8, opcode);
            break;
        }

        break;
    case 0x9000: // SNE Vx, Vy
        if ((opcode & 0x000F) != 0) break;
        x = (opcode & 0x0F00) >> 8;
        y = (opcode & 0x00F0) >> 4;
        if (ch8->V[x] != ch8->V[y]) ch8->PC += 2;
        break;
    case 0xA000: // LD I, addr
        ch8->I = (opcode & 0x0FFF);
        break;
    case 0xB000: // JP V0, addr
        ch8->PC = ch8->V[0] + (opcode & 0x0FFF);
        break;
    case 0xC000: // RND Vx, byte
        k = (opcode & 0x00FF);
        x = (opcode & 0x0F00) >> 8;
        ch8->V[x] = (rand() % 256) & k;
        break;
    case 0xD000: // DRW Vx, Vy, nibble
        display_draw(ch8, opcode);
        ch8->wait_for_vblank = true;
        break;
    case 0xE000:
        x = (opcode & 0x0F00) >> 8;

        switch (opcode & 0xF0FF)
        {
            case 0xE09E: // SKP Vx
            {
                uint8_t key = ch8->V[x] & 0xF;
                
                if (ch8->keys & (1 << key))
                    ch8->PC += 2;
                break;
            }
            case 0xE0A1: // SKNP Vx
            {
                uint8_t key = ch8->V[x] & 0xF;

                if (!(ch8->keys & (1 << key)))
                    ch8->PC += 2;
                break;
            }
            default:
                unknown_opcode_error(ch8, opcode);
                break;
        }
        break;
    case 0xF000:
        x = (opcode & 0x0F00) >> 8;
        switch (opcode & 0xF0FF) {
            case 0xF007: // LD Vx, DT
                ch8->V[x] = ch8->delay_timer;
                break;
            case 0xF00A: // LD Vx, K
                if (ch8->waiting_key == -1) 
                {
                    for (int i = 0; i < 16; i++)
                    {
                        if (ch8->keys & (1 << i))
                        {
                            ch8->waiting_key = i;
                            break;
                        }
                    }
                    
                    if (ch8->waiting_key == -1)
                    {
                        ch8->PC -= 2;
                        break;
                    }
                }


                if (ch8->keys & (1 << ch8->waiting_key))
                {
                    ch8->PC -= 2;
                    break;
                }

                ch8->V[x] = ch8->waiting_key;
                ch8->waiting_key = -1;
                break;
            case 0xF015: // LD DT, Vx
                ch8->delay_timer = ch8->V[x];
                break;
            case 0xF018: // LD ST, Vx
                ch8->sound_timer = ch8->V[x];
                break;
            case 0xF01E: // ADD I, Vx
                ch8->I += ch8->V[x];
                break;
            case 0xF029: // LD F, Vx
                ch8->I = (ch8->V[x] & 0xF) * 5;
                break;
            case 0xF033: // LD B, Vx
                uint8_t value = ch8->V[x];

                ch8->ram[(ch8->I) & 0xFFF]     = value / 100;
                ch8->ram[(ch8->I + 1) & 0xFFF] = (value / 10) % 10;
                ch8->ram[(ch8->I + 2) & 0xFFF] = value % 10;

                break;
            case 0xF055: // LD [I], Vx
                for (int i = 0; i <= x; i++)
                    ch8->ram[(ch8->I + i) & 0xFFF] = ch8->V[i];
                ch8->I += x + 1;
                break;
            case 0xF065: // LD Vx, [I]
                for (int i = 0; i <= x; i++)
                    ch8->V[i] = ch8->ram[(ch8->I + i) & 0xFFF];
                ch8->I += x + 1;
                break;
            default:
                unknown_opcode_error(ch8, opcode);
                break;
        }
        break;
    default:
        unknown_opcode_error(ch8, opcode);
        break;
    }
}

void update_keys(Chip8 *ch8)
{
    ch8->keys = 0;

    for (int i = 0; i < CH8_KEYMAP_SIZE; i++)
    {
        if (get_press(CH8_KEYMAP[i]))
        {
            ch8->keys |= (1 << i);
        }
    }
}

void update_timer(Chip8 *ch8)
{
    if (ch8->delay_timer > 0)
        ch8->delay_timer--;

    if (ch8->sound_timer > 0)
        ch8->sound_timer--;

    sound_active = (ch8->sound_timer > 0);
}

void chip8_main(Chip8Config* cfg)
{
    Chip8 ch8 = init_chip8();
    uint8_t* romBytes;
    long romSize;

    if (!file_to_bytes(cfg->rom_path, &romBytes, &romSize)) error_exit("Can't read file");
    if (cfg->print_hex_rom)
    {
        print_hex_array(romBytes, romSize);
	    printf("\n");
        exit(0);
    }

    if (!load_rom_to_ram(&ch8, romBytes, romSize)) error_exit("RAM overflow");
    free(romBytes);

    load_fontset_to_ram(&ch8);

    clear_terminal();

    double timer_acc = 0;
    double frame_time = 0;
    const double timer_time = 1.0 / CH8_TIMER_HZ;
    const double target_frame_time = 1.0 / 60.0;

    double last = now();

    while (1)
    {
        double current = now();
        double dt = current - last;
        last = current;
        frame_time = current;

        update_keys(&ch8);

        timer_acc += dt;
        while (timer_acc >= timer_time)
        {
            update_timer(&ch8);
            timer_acc -= timer_time;
        }
        
        for (int i = 0; i < CH8_IPF; i++)
        {            
            if (ch8.wait_for_vblank)
                break;
            
            uint16_t opcode = (ch8.ram[ch8.PC] << 8) | ch8.ram[ch8.PC + 1];
            ch8.PC += 2;
            opcode_runtime(&ch8, opcode);
        }

        if (ch8.wait_for_vblank)
        {
            print_display(&ch8, cfg->debug);
            ch8.wait_for_vblank = false;
        }

        double elapsed = now() - frame_time;
        double remaining = target_frame_time - elapsed;
        if (remaining > 0)
            delay((int)(remaining * 1000));
    }
    

   // while (1)
   // {
   //     double current = now();
   //     double dt = current - last;
   //     last = current;

   //     cpu_acc += dt;
   //     timer_acc += dt;
   //     frame_acc += dt;

   //     update_keys(&ch8);

   //     while (cpu_acc >= cpu_time)
   //     {
   //         if (ch8.wait_for_vblank)
   //             break;
   //         
   //         uint16_t opcode = (ch8.ram[ch8.PC] << 8) | ch8.ram[ch8.PC + 1];
   //         ch8.PC += 2;
   //         opcode_runtime(&ch8, opcode);

   //         cpu_acc -= cpu_time;
   //     }

   //     while (timer_acc >= timer_time)
   //     {
   //         update_timer(&ch8);
   //         timer_acc -= timer_time;
   //     }

   //     if (frame_acc >= frame_time)
   //     {
   //         print_display(&ch8, cfg->debug);
   //         ch8.wait_for_vblank = false;
   //         frame_acc -= frame_time;
   //     }

   //     delay(1);
   // }
}

//## MAIN LOOP ##

Chip8Config parse_args(int argc, char** argv)
{
    Chip8Config cfg = {0};

    for(int i = 1; i < argc; i++)
    {
        char* flag = argv[i];
        if (flag[0] == '-')
        {
            if (strcmp(flag, "-h") == 0) cfg.help = 1;
            if (strcmp(flag, "-d") == 0) cfg.debug = 1;
            if (strcmp(flag, "-p") == 0) cfg.print_hex_rom = 1;
            if (strcmp(flag, "--ascii") == 0) use_unicode = 0;
            if (strcmp(flag, "--mute") == 0) sound_mute = 1;
        }
        else
        {
            cfg.rom_path = flag;
        }
    }

    return cfg;
}

int main(int argc, char** argv)
{
    init_terminal();
    init_audio();

    srand(time(NULL));

    Chip8Config cfg = parse_args(argc, argv);

    if (cfg.help == 1 || !cfg.rom_path) print_help();

    chip8_main(&cfg);

    return 0;
}
