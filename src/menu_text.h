/*
 * The menu's text in the chosen language.
 *
 * The menu's text is written in English in the source. Every piece of text
 * people see is marked there: T("Play") where it is used, or N_("Play")
 * where it only sits in a table (and the code that shows it passes it
 * through T later). tools/build-menu-text.py collects the marked text,
 * checks every piece has a translation in data/menu-text-uk.txt (with the
 * same %d, %s... in the same order) and generates menu_text_data.h, a
 * sorted table of English text and its translation.
 *
 * T looks the English up while a translation is chosen, and otherwise
 * returns it unchanged. Text that is not marked (file names, game names,
 * the diagnostics file) is never translated. All text is UTF-8.
 */
#ifndef OH_MY_3DO_MENU_TEXT_H
#define OH_MY_3DO_MENU_TEXT_H

/* The languages, as the language setting stores them (menu_config.h). */
#define MENU_LANGUAGE_ENGLISH   0
#define MENU_LANGUAGE_UKRAINIAN 1
#define MENU_LANGUAGE_LAST      MENU_LANGUAGE_UKRAINIAN

/* Chooses the language T uses from now on; an unknown one means English. */
void menu_text_language(int language);

/* The language T uses now. */
int menu_text_current(void);

/* `english` in the current language: its translation, or `english` itself
 * if there is none (or English is chosen). */
const char *menu_text(const char *english);

#define T(english) menu_text(english)
/* Marks text in a table for translation; the code showing it uses T. */
#define N_(english) english

#endif
