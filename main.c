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
#define CH8_MAX_FILL_LEN 4
#define CH8_LINE_LEN (CH8_SCREEN_WIDTH * CH8_MAX_FILL_LEN + 3 + 1)
#define CH8_VF CH8_V_SIZE-1
#define CH8_SYMBOL_FILL "█"
#define CH8_SYMBOL_FILL_UP "▀"
#define CH8_SYMBOL_FILL_DOWN "▄"
#define CH8_SYMBOL_FREE " "
#define CH8_TIMER_HZ 60.0
#define CH8_FRAME_HZ 60.0
#define CH8_IPF_DEFAULT 11
#define CH8_DEBUG_HISTORY 20

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
    bool help;

    bool debug;
    const char* rom_path;
    int ipf;
} Chip8Config;

typedef struct
{
    bool debug;
    const char* rom_path;
    int ipf; 
} Chip8EmulationConfig;

typedef struct
{
    uint8_t display[CH8_SCREEN_WIDTH * CH8_SCREEN_HEIGHT];
    uint16_t keys;

    uint8_t ram[CH8_RAM_SIZE];
    uint8_t V[CH8_V_SIZE];

    uint16_t I;
    uint16_t PC;
    uint8_t SP;

    uint8_t delay_timer;
    uint8_t sound_timer;

    uint16_t stack[CH8_STACK_SIZE];
} Chip8;

typedef struct 
{
    uint16_t opcode;

    uint16_t addr;
    uint8_t kk;
    uint8_t x;
    uint8_t y;
    uint8_t n;
} Chip8Opcode;

typedef struct
{
    bool enable;

    Chip8* ch8;
    uint16_t history[CH8_DEBUG_HISTORY];
    int history_pos;
    
    int fps;
    double frame_ms;
    double cpu_ms;
} Chip8Debug;

typedef enum
{
    EXEC_CONTINUE,
    EXEC_WAIT_VBLANK
} Chip8ExecuteResult;

// ## HEADERS

int main(int argc, char** argv);
Chip8Config parse_args(int argc, char** argv);
Chip8EmulationConfig create_emulation_config(Chip8Config conf);
void chip8_main(Chip8EmulationConfig conf);
void chip8_cycle(Chip8* ch8, Chip8Debug* debug, int ipf);
Chip8 create_chip8_and_load_rom(const char* romPath);
void update_timer(Chip8 *ch8);
void update_keys(Chip8 *ch8);
void unknown_opcode_error(Chip8* ch8, uint16_t opcode);
void load_fontset_to_ram(Chip8* ch8);
Chip8Debug create_debug_info(bool enable, Chip8* ch8);
bool load_rom_to_ram(Chip8* ch8, const uint8_t* bytes, const long size);
Chip8 init_chip8();
bool file_to_bytes(const char* path, uint8_t** romBytes, long* romSize);
void print_display(Chip8* ch8, Chip8Debug debug);
void print_hex_array(uint8_t* bytes, long len);
void print_help();
void error_exit(const char* message);

Chip8ExecuteResult opcode_execute(Chip8* ch8, uint16_t opcode);
Chip8Opcode decode_opcode(uint16_t opcode);
void opcode_CLS(Chip8* ch8, Chip8Opcode op);
void opcode_RET(Chip8* ch8, Chip8Opcode op);
void opcode_SYS_addr(Chip8* ch8, Chip8Opcode op);
void opcode_JP_addr(Chip8* ch8, Chip8Opcode op);
void opcode_CALL_addr(Chip8* ch8, Chip8Opcode op);
void opcode_SE_Vx_byte(Chip8* ch8, Chip8Opcode op);
void opcode_SNE_Vx_byte(Chip8* ch8, Chip8Opcode op);
void opcode_SE_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_LD_Vx_byte(Chip8* ch8, Chip8Opcode op);
void opcode_ADD_Vx_byte(Chip8* ch8, Chip8Opcode op);
void opcode_LD_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_OR_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_AND_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_XOR_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_ADD_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_SUB_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_SHR_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_SUBN_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_SHL_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_SNE_Vx_Vy(Chip8* ch8, Chip8Opcode op);
void opcode_LD_I_addr(Chip8* ch8, Chip8Opcode op);
void opcode_JP_V0_addr(Chip8* ch8, Chip8Opcode op);
void opcode_RND_Vx_byte(Chip8* ch8, Chip8Opcode op);
void opcode_DRW_Vx_Vy_nibble(Chip8* ch8, Chip8Opcode op);
void opcode_SKP_Vx(Chip8* ch8, Chip8Opcode op);
void opcode_SKNP_Vx(Chip8* ch8, Chip8Opcode op);
void opcode_LD_Vx_DT(Chip8* ch8, Chip8Opcode op);
void opcode_LD_Vx_K(Chip8* ch8, Chip8Opcode op);
void opcode_LD_DT_Vx(Chip8* ch8, Chip8Opcode op);
void opcode_LD_ST_Vx(Chip8* ch8, Chip8Opcode op);
void opcode_ADD_I_Vx(Chip8* ch8, Chip8Opcode op);
void opcode_LD_F_Vx(Chip8* ch8, Chip8Opcode op);
void opcode_LD_B_Vx(Chip8* ch8, Chip8Opcode op);
void opcode_LD_ramI_Vx(Chip8* ch8, Chip8Opcode op);
void opcode_LD_Vx_ramI(Chip8* ch8, Chip8Opcode op);

