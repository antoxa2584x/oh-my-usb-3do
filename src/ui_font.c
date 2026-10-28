/*
 * Measuring and drawing text with the menu's pixel font. See ui_font.h.
 *
 * Glyph data (from ui_font_data.h), for each of the four base styles:
 *   ui_advances[style][glyph]  how far the pen moves after the glyph, in pixels
 *   ui_glyphs[style]           the style's glyph pictures, one after another:
 *                              ui_font_height(style) rows of 16 pixels each,
 *                              4 bits per pixel, so 8 bytes per row; the
 *                              left pixel of each pair is in the high 4 bits
 * Glyph 0 is the space character (ASCII 32), glyph 94 is '~' (ASCII 126), and
 * the rest are the code points in ui_codes, in order.
 *
 * Text is UTF-8. A character's first byte carries its whole width and
 * picture, and the bytes after it in a multi-byte character have none, so
 * functions that count bytes (fits, tails, scroll offsets) still work: a
 * character is never measured twice, and starting or stopping inside one
 * just leaves it out.
 */
#include "ui_font.h"
#include "ui_font_data.h"
#include <string.h>

/* A glyph picture is 16 pixels wide and 8 bytes per row. */
#define GLYPH_WIDTH      16
#define GLYPH_ROW_BYTES  8
#define BASE_STYLE_COUNT 4

/* Glyph number for no glyph at all: the later bytes of a character. */
#define NO_GLYPH      (-1)
#define UNKNOWN_GLYPH ('?' - 32)

/* The glyph for the character starting at `text`: printable ASCII, a
 * character in ui_codes, NO_GLYPH for a byte inside a character, and '?'
 * for anything else (including a character cut short). */
static int glyph_index(const char *text)
{
    const unsigned char *bytes = (const unsigned char *)text;
    unsigned long code;
    int low, high;
    if (bytes[0] < 0x80)
        return bytes[0] >= 32 && bytes[0] <= 126 ? bytes[0] - 32 : UNKNOWN_GLYPH;
    if ((bytes[0] & 0xc0U) == 0x80U)
        return NO_GLYPH;
    /* Only two-byte characters (up to U+07FF) and three-byte ones are drawn. */
    if ((bytes[0] & 0xe0U) == 0xc0U) {
        if ((bytes[1] & 0xc0U) != 0x80U)
            return UNKNOWN_GLYPH;
        code = ((bytes[0] & 0x1fUL) << 6) | (bytes[1] & 0x3fUL);
    } else if ((bytes[0] & 0xf0U) == 0xe0U) {
        if ((bytes[1] & 0xc0U) != 0x80U || (bytes[2] & 0xc0U) != 0x80U)
            return UNKNOWN_GLYPH;
        code = ((bytes[0] & 0x0fUL) << 12) | ((bytes[1] & 0x3fUL) << 6) | (bytes[2] & 0x3fUL);
    } else
        return UNKNOWN_GLYPH;
    low = 0;
    high = (int)(sizeof(ui_codes) / sizeof(ui_codes[0]));
    while (low < high) {
        int mid = (low + high) / 2;
        if (ui_codes[mid] < code)
            low = mid + 1;
        else
            high = mid;
    }
    if (low < (int)(sizeof(ui_codes) / sizeof(ui_codes[0])) && ui_codes[low] == code)
        return 95 + low;
    return UNKNOWN_GLYPH;
}

/* How many bytes the character starting at `text` takes (1 for a stray
 * byte), so a fit never ends inside a character. */
static size_t character_bytes(const char *text)
{
    unsigned char first = (unsigned char)text[0];
    size_t count = 1;
    if (first >= 0xc0)
        while (count < 4 && ((unsigned char)text[count] & 0xc0U) == 0x80U)
            ++count;
    return count;
}

/* Which glyph table a style uses: small bold reuses the small glyphs. */
static int base_style(int style)
{
    if (style == UI_FONT_SMALL_BOLD)
        return UI_FONT_SMALL;
    if (style >= 0 && style < BASE_STYLE_COUNT)
        return style;
    return UI_FONT_NORMAL;
}

/* How far the pen moves after glyph `glyph` in `style`. Bold strokes are a
 * pixel wider, so every visible bold glyph (not the space) takes one more. */
