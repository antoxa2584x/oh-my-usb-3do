/*
 * The help text for every screen, and the logic that decides when to show
 * it. See menu_help.h.
 *
 * Each screen has a title and up to MENU_HELP_ROWS lines; lines left out at
 * the end of a list are shown blank. The tables are indexed by
 * MenuHelpTopic, so their order must match the enum in menu_help.h. The
 * text is English, marked for translation (menu_text.h); the functions below
 * return it in the chosen language.
 */
#include "menu_help.h"
#include "menu_text.h"

static const char *const titles[HELP_TOPIC_COUNT] = {
    N_("Game library"),            /* HELP_LIBRARY */
    N_("Disc queue"),              /* HELP_QUEUE */
    N_("Alphabet strip"),          /* HELP_ALPHABET */
    N_("Tools"),                   /* HELP_TOOLS */
    N_("Console saves"),           /* HELP_SAVES */
    N_("USB backup slots"),        /* HELP_SLOTS */
    N_("Choose destination slot"), /* HELP_DESTINATION */
    N_("Backup contents"),         /* HELP_ARCHIVE */
    N_("Hex editor"),              /* HELP_HEX */
    N_("Editing a byte"),          /* HELP_HEX_BYTE */
    N_("Known values & cheats"),   /* HELP_CHEATS */
    N_("Settings"),                /* HELP_SETTINGS */
    N_("Category visibility"),     /* HELP_CATEGORIES */
    N_("Text keyboard"),           /* HELP_KEYBOARD */
    N_("Favorites"),               /* HELP_FAVORITES */
    N_("Recent games"),            /* HELP_RECENT */
    N_("Search results"),          /* HELP_RESULTS */
    N_("Compare backups"),         /* HELP_COMPARE */
    N_("Byte differences"),        /* HELP_COMPARE_BYTES */
    N_("Confirm action"),          /* HELP_CONFIRM */
    N_("Diagnostics"),             /* HELP_DIAGNOSTICS */
    N_("Game details"),            /* HELP_DETAILS */
    N_("Reorder favorites"),       /* HELP_FAVORITE_MOVE */
    N_("Jump to slot"),            /* HELP_SLOT_JUMP */
    N_("General controls"),        /* HELP_GENERAL */
    N_("Find backup slots"),       /* HELP_SLOT_FIND */
    N_("Review backup contents"),  /* HELP_BACKUP_DETAILS */
    N_("Choose comparison slot"),  /* HELP_COMPARE_PICK */
};

