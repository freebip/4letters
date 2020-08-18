#include "libbip.h"
#include "four_letters.h"
#include "words.h"

byte _bytes_title[] = { 0xc1, 0xd3, 0xca, 0xc2, 0xdb, 0x00 };
byte _bytes_record[] = { 0xd0, 0xc5, 0xca, 0xce, 0xd0, 0xc4, 0x00 };
byte _bytes_word[] = { 0xd1, 0xcb, 0xce, 0xc2, 0xce, 0x00 };
byte _bytes_scores[] = { 0xCe, 0xd7, 0xca, 0xce, 0xc2, 0x00 };

struct regmenu_ menu_screen = { 55, 1, 0, dispatch_screen, keypress_screen, screen_job, 0, show_screen, 0, 0 };
struct appdata_t** appdata_p;
struct appdata_t* appdata;

int main(int p, char** a)
{
    show_screen((void*)p);
}

unsigned short randint(short max)
{
    appdata->randseed = appdata->randseed * get_tick_count();
    appdata->randseed++;
    return ((appdata->randseed >> 16) * max) >> 16;
}

void show_screen(void* p)
{
    appdata_p = (struct appdata_t **)get_ptr_temp_buf_2();

    if ((p == *appdata_p) && get_var_menu_overlay()) {
        appdata = *appdata_p;
        *appdata_p = (struct appdata_t *)NULL;
        reg_menu(&menu_screen, 0);
        *appdata_p = appdata;
    }
    else {
        reg_menu(&menu_screen, 0);
        *appdata_p = (struct appdata_t*)pvPortMalloc(sizeof(struct appdata_t));
        appdata = *appdata_p;
        _memclr(appdata, sizeof(struct appdata_t));
        appdata->proc = (Elf_proc_*)p;

        appdata->randseed = get_tick_count();
    }

    if (p && appdata->proc->elf_finish)
        appdata->ret_f = appdata->proc->elf_finish;
    else
        appdata->ret_f = show_watchface;

    draw_screen();

    // не выключаем экран, не выключаем подсветку
    set_display_state_value(8, 1);  
    set_display_state_value(4, 1);
    set_display_state_value(2, 0);

    set_update_period(1, 100);
}

void mix_letters(byte* res)
{
    for (int i = 3; i >= 1; i--)
    {
        int j = randint(37) % (i + 1);
        byte tmp = res[j];
        res[j] = res[i];
        res[i] = tmp;
    }
}

int get_possible_words_count(byte *letters)
{
    int count = 0;
    byte temp[4];
    for (int i = 0; i < sizeof(_words) >> 2; i++)
    {
        _memcpy(temp, &_words[i<<2], 4);

        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
                if (letters[j] == temp[k])
                {
                    temp[k] = 0;
                    break;
                }
        }

        if (!(temp[0] + temp[1] + temp[2] + temp[3]))
            count++;
    }
    return count;
}

void set_random_word()
{
    appdata->valid_word_index = randint(sizeof(_words) >> 2);
    _memcpy(&appdata->game_state.letters, &_words[appdata->valid_word_index << 2], 4);
    mix_letters(appdata->game_state.letters);
    appdata->game_state.words = get_possible_words_count(appdata->game_state.letters);
    appdata->game_state.selected_letter_count = 0;
}

void game_state_process()
{
    appdata->ticks++;

    switch (appdata->current_screen)
    {
        case GAME_STATE_PLAY:
            if (appdata->game_state.selected_letter_count == 4)
            {
                if (appdata->ticks > SHOW_RIGHT_WRONG_ANSWER_TIME)
                {
                    appdata->game_state.selected_letter_count = 0;

                    if (appdata->is_valid_word)
                    {
                        appdata->game_state.score++;
                        if (appdata->game_state.time + RIGHT_ANSWER_ADD_TIME > GAME_TIMEOVER)
                            appdata->game_state.time = GAME_TIMEOVER;
                        else
                            appdata->game_state.time += RIGHT_ANSWER_ADD_TIME;

                        set_random_word();
                    }
                    else
                    {
                        appdata->game_state.selected_letter_count = 0;
                    }
                }
            }
            else
            {
                if (appdata->game_state.time == 0)
                {
                    if (appdata->ticks > SHOW_TIMEOVER_PLATE_TIME)
                        appdata->current_screen = GAME_STATE_GAME_OVER;
                }
                else
                {
                    appdata->game_state.time--;
                    if (appdata->game_state.time == 0)
                    {
                        appdata->ticks = 0;

                        if (appdata->is_valid_word && appdata->game_state.score > appdata->max_score)
                            ElfWriteSettings(ELF_INDEX_SELF, &appdata->game_state.score, 0, 2);
                    }
                }
            }
            break;
    }

}

void screen_job()
{
    draw_screen();
    repaint_screen_lines(0, 176);
    game_state_process();
    set_update_period(1, 100);
}

