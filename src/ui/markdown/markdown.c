#include <stdint.h>
#include <string.h>

#include <graphics.h>
#include <font.h>

#include <markdown.h>

// The renderer works in an 8px cell for body text and the 16px face for
// headings, matching the two fonts the shell already ships.
#define MD_CW 8
#define MD_CH 8
#define MD_HEAD_CH 16

// Vertical metrics per block kind, in pixels.
#define MD_H_BLANK 5
#define MD_H_PARA  10
#define MD_H_H1    22
#define MD_H_H2    20
#define MD_H_H3    16
#define MD_H_H4    14
#define MD_H_RULE  8

// Horizontal padding inside the text column.
#define MD_PAD_X   8

static md_line_t md_lines[384];
static int md_count;
static int md_roadmap;
static int md_truncated;

static md_vrow_t md_vrows[MD_MAX_VROWS];
static int md_vrow_count;
static int md_total_height;

// ---------------------------------------------------------------------------
// Small string helpers. The kernel has no libc, and only a handful of these are
// declared in the project's string.h, so the few needed here are local.
// ---------------------------------------------------------------------------

static int md_str_len(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static char md_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static int md_is_space(char c) { return c == ' ' || c == '\t'; }

static int md_is_digit(char c) { return c >= '0' && c <= '9'; }

// Copies src into dst, stopping at a space or the end, never splitting a word
// and never writing past cap-1. Returns the number of characters copied.
static int md_copy_word(char* dst, int cap, const char* src, int max_chars) {
    int n = 0;
    while (src[n] && !md_is_space(src[n]) && n < max_chars && n < cap - 1) {
        dst[n] = src[n];
        n++;
    }
    dst[n] = 0;
    return n;
}

static int md_starts_with(const char* s, const char* prefix) {
    int i = 0;
    while (prefix[i]) {
        if (s[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Inline parsing
//
// Emits marker-free text into `out->text` and records styled runs. Code spans
// are resolved first so that emphasis markers inside backticks stay literal.
// Underscore emphasis only opens at a word boundary, otherwise file names like
// NOTEPAD.TXT would lose their underscores.
// ---------------------------------------------------------------------------

static void md_span_push(md_line_t* out, int start, int len, int style) {
    if (len <= 0) return;
    if (out->span_count >= MD_MAX_SPANS) return;
    md_span_t* s = &out->spans[out->span_count++];
    s->start = (uint16_t)start;
    s->len = (uint16_t)len;
    s->style = (uint8_t)style;
}

static void md_inline(const char* src, md_line_t* out) {
    out->span_count = 0;
    out->text[0] = 0;
    if (!src) return;

    int i = 0;          // read cursor in src
    int n = 0;          // write cursor in out->text
    int prev_word = 0;  // previous emitted char was alphanumeric

    while (src[i] && n < MD_TEXT_MAX - 1) {
        char c = src[i];

        // Backslash escape: the next character is always literal.
        if (c == '\\' && src[i + 1]) {
            out->text[n++] = src[i + 1];
            prev_word = (src[i + 1] > 'a' && src[i + 1] < 'z') ||
                        (src[i + 1] > 'A' && src[i + 1] < 'Z') ||
                        md_is_digit(src[i + 1]);
            i += 2;
            continue;
        }

        // Inline code: everything up to the next backtick, verbatim.
        if (c == '`') {
            int j = i + 1;
            while (src[j] && src[j] != '`' && n < MD_TEXT_MAX - 1) {
                out->text[n++] = src[j++];
            }
            if (src[j] == '`') {
                md_span_push(out, n - (j - i - 1), j - i - 1, MD_S_CODE);
                i = j + 1;
            } else {
                i = j;  // Unterminated: treat the rest as plain text.
            }
            prev_word = 0;
            continue;
        }

        // Strong emphasis: ** or __
        if ((c == '*' && src[i + 1] == '*') || (c == '_' && src[i + 1] == '_')) {
            char mark = c;
            int j = i + 2;
            int start = n;
            while (src[j] && n < MD_TEXT_MAX - 1) {
                if (src[j] == mark && src[j + 1] == mark) break;
                out->text[n++] = src[j++];
            }
            if (src[j] == mark && src[j + 1] == mark) {
                md_span_push(out, start, n - start, MD_S_BOLD);
                i = j + 2;
            } else {
                i = j;
            }
            prev_word = 0;
            continue;
        }

        // Strikethrough: ~~
        if (c == '~' && src[i + 1] == '~') {
            int j = i + 2;
            int start = n;
            while (src[j] && n < MD_TEXT_MAX - 1) {
                if (src[j] == '~' && src[j + 1] == '~') break;
                out->text[n++] = src[j++];
            }
            if (src[j] == '~' && src[j + 1] == '~') {
                md_span_push(out, start, n - start, MD_S_STRIKE);
                i = j + 2;
            } else {
                i = j;
            }
            prev_word = 0;
            continue;
        }

        // Emphasis: * or _, the latter only at a word boundary.
        if (c == '*' || (c == '_' && !prev_word)) {
            char mark = c;
            int j = i + 1;
            int start = n;
            while (src[j] && n < MD_TEXT_MAX - 1) {
                if (src[j] == mark) break;
                // An underscore opener that never closes should stay literal.
                out->text[n++] = src[j++];
            }
            if (src[j] == mark && j > i + 1) {
                md_span_push(out, start, n - start, MD_S_ITALIC);
                i = j + 1;
            } else {
                i = j;
            }
            prev_word = 0;
            continue;
        }

        out->text[n++] = c;
        prev_word = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || md_is_digit(c);
        i++;
    }

    out->text[n] = 0;
}

// ---------------------------------------------------------------------------
// Line store
// ---------------------------------------------------------------------------

static md_line_t* md_line_new(md_kind_t kind) {
    if (md_count >= (int)(sizeof(md_lines) / sizeof(md_lines[0]))) {
        md_truncated = 1;
        return 0;
    }
    md_line_t* l = &md_lines[md_count++];
    l->text[0] = 0;
    l->span_count = 0;
    l->kind = (uint8_t)kind;
    l->depth = 0;
    l->ordinal = 0;
    return l;
}

// A horizontal rule is three or more of -, * or _ with nothing else on the line.
static int md_is_rule(const char* s, int len) {
    if (len < 3) return 0;
    char c = s[0];
    if (c != '-' && c != '*' && c != '_') return 0;
    for (int i = 0; i < len; i++) {
        if (s[i] != c) return 0;
    }
    return 1;
}

// Records where the roadmap section begins so the Settings view can split the
// document. Accepts the usual spellings rather than only "Planned".
static void md_note_roadmap(const md_line_t* l) {
    if (md_roadmap >= 0 || l->kind != MD_K_H2) return;
    if (md_starts_with(l->text, "Planned") ||
        md_starts_with(l->text, "Roadmap") ||
        md_starts_with(l->text, "Upcoming")) {
        md_roadmap = md_count - 1;
    }
}

// ---------------------------------------------------------------------------
// Block parsing
// ---------------------------------------------------------------------------

void md_parse(const char* text) {
    md_count = 0;
    md_roadmap = -1;
    md_truncated = 0;

    if (!text) return;

    int in_fence = 0;
    int prev_blank = 1;   // suppresses runs of blank lines
    int pos = 0;
    char line[MD_TEXT_MAX + 8];

    for (;;) {
        // Accumulate one source line.
        pos = 0;
        int done = 0;
        while (!done) {
            char ch = text[pos];
            if (ch == '\n' || ch == 0) {
                line[pos] = 0;
                done = 1;
                if (ch == 0) pos = -1;  // signal end of document
                else pos++;
                continue;
            }
            if (ch != '\r' && pos < (int)sizeof(line) - 1) line[pos++] = ch;
            pos++;
        }

        int len = md_str_len(line);
        while (len > 0 && md_is_space(line[len - 1])) line[--len] = 0;

        if (in_fence) {
            if (md_starts_with(line, "```")) {
                in_fence = 0;
            } else {
                md_line_t* l = md_line_new(MD_K_CODE);
                if (l) {
                    for (int i = 0; i < len && i < MD_TEXT_MAX - 1; i++) l->text[i] = line[i];
                    l->text[len < MD_TEXT_MAX ? len : MD_TEXT_MAX - 1] = 0;
                }
            }
            prev_blank = 0;
            if (pos < 0) break;
            continue;
        }

        if (len == 0) {
            if (!prev_blank) {
                md_line_new(MD_K_BLANK);
                prev_blank = 1;
            }
            if (pos < 0) break;
            continue;
        }
        prev_blank = 0;

        // Fenced code block.
        if (md_starts_with(line, "```")) {
            in_fence = 1;
            if (pos < 0) break;
            continue;
        }

        if (md_is_rule(line, len)) {
            md_line_new(MD_K_RULE);
            if (pos < 0) break;
            continue;
        }

        // Leading whitespace gives list depth and blockquote nesting.
        int i = 0;
        int depth = 0;
        while (line[i] == ' ') { i++; depth++; }
        if (line[i] == '\t') { i++; depth += 4; }
        depth /= 2;
        const char* body = line + i;

        // ATX heading: one to six '#' followed by a space.
        if (body[0] == '#') {
            int level = 0;
            while (body[level] == '#' && level < 6) level++;
            if (level >= 1 && level <= 6 && md_is_space(body[level])) {
                const char* htext = body + level;
                while (md_is_space(*htext)) htext++;
                md_kind_t kind = (md_kind_t)(MD_K_H1 + level - 1);
                md_line_t* l = md_line_new(kind);
                if (l) {
                    md_inline(htext, l);
                    md_note_roadmap(l);
                }
                if (pos < 0) break;
                continue;
            }
        }

        // Blockquote: one or more '>' markers.
        if (body[0] == '>') {
            const char* q = body;
            int level = 0;
            while (q[level] == '>') level++;
            q += level;
            while (md_is_space(*q)) q++;
            md_line_t* l = md_line_new(MD_K_QUOTE);
            if (l) {
                l->depth = (uint8_t)(level - 1);
                md_inline(q, l);
            }
            if (pos < 0) break;
            continue;
        }

        // Unordered list item: -, * or + followed by a space.
        if ((body[0] == '-' || body[0] == '*' || body[0] == '+') &&
            md_is_space(body[1])) {
            const char* item = body + 1;
            while (md_is_space(*item)) item++;
            md_line_t* l = md_line_new(MD_K_BULLET);
            if (l) {
                l->depth = (uint8_t)depth;
                md_inline(item, l);
            }
            if (pos < 0) break;
            continue;
        }

        // Ordered list item: digits then '.' or ')' then a space.
        if (md_is_digit(body[0])) {
            int d = 0;
            int v = 0;
            while (md_is_digit(body[d])) {
                if (v < 9999) v = v * 10 + (body[d] - '0');
                d++;
            }
            if ((body[d] == '.' || body[d] == ')') && md_is_space(body[d + 1])) {
                const char* item = body + d + 1;
                while (md_is_space(*item)) item++;
                md_line_t* l = md_line_new(MD_K_ORDERED);
                if (l) {
                    l->depth = (uint8_t)depth;
                    l->ordinal = (uint16_t)v;
                    md_inline(item, l);
                }
                if (pos < 0) break;
                continue;
            }
        }

        // Anything else is paragraph text.
        {
            md_line_t* l = md_line_new(MD_K_PARA);
            if (l) md_inline(body, l);
        }

        if (pos < 0) break;
    }
}

int md_line_count(void) { return md_count; }
int md_roadmap_start(void) { return md_roadmap; }
int md_was_truncated(void) { return md_truncated; }

// ---------------------------------------------------------------------------
// Layout
//
// Lines are wrapped here rather than at parse time because the usable width is
// only known once the window has been laid out. The result is cached per width so
// the wheel handler and the renderer measure the same document.
// ---------------------------------------------------------------------------

static int md_kind_height(int kind) {
    switch (kind) {
        case MD_K_BLANK: return MD_H_BLANK;
        case MD_K_H1:    return MD_H_H1;
        case MD_K_H2:    return MD_H_H2;
        case MD_K_H3:    return MD_H_H3;
        case MD_K_H4:
        case MD_K_H5:
        case MD_K_H6:    return MD_H_H4;
        case MD_K_RULE:  return MD_H_RULE;
        default:         return MD_H_PARA;
    }
}

// Left inset for a block, in pixels.
static int md_kind_indent(int kind, int depth) {
    if (kind == MD_K_BULLET || kind == MD_K_ORDERED) {
        return 6 + (depth * 14) + 12;   // marker gutter plus nesting
    }
    if (kind == MD_K_QUOTE) return 8 + (depth * 6);
    return 0;
}

static void md_view_range(int view, int* start, int* end) {
    if (md_roadmap >= 0) {
        if (view == MD_VIEW_ROADMAP) {
            *start = md_roadmap;
            *end = md_count;
        } else {
            *start = 0;
            *end = md_roadmap;
        }
    } else {
        *start = 0;
        *end = (view == MD_VIEW_ROADMAP) ? 0 : md_count;
    }
    if (*end < *start) *end = *start;
}

// Width available for text on a line, in characters. Headings use the upscaled
// face, so they fit proportionally fewer characters per pixel.
static int md_line_chars(int w, int kind, int depth) {
    int used = MD_PAD_X + md_kind_indent(kind, depth) + MD_PAD_X;
    int cell = ((kind == MD_K_H1) || (kind == MD_K_H2)) ? MD_HEAD_CH : MD_CW;
    int chars = (w - used) / cell;
    if (chars < 8) chars = 8;
    return chars;
}

// Draws text in the upscaled face. font_sample_pixel nearest-neighbour scales the
// 8x8 glyph, so this is a blocky 2x of the same face rather than a distinct one.
static void md_draw_big(int x, int y, const char* s, int size, uint8_t colour) {
    uint8_t cw = 8, ch = 8;
    font_get_size((font_size_t)size, &cw, &ch);
    for (; s && *s; s++) {
        for (int r = 0; r < ch; r++) {
            for (int c = 0; c < cw; c++) {
                if (font_sample_pixel(*s, (font_size_t)size, (uint8_t)c, (uint8_t)r)) {
                    draw_pixel(x + c, y + r, colour);
                }
            }
        }
        x += cw;
    }
}

// Wraps the document for a view at width w, filling md_vrows and returning the
// total height in pixels. Returns the number of visual rows.
static int md_layout(int w, int view) {
    int start, end;
    md_view_range(view, &start, &end);

    md_vrow_count = 0;
    md_total_height = 0;

    for (int li = start; li < end; li++) {
        const md_line_t* l = &md_lines[li];
        int height = md_kind_height(l->kind);

        // A rule spans the full column regardless of its (empty) text.
        if (l->kind == MD_K_RULE || l->kind == MD_K_BLANK) {
            if (md_vrow_count < MD_MAX_VROWS) {
                md_vrow_t* r = &md_vrows[md_vrow_count++];
                r->line = (int16_t)li;
                r->start = 0;
                r->len = 0;
                r->height = (uint8_t)height;
                md_total_height += height;
            }
            continue;
        }

        int chars = md_line_chars(w, l->kind, l->depth);
        int len = md_str_len(l->text);

        if (len <= chars) {
            if (md_vrow_count < MD_MAX_VROWS) {
                md_vrow_t* r = &md_vrows[md_vrow_count++];
                r->line = (int16_t)li;
                r->start = 0;
                r->len = (int16_t)len;
                r->height = (uint8_t)height;
                md_total_height += height;
            }
            continue;
        }

        // Word wrap. Words longer than the column are hard-split so a long token
        // cannot push the rest of the line off screen.
        int pos = 0;
        int first = 1;
        while (pos < len) {
            if (md_vrow_count >= MD_MAX_VROWS) {
                md_truncated = 1;
                break;
            }
            int take = 0;
            int end_pos = pos;

            // Skip leading spaces on continuation rows.
            if (!first) {
                while (pos < len && l->text[pos] == ' ') pos++;
                if (pos >= len) break;
            }
            first = 0;

            int scan = pos;
            int last_space = -1;
            while (scan < len && (scan - pos) < chars) {
                if (l->text[scan] == ' ') last_space = scan;
                scan++;
            }
            if (scan >= len) {
                end_pos = len;
            } else if (last_space > pos) {
                end_pos = last_space;
            } else {
                end_pos = scan;   // single long word
            }

            md_vrow_t* r = &md_vrows[md_vrow_count++];
            r->line = (int16_t)li;
            r->start = (int16_t)pos;
            r->len = (int16_t)(end_pos - pos);
            r->height = (uint8_t)height;
            md_total_height += height;
            take = end_pos - pos;
            pos = end_pos;
            if (take <= 0) pos++;
        }
    }

    return md_vrow_count;
}

int md_content_height(int w, int view) {
    md_layout(w, view);
    return md_total_height;
}

int md_scroll_by(int w, int view, int scroll, int wheel_notches) {
    md_layout(w, view);
    scroll -= wheel_notches * 24;
    if (scroll < 0) scroll = 0;
    return scroll;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

// Draws a run of characters with one inline style applied.
static void md_draw_run(int x, int y, const char* text, int len, int style) {
    if (len <= 0) return;

    if (style == MD_S_CODE) {
        // A chip behind the run, so inline code reads as code without relying on
        // a colour shift alone.
        fill_rect(x - 1, y - 1, (len * MD_CW) + 2, MD_CH + 2, XP_LGRAY);
    }

    for (int i = 0; i < len; i++) {
        // The 8px face has no bold weight, so bold is faked by over-printing one
        // pixel to the right. At this cell size it reads correctly.
        draw_char(x, y, text[i], style == MD_S_CODE ? XP_BLACK : XP_DGRAY);
        if (style == MD_S_BOLD) {
            draw_char(x + 1, y, text[i], XP_DGRAY);
        }
    }

    if (style == MD_S_STRIKE) {
        fill_rect(x, y + 3, len * MD_CW, 1, XP_DGRAY);
    }
}

// Draws the marker that sits in the gutter left of a list item or quote.
static void md_draw_marker(int x, int y, const md_line_t* l, int kind) {
    if (kind == MD_K_BULLET) {
        // A 2x2 dot rather than a glyph, because the 8px font's bullet cell is
        // not guaranteed to hold one.
        int cx = x + 4;
        int cy = y + 3;
        fill_rect(cx, cy, 3, 3, XP_DGRAY);
    } else if (kind == MD_K_ORDERED) {
        char num[8];
        int n = 0;
        unsigned v = l->ordinal;
        if (v == 0) {
            num[n++] = '0';
        } else {
            char d[6];
            int m = 0;
            while (v > 0 && m < 6) { d[m++] = (char)('0' + (v % 10)); v /= 10; }
            while (m > 0) num[n++] = d[--m];
        }
        num[n++] = '.';
        num[n] = 0;
        for (int i = 0; i < n; i++) {
            draw_char(x + i * MD_CW, y, num[i], XP_DGRAY);
        }
    }
}

void md_draw(int x, int y, int w, int h, int view, int* scroll) {
    int rows = md_layout(w, view);

    int max_scroll = md_total_height - h;
    if (max_scroll < 0) max_scroll = 0;
    int top = scroll ? *scroll : 0;
    if (top < 0) top = 0;
    if (top > max_scroll) top = max_scroll;
    if (scroll) *scroll = top;

    int cy = y - top;
    for (int i = 0; i < rows; i++) {
        const md_vrow_t* r = &md_vrows[i];
        int row_h = r->height;

        if (cy + row_h > y && cy < y + h) {
            const md_line_t* l = &md_lines[r->line];
            int indent = md_kind_indent(l->kind, l->depth);
            int tx = x + MD_PAD_X + indent;

            switch (l->kind) {
                case MD_K_H1:
                case MD_K_H2: {
                    int face = (l->kind == MD_K_H1) ? FONT_SIZE_16X16 : FONT_SIZE_12X12;
                    int tw = (int)font_measure_text(l->text, (font_size_t)face);
                    md_draw_big(tx, cy, l->text, (int)face,
                                l->kind == MD_K_H1 ? XP_LBLUE : XP_BLUE);
                    int rule_y = cy + (l->kind == MD_K_H1 ? 19 : 14);
                    fill_rect(tx, rule_y, tw, l->kind == MD_K_H1 ? 2 : 1,
                              l->kind == MD_K_H1 ? XP_LBLUE : XP_DGRAY);
                    break;
                }
                case MD_K_H3: {
                    int sx = tx, n = md_str_len(l->text), i2;
                    for (i2 = 0; i2 < n; i2++) {
                        draw_char(sx, cy, l->text[i2], XP_BLUE);
                        draw_char(sx + 1, cy, l->text[i2], XP_BLUE);
                        sx += MD_CW + 1;
                    }
                    break;
                }
                case MD_K_H4:
                case MD_K_H5:
                case MD_K_H6: {
                    int sx = tx, n = md_str_len(l->text), i2;
                    for (i2 = 0; i2 < n; i2++) {
                        draw_char(sx, cy, l->text[i2], XP_DGRAY);
                        draw_char(sx + 1, cy, l->text[i2], XP_DGRAY);
                        sx += MD_CW + 1;
                    }
                    break;
                }
                case MD_K_BULLET:
                case MD_K_ORDERED: {
                    md_draw_marker(x + MD_PAD_X + (l->depth * 14), cy, l, l->kind);
                    // Fall through to the span-aware text pass below.
                    goto text_pass;
                }
                case MD_K_QUOTE: {
                    fill_rect(x + MD_PAD_X + (l->depth * 6), cy - 1, 2, row_h - 1, XP_BLUE);
                    goto text_pass;
                }
                case MD_K_CODE: {
                    fill_rect(x + MD_PAD_X, cy - 1, w - (MD_PAD_X * 2), row_h - 1, XP_LGRAY);
                    goto text_pass;
                }
                case MD_K_RULE: {
                    fill_rect(x + MD_PAD_X, cy + (row_h / 2) - 1,
                              w - (MD_PAD_X * 2), 1, XP_DGRAY);
                    break;
                }
                default:
                text_pass: {
                    // Walk the inline spans, drawing the slice of each that falls
                    // on this visual row.
                    int seg_start = r->start;
                    int seg_end = r->start + r->len;
                    int cx2 = tx;
                    int cur = seg_start;
                    for (int s = 0; s <= l->span_count; s++) {
                        int s_start = (s < l->span_count) ? l->spans[s].start : seg_end;
                        int s_end = (s < l->span_count)
                                        ? l->spans[s].start + l->spans[s].len
                                        : seg_end;
                        int style = (s < l->span_count) ? l->spans[s].style : MD_S_NONE;
                        int a = (cur > s_start) ? cur : s_start;
                        int b = (s_end < seg_end) ? s_end : seg_end;
                        if (b > a) {
                            md_draw_run(cx2, cy, l->text + a, b - a, style);
                            cx2 += (b - a) * MD_CW;
                        }
                        if (s < l->span_count && s_end > cur) cur = s_end;
                    }
                    break;
                }
            }
        }
        cy += row_h;
        if (cy > y + h) break;
    }
}