static const char *const lines[HELP_TOPIC_COUNT][MENU_HELP_ROWS] = {
    /* HELP_LIBRARY: Game library */
    {
        N_("Up/Down: Select a game"),
        N_("Left/Right or L/R: Page"),
        N_("A: Open folder / play game"),
        N_("B: Parent folder"),
        N_("C: Add selected disc to queue"),
        N_("Tap P: Queue; Hold C + P: Details"),
        N_("Tap X: Cover/video or refresh"),
        N_("Hold X: Full-screen media"),
        N_("Hold C + Up: Alphabet strip"),
        N_("L+R together: Tools"),
    },
    /* HELP_QUEUE: Disc queue */
    {
        N_("Up/Down: Select a queued disc"),
        N_("A or tap P: Play queue"),
        N_("B: Back (or finish moving)"),
        N_("C: Toggle disc move mode"),
        N_("Move mode: Up/Down reorders"),
        N_("X: Remove selected disc"),
        N_("Hold C + X: Clear queue"),
        N_("The queue is for one multi-disc game."),
        N_("L+R together: Tools"),
    },
    /* HELP_ALPHABET: Alphabet strip */
    {
        N_("Left/Right: Choose a letter"),
        N_("A: Jump to that letter"),
        N_("Down: Return to game list"),
        N_("B: Cancel letter navigation"),
        N_("Only available letters can be chosen."),
    },
    /* HELP_TOOLS: Tools */
    {
        N_("Arrows: Choose a tool"),
        N_("A: Open selected tool"),
        N_("Hold C + P in library: Game details"),
        N_("B: Return to library"),
        N_("L+R: Return to library"),
        N_("Hold P on any menu: Screen help"),
        N_("Tap P keeps its normal action."),
    },
    /* HELP_SAVES: Console saves */
    {
        N_("Up/Down: Select an NVRAM save"),
        N_("A: Back up only selected save"),
        N_("C: Back up the entire NVRAM"),
        N_("X: Refresh console save list"),
        N_("B: Back"),
        N_("A single-save backup merges into slot."),
        N_("A full backup replaces the slot."),
        N_("Left/Right: Switch to USB backups"),
    },
    /* HELP_SLOTS: USB backup slots */
    {
        N_("Up/Down: Select a slot"),
        N_("L/R: Slot pages; Left/Right: View"),
        N_("A: Open selected slot"),
        N_("C: Name the slot"),
        N_("X: Compare with another slot"),
        N_("Tap P: Find slots / jump by number"),
        N_("B: Back"),
        N_("Slot search uses the drive catalog."),
    },
    /* HELP_DESTINATION: Choose destination slot */
    {
        N_("Up/Down: Choose destination slot"),
        N_("L/R: Page through destination slots"),
        N_("A: Review and confirm destination"),
        N_("Tap P: Find slots / jump by number"),
        N_("B: Cancel without writing"),
        N_("Single saves merge by filename."),
        N_("Full backups replace the whole slot."),
        N_("Review the confirmation before A."),
    },
    /* HELP_ARCHIVE: Backup contents */
    {
        N_("Up/Down: Select a save"),
        N_("A: Restore selected save to NVRAM"),
        N_("C: Open raw hex editor"),
        N_("P: Copy selected save to another slot"),
        N_("X: Open known cheats for this save"),
        N_("B: Return to slot list"),
        N_("Restore keeps a recovery backup."),
    },
    /* HELP_HEX: Hex editor */
    {
        N_("Arrows: Move between bytes"),
        N_("L/R: Page through bytes"),
        N_("A: Edit selected byte"),
        N_("C: Save edited save into a slot"),
        N_("B: Back / review unsaved edits"),
        N_("Edits stay in the USB draft."),
        N_("Raw edits may need game checksums."),
        N_("X: Open known cheats"),
    },
    /* HELP_HEX_BYTE: Editing a byte */
    {
        N_("Left/Right: Choose high/low nibble"),
        N_("Up/Down: Change nibble value"),
        N_("A: Apply byte to draft"),
        N_("B: Cancel this byte edit"),
        N_("Applying does not write the slot."),
        N_("Use C outside byte edit to save."),
    },
    /* HELP_CHEATS: Known values & cheats */
    {
        N_("Up/Down: Select preset or field"),
        N_("A: Apply selected preset"),
        N_("Left/Right: Change field value"),
        N_("C: Save edited save into a slot"),
        N_("X: Switch to raw hex editor"),
        N_("B: Back"),
        N_("Changes remain in the USB draft."),
        N_("Verified profiles maintain checksums."),
    },
    /* HELP_SETTINGS: Settings */
    {
        N_("Up/Down: Choose a setting"),
        N_("A: Toggle / open selected setting"),
        N_("Left/Right: Adjust selected value"),
        N_("C: Save settings to USB"),
        N_("X: Discard unsaved settings"),
        N_("B: Back"),
        N_("Categories can be shown or hidden."),
    },
    /* HELP_CATEGORIES: Category visibility */
    {
        N_("Up/Down: Choose a category"),
        N_("A: Toggle shown / hidden"),
        N_("B: Return to Settings"),
        N_("Visibility saves immediately."),
        N_("Hidden games remain on the drive."),
    },
    /* HELP_KEYBOARD: Text keyboard */
    {
        N_("Arrows: Choose a character"),
        N_("A: Add selected character"),
        N_("X: Erase last character"),
        N_("C: Clear text"),
        N_("Tap P: Search or save this text"),
        N_("B: Cancel / back"),
        N_("Hold P: Help without submitting."),
    },
    /* HELP_FAVORITES: Favorites */
    {
        N_("Up/Down: Select favorite"),
        N_("A: Open its containing game folder"),
        N_("C: Add current browser game"),
        N_("X: Remove selected favorite"),
        N_("R: Reorder favorites; P: Reload"),
        N_("B: Back"),
    },
    /* HELP_RECENT: Recent games */
    {
        N_("Up/Down: Select a recent game"),
        N_("A: Open its containing folder"),
        N_("P: Reload recent list"),
        N_("B: Back"),
    },
    /* HELP_RESULTS: Search results */
    {
        N_("Up/Down: Select a result"),
        N_("A: Open its containing folder"),
        N_("B: Return to search"),
    },
    /* HELP_COMPARE: Compare backups */
    {
        N_("Up/Down: Select a save difference"),
        N_("A: Show changed byte values"),
        N_("B: Return to slot list"),
        N_("Added/removed are relative to first slot."),
        N_("Comparing does not change saves."),
    },
    /* HELP_COMPARE_BYTES: Byte differences */
    {
        N_("Left/Right: Page byte differences"),
        N_("B: Return to comparison list"),
        N_("-- means the byte is absent."),
        N_("Comparing does not change saves."),
    },
    /* HELP_CONFIRM: Confirm action */
    {
        N_("A: Confirm the displayed operation"),
        N_("B: Cancel without writing"),
        N_("Read the destination and scope."),
        N_("Help does not confirm the operation."),
    },
    /* HELP_DIAGNOSTICS: Diagnostics */
    {
        N_("Left/Right: More diagnostic pages"),
        N_("C: Export diagnostics to USB"),
        N_("X: Test return to the library"),
        N_("B: Back"),
        N_("Export needs a working connection."),
        N_("A transport timeout needs a reboot."),
    },
    /* HELP_DETAILS: Game details */
    {
        N_("Left/Right: More detail pages"),
        N_("B: Return to previous menu"),
        N_("Details do not change game files."),
    },
    /* HELP_FAVORITE_MOVE: Reorder favorites */
    {
        N_("Up/Down: Move selected favorite"),
        N_("A or R: Save new order"),
        N_("B: Cancel reordering"),
    },
    /* HELP_SLOT_JUMP: Jump to slot */
    {
        N_("Left/Right: Choose a digit"),
        N_("A: Add digit to slot number"),
        N_("X: Erase last digit"),
        N_("C: Clear number"),
        N_("Tap P: Go to that slot"),
        N_("B: Cancel"),
        N_("Hold P: Help without jumping."),
    },
    /* HELP_GENERAL: General controls */
    {
        N_("Left/Right: More help pages"),
        N_("B: Return to Tools"),
        N_("Hold P: Help for the current screen"),
        N_("Tap P keeps its normal action."),
    },
    /* HELP_SLOT_FIND: Find backup slots */
    {
        N_("Up/Down: Choose a search option"),
        N_("A: Open selected option"),
        N_("B: Return to slots"),
        N_("Search uses the drive save catalog."),
    },
    /* HELP_BACKUP_DETAILS: Review backup contents */
    {
        N_("Up/Down: Review saves"),
        N_("C: Switch existing / new contents"),
        N_("A: Continue to confirmation"),
        N_("B: Return to slots"),
        N_("Nothing is written on this screen."),
    },
    /* HELP_COMPARE_PICK: Choose comparison slot */
    {
        N_("Up/Down: Select comparison slot"),
        N_("L/R: Page through slots"),
        N_("A: Compare with first slot"),
        N_("B: Cancel comparison"),
        N_("Comparing does not change saves."),
    },
};