// ## ENTRY POINT

int main(int argc, char** argv)
{
    init_terminal();
    init_audio();

    srand(time(NULL));

    Chip8Config cfg = parse_args(argc, argv);

    if (cfg.help || !cfg.rom_path) print_help();

    chip8_main(create_emulation_config(cfg));

    return 0;
}

Chip8Config parse_args(int argc, char** argv)
{
    Chip8Config cfg = {0};

    cfg.ipf = CH8_IPF_DEFAULT;

    for(int i = 1; i < argc; i++)
    {
        char* flag = argv[i];
        if (flag[0] == '-')
        {
            if (strcmp(flag, "-h") == 0) cfg.help = true;
            else if (strcmp(flag, "-d") == 0) cfg.debug = true;
            else if (strcmp(flag, "--mute") == 0) sound_mute = 1;
            else if (strcmp(flag, "--ipf") == 0)
            {
                if (i + 1 >= argc)
                    error_exit("--ipf requires a value");
                
                char *end;
                long value = strtol(argv[++i], &end, 10);

                if (*end != '\0')
                    error_exit("Invalid value for --ipf");

                cfg.ipf = (int)value;
            }
            else error_exit("Unknown option");
        }
        else
        {
            cfg.rom_path = flag;
        }
    }

    return cfg;
}

Chip8EmulationConfig create_emulation_config(Chip8Config conf)
{
    Chip8EmulationConfig econf = {0};

    econf.debug = conf.debug;
    econf.rom_path = conf.rom_path;
    econf.ipf = conf.ipf;

    return econf;
}

// ## CHIP-8 BEHAVIOR

void chip8_main(Chip8EmulationConfig conf)
{
    Chip8 ch8 = create_chip8_and_load_rom(conf.rom_path);
    Chip8Debug debug = create_debug_info(conf.debug, &ch8);
    
    clear_terminal();

    while (1)
    {
        chip8_cycle(&ch8, &debug, conf.ipf);
    }
}

Chip8 create_chip8_and_load_rom(const char* romPath)
{
    Chip8 ch8 = init_chip8();
    uint8_t* romBytes;
    long romSize;

    if (!file_to_bytes(romPath, &romBytes, &romSize)) error_exit("Can't read file");
    if (!load_rom_to_ram(&ch8, romBytes, romSize)) error_exit("RAM overflow");
    free(romBytes);

    load_fontset_to_ram(&ch8);

    return ch8;
}