static int glyph_advance(int style, int glyph)
{
    int advance;
    if (glyph == NO_GLYPH)
        return 0;
    advance = ui_advances[base_style(style)][glyph];
    if (style == UI_FONT_SMALL_BOLD && glyph != 0)
        advance += 1;
    return advance;
}

int ui_font_height(int style)
{
    static const int heights[BASE_STYLE_COUNT] = {15, 12, 17, 20};
    return heights[base_style(style)];
}

int ui_font_width(const char *text, int style)
{
    int width = 0;
    for (; *text; ++text)
        width += glyph_advance(style, glyph_index(text));
    return width;
}

size_t ui_font_fit(const char *text, size_t length, int pixels, int style)
{
    size_t count = 0;
    int width = 0;

    while (count < length && text[count]) {
        int advance = glyph_advance(style, glyph_index(text + count));
        size_t bytes = character_bytes(text + count);
        if (width + advance > pixels || count + bytes > length)
            break;
        width += advance;
        count += bytes;
    }
    return count;
}

size_t ui_font_tail_start(const char *text, size_t length, int pixels, int style)
{
    size_t start = length;
    int width = 0;

    /* Add characters from the end backwards while they still fit. */
    while (start > 0) {
        size_t first = start - 1;
        int advance;
        /* Step back over a whole character: its later bytes, then its first. */
        while (first > 0 && ((unsigned char)text[first] & 0xc0U) == 0x80U)
            --first;
        advance = glyph_advance(style, glyph_index(text + first));
        if (width + advance > pixels)
            break;
        width += advance;
        start = first;
    }
    return start;
}

void ui_font_measure(UiFontMeasure *measure, const char *text, size_t length, int pixels, int style)
{
    /* Same label as last time: keep everything already worked out. */
    if (measure->valid && measure->length == length && measure->pixels == pixels &&
        measure->style == style && !memcmp(measure->value, text, length))
        return;

    measure->length = length;
    measure->pixels = pixels;
    measure->style = style;
    measure->fit_valid = 0;
    measure->tail_valid = 0;
    measure->ellipsis_valid = 0;
    measure->offset_valid = 0;

    /* Remember the text if it fits, so the next call can recognize it. */
    measure->valid = length < UI_FONT_MEASURE_BYTES;
    if (measure->valid) {
        memcpy(measure->value, text, length);
        measure->value[length] = 0;
    }
}

size_t ui_font_measure_tail(UiFontMeasure *measure, const char *text)
{
    if (!measure->tail_valid) {
        measure->tail = ui_font_tail_start(text, measure->length, measure->pixels, measure->style);
        measure->tail_valid = 1;
    }
    return measure->tail;
}

size_t ui_font_measure_ellipsis(UiFontMeasure *measure, const char *text)
{
    if (!measure->ellipsis_valid) {
        /*
         * Measured separately from the plain fit: a line can fit in the
         * pixels yet still be cut short by the caller's 64-character limit,
         * and then it needs the "..." too.
         */
        int room = measure->pixels - ui_font_width("...", measure->style);
        measure->ellipsis = ui_font_fit(text, measure->length, room, measure->style);
        measure->ellipsis_valid = 1;
    }
    return measure->ellipsis;
}

size_t ui_font_measure_fit(UiFontMeasure *measure, const char *text, size_t offset)
{
    if (offset >= measure->length)
        return 0;

    /* Not scrolled: the fit from the start, remembered separately because
     * the label returns to it at the start of every scroll cycle. */
    if (offset == 0) {
        if (!measure->fit_valid) {
            measure->fit = ui_font_fit(text, measure->length, measure->pixels, measure->style);
            measure->fit_valid = 1;
        }
        return measure->fit;
    }

    /* Scrolled: the fit at the current position, which stays the same for
     * several frames between steps. */
    if (measure->offset_valid && measure->offset == offset)
        return measure->offset_fit;
    measure->offset = offset;
    measure->offset_valid = 1;
    measure->offset_fit =
        ui_font_fit(text + offset, measure->length - offset, measure->pixels, measure->style);
    return measure->offset_fit;
}

/* The ink level (0 to 15) of pixel `x` in a row of packed 4-bit pixels. */
static unsigned int read_pixel(const unsigned char *row, int x)
{
    return (x & 1) ? row[x / 2] & 15U : row[x / 2] >> 4;
}