const char *menu_help_title(MenuHelpTopic topic)
{
    if ((unsigned int)topic >= HELP_TOPIC_COUNT)
        return T("Controls");
    return T(titles[topic]);
}

const char *menu_help_line(MenuHelpTopic topic, unsigned int row)
{
    if ((unsigned int)topic >= HELP_TOPIC_COUNT || row >= MENU_HELP_ROWS)
        return "";
    /* Rows left out of a topic's list above are null pointers, shown as blank. */
    return lines[topic][row] ? T(lines[topic][row]) : "";
}

int menu_help_poll(MenuHelpHold *hold, unsigned long now, int down, int pressed, int released,
                   int other)
{
    /* Help stays open until the screen closes it and clears `active`. */
    if (hold->active)
        return MENU_HELP_SHOWING;

    /* P went down on its own: start timing, it may become a hold. */
    if (pressed && !other) {
        hold->pending = 1;
        hold->since = now;
    }

    /* Another button joined in: this is a combination such as C + P, which
     * the screen handles itself, so it is neither a tap nor a hold. */
    if (hold->pending && other) {
        hold->pending = 0;
        return MENU_HELP_NOTHING;
    }

    /* Held long enough (whether still down or just released): open help. */
    if (hold->pending && (down || released) && now - hold->since >= MENU_HELP_HOLD_MS) {
        hold->pending = 0;
        hold->active = 1;
        return MENU_HELP_OPEN;
    }

    /* Released before the hold time: an ordinary tap. */
    if (hold->pending && released) {
        hold->pending = 0;
        return MENU_HELP_TAP;
    }

    /* P is not involved this frame: forget any half-finished press. */
    if (!down && !released)
        hold->pending = 0;
    return MENU_HELP_NOTHING;
}
