/*
 * The menu's text in the chosen language. See menu_text.h.
 */
#include "menu_text.h"
#include "menu_text_data.h"
#include <string.h>

static int language = MENU_LANGUAGE_ENGLISH;

void menu_text_language(int chosen)
{
    language = chosen == MENU_LANGUAGE_UKRAINIAN ? chosen : MENU_LANGUAGE_ENGLISH;
}

int menu_text_current(void)
{
    return language;
}

const char *menu_text(const char *english)
{
    size_t low = 0, high = MENU_TEXT_COUNT;
    if (language == MENU_LANGUAGE_ENGLISH || !english)
        return english;
    /* The table is sorted by the English text's bytes (strcmp order). */
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        int order = strcmp(menu_text_english[mid], english);
        if (!order)
            return menu_text_ukrainian[mid];
        if (order < 0)
            low = mid + 1;
        else
            high = mid;
    }
    return english;
}