int is_exist_word(byte* word)
{
    for (int i = 0; i < sizeof(_words); i+=4)
    {
        int res = _memcmp(word, &_words[i], 4);
        if (!res)
            return 1;
    }
    return 0;
}

int is_index_selected(int index)
{
    for (int i = 0; i < appdata->game_state.selected_letter_count; i++)
    {
        if (appdata->game_state.selected_positions[i] == index)
            return 1;
    }
    return 0;
}

int dispatch_screen(void* p)
{
    struct gesture_* gest = (struct gesture_*)p;

    if( gest->gesture == GESTURE_CLICK)
    {
        switch (appdata->current_screen)
        {
        case GAME_STATE_START:
        case GAME_STATE_GAME_OVER:
            if (gest->touch_pos_y >= 100 && gest->touch_pos_y <= 147)
            {
                set_random_word();
                appdata->game_state.selected_letter_count = 0;
                appdata->game_state.score = 0;
                appdata->game_state.time = GAME_TIMEOVER;
                appdata->current_screen = GAME_STATE_PLAY;
            }
            break;
        case GAME_STATE_PLAY:
            if (appdata->game_state.selected_letter_count == 4)
                break;
            if (appdata->game_state.time == 0)
                break;

            if (gest->touch_pos_y < 44)
                break;
            int index = 0;
            if (gest->touch_pos_y < 106)
            {
                if (gest->touch_pos_x < 86)
                {
                    index = 0;
                }
                else
                {
                    index = 1;
                }
            }
            else
            {
                if (gest->touch_pos_x < 86)
                {
                    index = 2;
                }
                else
                {
                    index = 3;
                }
            }

            if (is_index_selected(index))
                break;

            appdata->game_state.selected_positions[appdata->game_state.selected_letter_count] = index;
            appdata->game_state.selected_letters[appdata->game_state.selected_letter_count] = appdata->game_state.letters[index];
            appdata->game_state.selected_letter_count++;

            if (appdata->game_state.selected_letter_count == 4)
            {
                appdata->ticks = 0;
                appdata->is_valid_word = is_exist_word(appdata->game_state.selected_letters);
            }

            break;
        }
    }
    else if (gest->gesture == GESTURE_SWIPE_LEFT && appdata->current_screen == GAME_STATE_PLAY)
    {
        if (appdata->game_state.selected_letter_count > 0)
            appdata->game_state.selected_letter_count--;
    }

    return 0;
}

void keypress_screen()
{
    show_menu_animate(appdata->ret_f, (unsigned int)show_screen, ANIMATE_RIGHT);
};

void print_digits(int value, int x, int y, int spacing)
{
    struct res_params_ res_params;

    int max = 100000;

    while (max)
    {
        if (value / max)
            break;
        max = max / 10;
    }

    do
    {
        int mm = max == 0 ? 0 : value / max;

        get_res_params(ELF_INDEX_SELF, RES_DIGIT_START+mm, &res_params);
        show_elf_res_by_id(ELF_INDEX_SELF, RES_DIGIT_START + mm, x, y);
        x += res_params.width + spacing;
        if (max == 0)
            break;

        value = value % max;
        max = max / 10;
    } while (max);
}

int get_symbols_width(byte* value, int spacing, int is_big)
{
    int x = 0;
    while ((byte)*value)
    {
        int res = -1;
        for (int i = 0; i < sizeof(letter_res) / 3; i++)
        {
            if (letter_res[3 * i] == *value)
            {
                res = is_big ? letter_res[3 * i + 1] : letter_res[3 * i + 2];
                value++;
                break;
            }
        }

        if (res == -1)
            break;

        struct res_params_ res_params;
        get_res_params(ELF_INDEX_SELF, res, &res_params);
        x += res_params.width + spacing;
    }
    return x > 0 ? x - spacing : 0;
}

void print_symbols(byte* value, int x, int y, int spacing, int is_big)
{
    while ((byte)*value)
    {
        int res = -1;
        for (int i = 0; i < sizeof(letter_res) / 3; i++)
        {
            if (letter_res[3 * i] == *value)
            {
                res = is_big ? letter_res[3 * i + 1] : letter_res[3 * i + 2];
                value++;
                break;
            }
        }

        if (res == -1)
            break;

        struct res_params_ res_params;
        get_res_params(ELF_INDEX_SELF, res, &res_params);
        show_elf_res_by_id(ELF_INDEX_SELF, res, x, y);
        x += res_params.width + spacing;
    }
}

int get_height_offset(byte b, int is_big)
{

    switch (b)
    {
    case 0xa8:
        return is_big?-7:-3;
    case 0xc9:
        return is_big?-10:-4;
    }
    return 0;
}

