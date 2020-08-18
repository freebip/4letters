#ifndef __FOUR_LETTERS_H__
#define __FOUR_LETTERS_H__

#include "libbip.h"

typedef unsigned short word;
typedef unsigned char byte;

struct game_state_t
{
    // ��������� ����� �� ������� ����
    byte letters[4];
    // ��������� ������������� �����
    byte selected_letters[4];
    // ��������� ������������� �������
    byte selected_positions[4];
    // ���-�� ��������� ������������� ����
    byte selected_letter_count;
    // ��������� ���������� ����
    byte words;
    // ������� ���-�� �����
    word score;
    // �������� �������
    word time;
};

struct appdata_t
{
    Elf_proc_* proc;
    void* ret_f;
    int ticks;
    int current_screen;
    struct game_state_t game_state;
    int is_valid_word;
    int valid_word_index;
    unsigned short max_score;
    unsigned long randseed;
};

void show_screen(void* return_screen);
void keypress_screen();
int dispatch_screen(void* p);
void screen_job();
void draw_screen();

// presets

#define GAME_TIMEOVER 176 
#define RIGHT_ANSWER_ADD_TIME 88
#define SHOW_RIGHT_WRONG_ANSWER_TIME 10
#define SHOW_TIMEOVER_PLATE_TIME 30

// game state's

#define GAME_STATE_START         0
#define GAME_STATE_PLAY           1
#define GAME_STATE_GAME_OVER  2

// res's

#define RES_SCORE 66
#define RES_WORDS 67
#define RES_TITLE 78
#define RES_START 79
#define RES_DIGIT_START 68
#define RES_TIMELEFT 80
#define RES_AGAIN 81

#endif