/*
 * Makes the small font bold: widens every stroke one pixel to the right,
 * except where that would fill a one-pixel gap before the next stroke, so
 * letters like "m" and "B" stay open instead of becoming solid blocks.
 */
static void embolden(unsigned char *out, int width, int height)
{
    /* One row's ink levels, unpacked, with two extra zero pixels at the end
     * so looking two pixels ahead never reads past the row. */
    unsigned char ink[UI_FONT_WIDTH + 2];
    int row, x;

    for (row = 0; row < height; ++row) {
        unsigned char *line = out + row * UI_FONT_STRIDE;

        memset(ink, 0, sizeof(ink));
        for (x = 0; x < width; ++x)
            ink[x] = (unsigned char)read_pixel(line, x);

        for (x = 0; x + 1 < width; ++x) {
            /* Inked pixel, then two blank ones: widening cannot close a gap. */
            if (ink[x] && !ink[x + 1] && !ink[x + 2]) {
                int right = x + 1;
                if (right & 1)
                    line[right / 2] |= ink[x]; /* low 4 bits: odd pixel */
                else
                    line[right / 2] |= (unsigned char)(ink[x] << 4); /* high 4 bits: even pixel */
            }
        }
    }
}

int ui_font_render(unsigned char *out, const char *text, int style, int max_width)
{
    int pen_x = 0; /* where the next glyph starts */
    int table = base_style(style);
    int height = ui_font_height(style);

    if (max_width > UI_FONT_WIDTH)
        max_width = UI_FONT_WIDTH;
    memset(out, 0, UI_FONT_STRIDE * UI_FONT_HEIGHT);

    while (*text) {
        int glyph = glyph_index(text++);
        int advance = glyph_advance(style, glyph);
        const unsigned char *picture;
        int row, column;

        if (glyph == NO_GLYPH)
            continue;
        if (pen_x + advance > max_width)
            break; /* the next character would not fit: stop */
        picture = ui_glyphs[table] + (size_t)glyph * (size_t)height * GLYPH_ROW_BYTES;

        /* Copy the glyph's pixels, keeping the stronger ink where glyphs
         * overlap (a glyph's picture can extend past its advance). */
        for (row = 0; row < height; ++row) {
            for (column = 0; column < GLYPH_WIDTH && pen_x + column < max_width; ++column) {
                unsigned int packed = picture[row * GLYPH_ROW_BYTES + column / 2];
                unsigned int coverage = (column & 1) ? packed & 15U : packed >> 4;
                int x = pen_x + column;

                if (coverage) {
                    unsigned char *pixel_pair = out + row * UI_FONT_STRIDE + x / 2;
                    unsigned int existing = read_pixel(out + row * UI_FONT_STRIDE, x);
                    if (coverage > existing) {
                        if (x & 1)
                            *pixel_pair = (unsigned char)((*pixel_pair & 0xf0U) | coverage);
                        else
                            *pixel_pair = (unsigned char)((*pixel_pair & 15U) | (coverage << 4));
                    }
                }
            }
        }
        pen_x += advance;
    }

    if (style == UI_FONT_SMALL_BOLD)
        embolden(out, pen_x, height);
    return pen_x;
}

/*
 * Mixes one 5-bit color channel: `level` fifteenths of the ink, the rest
 * background, rounded to the nearest value.
 */
static unsigned int blend_channel(unsigned int ink, unsigned int background, int level)
{
    return (ink * level + background * (15 - level) + 7) / 15;
}

void ui_font_palette(unsigned short *out, unsigned int ink, unsigned int background)
{
    int level;

    memset(out, 0, 32 * sizeof(*out));
    /* Entry 0 stays 0: on the 3DO, color 0 in a CEL is transparent. */
    for (level = 1; level < 16; ++level) {
        unsigned int red = blend_channel((ink >> 10) & 31U, (background >> 10) & 31U, level);
        unsigned int green = blend_channel((ink >> 5) & 31U, (background >> 5) & 31U, level);
        unsigned int blue = blend_channel(ink & 31U, background & 31U, level);
        out[level] = (unsigned short)((red << 10) | (green << 5) | blue);
    }
}