Chip8 init_chip8()
{
    Chip8 ch8 =  {0};

    ch8.keys = 0;

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

Chip8Debug create_debug_info(bool enable, Chip8* ch8)
{
    Chip8Debug debug = {};

    debug.enable = enable;
    debug.ch8 = ch8;
    
    return debug;
}

// CHIP-8 cycle sequence:
// 1. Handle keys
// 2. Update timers
// 3. Execute instructions IPF times
// 4. Update display
// 5. Limit frame rate
void chip8_cycle(Chip8* ch8, Chip8Debug* debug, int ipf)
{
static bool wait_for_vblank;

    static int frame_count = 0;
    static double fps_timer = 0;
    static double timer_acc = 0;

    static const double timer_time = 1.0 / CH8_TIMER_HZ;
    static const double target_frame_time = 1.0 / CH8_FRAME_HZ;

    static double last = 0;

    // --- Frame timing ---

    double current = now();

    if (last == 0)
        last = current;

    double dt = current - last;
    last = current;

    double frame_start = current;

    // --- Input ---

    update_keys(ch8);

    // --- Timers ---

    timer_acc += dt;

    while (timer_acc >= timer_time)
    {
        update_timer(ch8);
        timer_acc -= timer_time;
    }

    // --- CPU ---

    double cpu_start = now();

    for (int i = 0; i < ipf; i++)
    {
        if (wait_for_vblank)
            break;

        uint16_t opcode = (ch8->ram[ch8->PC] << 8) | ch8->ram[ch8->PC + 1];
        ch8->PC += 2;

        if (opcode_execute(ch8, opcode) == EXEC_WAIT_VBLANK)
            wait_for_vblank = true;

        if (debug->enable)
        {
            debug->history[debug->history_pos] = opcode;
            debug->history_pos = (debug->history_pos + 1) % CH8_DEBUG_HISTORY;
        }
    }

    debug->cpu_ms = (now() - cpu_start) * 1000.0;

    // --- Display ---

    if (wait_for_vblank)
    {
        print_display(ch8, *debug);
        wait_for_vblank = false;
    }

    // --- Frame limiter ---

    double elapsed = now() - frame_start;
    double remaining = target_frame_time - elapsed;

    if (remaining > 0)
        delay((int)(remaining * 1000));

    debug->frame_ms = (now() - frame_start) * 1000.0;

    // --- FPS counter ---

    frame_count++;
    fps_timer += dt;

    if (fps_timer >= 1.0)
    {
        debug->fps = frame_count;
        frame_count = 0;
        fps_timer -= 1.0;
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

// ## EXECUTION OPTACODES ##

Chip8ExecuteResult opcode_execute(Chip8* ch8, uint16_t opcode)
{
    Chip8Opcode op = decode_opcode(opcode);
    switch (opcode & 0xF000)
    {
    case 0x0000:
        if (opcode == 0x00E0) // CLS
            opcode_CLS(ch8, op);
        else if (opcode == 0x00EE) // RET
            opcode_RET(ch8, op);
        else // SYS addr
            opcode_SYS_addr(ch8, op);
        break;
    case 0x1000: // JP addr
        opcode_JP_addr(ch8, op);
        break;
    case 0x2000: // CALL addr
        opcode_CALL_addr(ch8, op);
        break;
    case 0x3000: // SE Vx, byte
        opcode_SE_Vx_byte(ch8, op);
        break;
    case 0x4000: // SNE Vx, byte
        opcode_SNE_Vx_byte(ch8, op);
        break;
    case 0x5000: // SE Vx, Vy
        if (op.n != 0)
            unknown_opcode_error(ch8, opcode);
        opcode_SE_Vx_Vy(ch8, op);
        break;
    case 0x6000: // LD Vx, byte
        opcode_LD_Vx_byte(ch8, op);
        break;
    case 0x7000: // ADD Vx, byte
        opcode_ADD_Vx_byte(ch8, op);
        break;
    case 0x8000:
        switch (opcode & 0xF00F)
        {
        case 0x8000: // LD Vx, Vy
            opcode_LD_Vx_Vy(ch8, op);
            break;
        case 0x8001: // OR Vx, Vy
            opcode_OR_Vx_Vy(ch8, op);
            break;
        case 0x8002: // AND Vx, Vy
            opcode_AND_Vx_Vy(ch8, op);
            break;
        case 0x8003: // XOR Vx, Vy
            opcode_XOR_Vx_Vy(ch8, op);
            break;
        case 0x8004: // ADD Vx, Vy
            opcode_ADD_Vx_Vy(ch8, op);
            break;
        case 0x8005: // SUB Vx, Vy
            opcode_SUB_Vx_Vy(ch8, op);
            break;
        case 0x8006: // SHR Vx, Vy
            opcode_SHR_Vx_Vy(ch8, op);
            break;
        case 0x8007: // SUBN Vx, Vy
            opcode_SUBN_Vx_Vy(ch8, op);
            break;
        case 0x800E: // SHL Vx or SHR Vx, Vy
            opcode_SHL_Vx_Vy(ch8, op);
            break;
        default:
            unknown_opcode_error(ch8, opcode);
            break;
        }

        break;
    case 0x9000: // SNE Vx, Vy
        if (op.n != 0) 
            unknown_opcode_error(ch8, opcode);
        opcode_SNE_Vx_Vy(ch8, op);
        break;
    case 0xA000: // LD I, addr
        opcode_LD_I_addr(ch8, op);
        break;
    case 0xB000: // JP V0, addr
        opcode_JP_V0_addr(ch8, op);
        break;
    case 0xC000: // RND Vx, byte
        opcode_RND_Vx_byte(ch8, op);
        break;
    case 0xD000: // DRW Vx, Vy, nibble
        opcode_DRW_Vx_Vy_nibble(ch8, op);
        return EXEC_WAIT_VBLANK;
    case 0xE000:
        switch (opcode & 0xF0FF)
        {
            case 0xE09E: // SKP Vx
                opcode_SKP_Vx(ch8, op);
                break;
            case 0xE0A1: // SKNP Vx
                opcode_SKNP_Vx(ch8, op);
                break;
            default:
                unknown_opcode_error(ch8, opcode);
                break;
        }
        break;
    case 0xF000:
        switch (opcode & 0xF0FF) {
            case 0xF007: // LD Vx, DT
                opcode_LD_Vx_DT(ch8, op);
                break;
            case 0xF00A: // LD Vx, K
                opcode_LD_Vx_K(ch8, op);
                break;
            case 0xF015: // LD DT, Vx
                opcode_LD_DT_Vx(ch8, op);
                break;
            case 0xF018: // LD ST, Vx
                opcode_LD_ST_Vx(ch8, op);
                break;
            case 0xF01E: // ADD I, Vx
                opcode_ADD_I_Vx(ch8, op);
                break;
            case 0xF029: // LD F, Vx
                opcode_LD_F_Vx(ch8, op);
                break;
            case 0xF033: // LD B, Vx
                opcode_LD_B_Vx(ch8, op);
                break;
            case 0xF055: // LD [I], Vx
                opcode_LD_ramI_Vx(ch8, op);
                break;
            case 0xF065: // LD Vx, [I]
                opcode_LD_Vx_ramI(ch8, op);
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

    return EXEC_CONTINUE;
}

Chip8Opcode decode_opcode(uint16_t opcode)
{
    Chip8Opcode op = {};

    op.opcode = opcode;
    op.addr   = opcode & 0x0FFF;
    op.kk     = opcode & 0x00FF;
    op.x      = (opcode >> 8) & 0x0F;
    op.y      = (opcode >> 4) & 0x0F;
    op.n      = opcode & 0x000F;

    return op;
}

void opcode_CLS(Chip8* ch8, Chip8Opcode op)
{
    memset(ch8->display, 0, CH8_SCREEN_WIDTH * CH8_SCREEN_HEIGHT * sizeof(uint8_t));
}

void opcode_RET(Chip8* ch8, Chip8Opcode op)
{
    if (ch8->SP == 0) error_exit("RET called with empty stack");
    ch8->PC = ch8->stack[--ch8->SP];
}

void opcode_SYS_addr(Chip8* ch8, Chip8Opcode op)
{
    //ignored
}

void opcode_JP_addr(Chip8* ch8, Chip8Opcode op)
{
    ch8->PC = op.addr;
}

void opcode_CALL_addr(Chip8* ch8, Chip8Opcode op)
{
    if (ch8->SP >= CH8_STACK_SIZE) error_exit("SP overflow in CALL opcode");
    ch8->stack[ch8->SP] = ch8->PC;
    ch8->SP++;
    ch8->PC = op.addr;
}

void opcode_SE_Vx_byte(Chip8* ch8, Chip8Opcode op)
{
    if (ch8->V[op.x] == op.kk) ch8->PC += 2;
}

void opcode_SNE_Vx_byte(Chip8* ch8, Chip8Opcode op)
{
    if (ch8->V[op.x] != op.kk) ch8->PC += 2;
}

void opcode_SE_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    if (ch8->V[op.x] == ch8->V[op.y]) ch8->PC += 2;
}

void opcode_LD_Vx_byte(Chip8* ch8, Chip8Opcode op)
{
    ch8->V[op.x] = op.kk;
}

void opcode_ADD_Vx_byte(Chip8* ch8, Chip8Opcode op)
{
    ch8->V[op.x] += op.kk;
}

void opcode_LD_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    ch8->V[op.x] = ch8->V[op.y];
}

void opcode_OR_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    ch8->V[op.x] = ch8->V[op.x] | ch8->V[op.y];
    ch8->V[CH8_VF] = 0;
}

void opcode_AND_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    ch8->V[op.x] = ch8->V[op.x] & ch8->V[op.y];
    ch8->V[CH8_VF] = 0;
}

void opcode_XOR_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    ch8->V[op.x] = ch8->V[op.x] ^ ch8->V[op.y];
    ch8->V[CH8_VF] = 0;
}