void draw_screen()
{
    int xrand, yrand, width;
    byte sym[5];

    set_graph_callback_to_ram_1();
    load_font();

    set_bg_color(COLOR_BLACK);
    fill_screen_bg();

    switch (appdata->current_screen)
    {
        case GAME_STATE_START:
            show_elf_res_by_id(ELF_INDEX_SELF, RES_TITLE, 55, 0);
            print_symbols(_bytes_title, 12, 55, 5, true);
            print_symbols(_bytes_record, 5, 155, 1, false);

            ElfReadSettings(ELF_INDEX_SELF, &appdata->max_score, 0, 2);
            
            print_digits(appdata->max_score, 85, 157, 2);

            set_fg_color(COLOR_BLUE);
            draw_filled_rect(0, 100, 176, 147);

            xrand = randint(2);
            yrand = randint(2);

            show_elf_res_by_id(ELF_INDEX_SELF, RES_START, xrand-1+35, yrand-1+110);

            break;

        case GAME_STATE_PLAY:
            set_bg_color(COLOR_BLACK);
            fill_screen_bg();

            show_elf_res_by_id(ELF_INDEX_SELF, RES_SCORE, 5, 5);
            print_digits(appdata->game_state.score, 51, 5, 2);
            show_elf_res_by_id(ELF_INDEX_SELF, RES_WORDS, 110, 5);
            print_digits(appdata->game_state.words, 154, 5, 2);

            if (appdata->game_state.selected_letter_count > 0)
            {
                for (int i = 0; i < appdata->game_state.selected_letter_count; i++)
                {
                    sym[i] = appdata->game_state.selected_letters[i];
                }
                sym[appdata->game_state.selected_letter_count] = 0;

                int xoffset = 0;
                int spacing = 3;
                if (appdata->game_state.selected_letter_count == 4 && !appdata->is_valid_word)
                {
                    xoffset = appdata->ticks % 2 ? -3 : 3;
                }
                if (appdata->game_state.selected_letter_count == 4 && appdata->is_valid_word)
                {
                    spacing += 2 * (appdata->ticks <= SHOW_RIGHT_WRONG_ANSWER_TIME/2 ? appdata->ticks : SHOW_RIGHT_WRONG_ANSWER_TIME - appdata->ticks);
                }
                width = get_symbols_width(sym, spacing, false);
                print_symbols(sym, 86 - width / 2 + xoffset, 23, spacing, false);
            }

            set_fg_color(is_index_selected(0) ? COLOR_YELLOW : COLOR_BLUE);
            draw_filled_rect(0, 44, 85, 108);
            sym[0] = appdata->game_state.letters[0];
            sym[1] = 0;
            width = get_symbols_width(sym, 0, true);
            print_symbols(sym, 43 - width / 2, 60 + get_height_offset(sym[0], true), 0, true);

            set_fg_color(is_index_selected(1) ? COLOR_YELLOW : COLOR_BLUE);
            draw_filled_rect(87, 44, 176, 108);
            sym[0] = appdata->game_state.letters[1];
            width = get_symbols_width(sym, 0, true);
            print_symbols(sym, 129 - width / 2, 60 + get_height_offset(sym[0], true), 0, true);

            set_fg_color(is_index_selected(2) ? COLOR_YELLOW : COLOR_BLUE);
            draw_filled_rect(0, 110, 85, 172);
            sym[0] = appdata->game_state.letters[2];
            width = get_symbols_width(sym, 0, true);
            print_symbols(sym, 43 - width / 2, 126 + get_height_offset(sym[0], true), 0, true);


            set_fg_color(is_index_selected(3) ? COLOR_YELLOW : COLOR_BLUE);
            draw_filled_rect(87, 110, 176, 172);
            sym[0] = appdata->game_state.letters[3];
            width = get_symbols_width(sym, 0, true);
            print_symbols(sym, 129 - width / 2, 126 + get_height_offset(sym[0], true), 0, true);

            set_fg_color(COLOR_WHITE);

            draw_filled_rect(0, 172, appdata->game_state.time, 176);

            if (appdata->game_state.time == 0)
            {
                show_elf_res_by_id(ELF_INDEX_SELF, RES_TIMELEFT, 23, 75 + appdata->ticks);
            }

            break;

        case GAME_STATE_GAME_OVER:
            set_bg_color(COLOR_BLACK);
            fill_screen_bg();
            print_symbols(_bytes_word, 55, 15, 2, false);

            _memcpy(sym, &_words[appdata->valid_word_index << 2], 4);
            sym[4] = 0;
            width = get_symbols_width(sym, 5, true);
            int x = 86 - width / 2;
            for (int i = 0; i < 4; i++)
            {
                sym[0] = _words[(appdata->valid_word_index << 2) + i];
                sym[1] = 0;
                print_symbols(sym, x, 45 + get_height_offset(sym[0], true), 5, true);
                width = get_symbols_width(sym, 5, true);
                x += width + 5;
            }

            print_symbols(_bytes_scores, 5, 155, 2, false);

            set_fg_color(COLOR_BLUE);
            draw_filled_rect(0, 100, 176, 147);

            xrand = randint(2);
            yrand = randint(2);

            show_elf_res_by_id(ELF_INDEX_SELF, RES_AGAIN, xrand - 1 + 35, yrand - 1 + 110);
            print_digits(appdata->game_state.score, 80, 157, 2);

            break;
    }


}
