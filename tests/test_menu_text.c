#include "menu_text.h"
#include "menu_text_data.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    size_t i;
    const char *play = "A:Play";

    /* The table is in strcmp order (binary search relies on it), every
     * entry has a translation, and none is translated to itself by accident
     * of an empty string. */
    for (i = 1; i < MENU_TEXT_COUNT; ++i)
        assert(strcmp(menu_text_english[i - 1], menu_text_english[i]) < 0);
    for (i = 0; i < MENU_TEXT_COUNT; ++i)
        assert(*menu_text_ukrainian[i]);

    /* English, the default: text comes back unchanged, as the same pointer. */
    assert(menu_text_current() == MENU_LANGUAGE_ENGLISH);
    assert(T(play) == play);
    assert(T(NULL) == NULL);

    /* Ukrainian: marked text is translated, anything else is left alone. */
    menu_text_language(MENU_LANGUAGE_UKRAINIAN);
    assert(menu_text_current() == MENU_LANGUAGE_UKRAINIAN);
    assert(!strcmp(T(play), "A:\320\223\321\200\320\260\321\202\320\270")); /* A:Грати */
    assert(!strcmp(T("Settings"), "\320\235\320\260\320\273\320\260\321\210\321\202\321\203\320\262"
                                  "\320\260\320\275\320\275\321\217")); /* Налаштування */
    assert(!strcmp(T("Road Rash.iso"), "Road Rash.iso"));
    for (i = 0; i < MENU_TEXT_COUNT; ++i)
        assert(T(menu_text_english[i]) == menu_text_ukrainian[i]);
    /* Translated text is not translated again (messages pass through T twice). */
    assert(T(T(play)) == T(play));

    /* An unknown language falls back to English. */
    menu_text_language(7);
    assert(menu_text_current() == MENU_LANGUAGE_ENGLISH && T(play) == play);
    puts("Menu text: sorted table, Ukrainian lookup, untranslated and English pass-through passed");
    return 0;
}