void opcode_ADD_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    uint16_t sum = ch8->V[op.x] + ch8->V[op.y];
    ch8->V[op.x] = sum & 0xFF;
    ch8->V[CH8_VF] = (sum > 0xFF);
}

void opcode_SUB_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    uint8_t vx = ch8->V[op.x];
    uint8_t vy = ch8->V[op.y];
            
    ch8->V[op.x] = vx - vy;
    ch8->V[CH8_VF] = (vx >= vy);
}

void opcode_SHR_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    uint8_t vy = ch8->V[op.y];

    ch8->V[op.x] = vy >> 1;
    ch8->V[CH8_VF] = vy & 1;
}

void opcode_SUBN_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    uint8_t vx = ch8->V[op.x];
    uint8_t vy = ch8->V[op.y];
    
    ch8->V[op.x] = vy - vx;
    ch8->V[CH8_VF] = (vy >= vx);
}

void opcode_SHL_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    uint8_t vy = ch8->V[op.y];

    ch8->V[op.x] = vy << 1;
    ch8->V[CH8_VF] = vy >> 7;
}

void opcode_SNE_Vx_Vy(Chip8* ch8, Chip8Opcode op)
{
    if (ch8->V[op.x] != ch8->V[op.y]) ch8->PC += 2;
}

void opcode_LD_I_addr(Chip8* ch8, Chip8Opcode op)
{
    ch8->I = op.addr;
}

void opcode_JP_V0_addr(Chip8* ch8, Chip8Opcode op)
{
    ch8->PC = ch8->V[0] + op.addr;
}

void opcode_RND_Vx_byte(Chip8* ch8, Chip8Opcode op)
{
    ch8->V[op.x] = (rand() % 256) & op.kk;
}

void opcode_DRW_Vx_Vy_nibble(Chip8* ch8, Chip8Opcode op)
{
    uint8_t vx = ch8->V[op.x] % CH8_SCREEN_WIDTH;
    uint8_t vy = ch8->V[op.y] % CH8_SCREEN_HEIGHT;

    ch8->V[0xF] = 0;

    for (int i = 0; i < op.n; i++)
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

void opcode_SKP_Vx(Chip8* ch8, Chip8Opcode op)
{
    uint8_t key = ch8->V[op.x] & 0xF;
                
    if (ch8->keys & (1 << key))
        ch8->PC += 2;
}

void opcode_SKNP_Vx(Chip8* ch8, Chip8Opcode op)
{
    uint8_t key = ch8->V[op.x] & 0xF;
                
    if (!(ch8->keys & (1 << key)))
        ch8->PC += 2;
}

void opcode_LD_Vx_DT(Chip8* ch8, Chip8Opcode op)
{
    ch8->V[op.x] = ch8->delay_timer;
}

void opcode_LD_Vx_K(Chip8* ch8, Chip8Opcode op)
{
    static int waiting_key = -1;

    if (waiting_key == -1) 
    {
        for (int i = 0; i < 16; i++)
        {
            if (ch8->keys & (1 << i))
            {
                waiting_key = i;
                break;
            }
        }
        
        if (waiting_key == -1)
        {
            ch8->PC -= 2;
            return;
        }
    }

    if (ch8->keys & (1 << waiting_key))
    {
        ch8->PC -= 2;
        return;
    }

    ch8->V[op.x] = waiting_key;
    waiting_key = -1;
}

void opcode_LD_DT_Vx(Chip8* ch8, Chip8Opcode op)
{
    ch8->delay_timer = ch8->V[op.x];
}

void opcode_LD_ST_Vx(Chip8* ch8, Chip8Opcode op)
{
    ch8->sound_timer = ch8->V[op.x];
}

void opcode_ADD_I_Vx(Chip8* ch8, Chip8Opcode op)
{
    ch8->I += ch8->V[op.x];
}

void opcode_LD_F_Vx(Chip8* ch8, Chip8Opcode op)
{
    ch8->I = (ch8->V[op.x] & 0xF) * 5;
}

void opcode_LD_B_Vx(Chip8* ch8, Chip8Opcode op)
{
    uint8_t value = ch8->V[op.x];

    ch8->ram[(ch8->I) & 0xFFF]     = value / 100;
    ch8->ram[(ch8->I + 1) & 0xFFF] = (value / 10) % 10;
    ch8->ram[(ch8->I + 2) & 0xFFF] = value % 10;
}

void opcode_LD_ramI_Vx(Chip8* ch8, Chip8Opcode op)
{
    for (int i = 0; i <= op.x; i++)
        ch8->ram[(ch8->I + i) & 0xFFF] = ch8->V[i];
    ch8->I += op.x + 1;
}

void opcode_LD_Vx_ramI(Chip8* ch8, Chip8Opcode op)
{
    for (int i = 0; i <= op.x; i++)
        ch8->V[i] = ch8->ram[(ch8->I + i) & 0xFFF];
    ch8->I += op.x + 1;
}

// ## DISPLAY ##

void print_display(Chip8* ch8, Chip8Debug debug)
{
    reset_screen();

    static char buffer[CH8_LINE_LEN * CH8_SCREEN_HEIGHT + 1];
    char* p = buffer;

    for (int y = 0; y < CH8_SCREEN_HEIGHT; y += 2)
    {
        for (int x = 0; x < CH8_SCREEN_WIDTH; x++)
        {
            uint8_t pixel_up = ch8->display[y * CH8_SCREEN_WIDTH + x];
            uint8_t pixel_down = ch8->display[(y+1) * CH8_SCREEN_WIDTH + x];
            const char *symbol = CH8_SYMBOL_FREE;

            if (pixel_up && !pixel_down)
                symbol = CH8_SYMBOL_FILL_UP;
            else if (!pixel_up && pixel_down)
                symbol = CH8_SYMBOL_FILL_DOWN;
            else if (pixel_up && pixel_down)
                symbol = CH8_SYMBOL_FILL;

            size_t len = strlen(symbol);

            memcpy(p, symbol, len);
            p += len;
        }

        *p++ = ' ';
        *p++ = ' ';

        if (debug.enable)
        {
            int line = y / 2;

            if (line < CH8_DEBUG_HISTORY)
            {
                int visible = CH8_SCREEN_HEIGHT / 2;
                const char *arrow = (line == visible - 1) ? "→" : " ";

                size_t len = strlen(arrow);
                memcpy(p, arrow, len);
                p += len;
                *p++ = ' ';

                int idx = (debug.history_pos + line) % CH8_DEBUG_HISTORY;
                p += sprintf(p, "%04X", debug.history[idx]);
            }
        }

        *p++ = '\n';
    }

    if (debug.enable)
    {
        p += sprintf(p, "FPS: %i  Frame: %.2fms  CPU: %.2fms", debug.fps, debug.frame_ms, debug.cpu_ms);
        p += sprintf(p, "\n");
        p += sprintf(p, "PC: %03X  ", ch8->PC);
        p += sprintf(p, "I: %03X  ", ch8->I);
        p += sprintf(p, "SP: %02X  ", ch8->SP);
        p += sprintf(p, "DT: %02X  ", ch8->delay_timer);
        p += sprintf(p, "ST: %02X  ", ch8->sound_timer);
        p += sprintf(p,
        "\n"
        "V0:%02X V1:%02X V2:%02X V3:%02X\n"
        "V4:%02X V5:%02X V6:%02X V7:%02X\n"
        "V8:%02X V9:%02X VA:%02X VB:%02X\n"
        "VC:%02X VD:%02X VE:%02X VF:%02X\n",
        ch8->V[0], ch8->V[1], ch8->V[2], ch8->V[3],
        ch8->V[4], ch8->V[5], ch8->V[6], ch8->V[7],
        ch8->V[8], ch8->V[9], ch8->V[10], ch8->V[11],
        ch8->V[12], ch8->V[13], ch8->V[14], ch8->V[15]);

    }
    *p = '\0';

    fputs(buffer, stdout);
    fflush(stdout);
}

// ## OTHER ##

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

void error_exit(const char* message)
{
    print_color_text(message, CONSOLE_RED);
    exit(1);
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
        "    --ipf <num>               Set Instructions Per Frame\n"
        "    --mute                    Off audio\n"
    );
    exit(0);
}

void print_hex_array(uint8_t* bytes, long len)
{
    for (int i = 0; i < len; i++)
        printf("%02X ", (uint8_t)bytes[i]);
}
