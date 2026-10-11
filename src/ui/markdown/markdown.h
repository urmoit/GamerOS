#ifndef GAMEROS_MARKDOWN_H
#define GAMEROS_MARKDOWN_H

#include <stdint.h>

// A small block + inline Markdown renderer for the Settings changelog.
//
// The previous implementation was line-oriented and kept one colour per line, so
// it could not express the things a changelog actually uses: real bullets,
// indented nesting, headings that look like headings, blockquotes with a quote
// bar, inline code as a chip, or emphasis. This module parses blocks and inline
// spans separately, then wraps and draws the result at display time so the same
// parsed data reflows correctly when the window is resized.

// Capacity of a logical line, in characters. Source lines longer than this are
// broken at a word boundary onto a continuation line rather than cut mid-word.
#define MD_TEXT_MAX 200

// Inline spans recorded per line. A changelog line rarely needs more; the parser
// stops adding spans (and keeps the text) rather than overflowing.
#define MD_MAX_SPANS 12

// Upper bound on wrapped visual rows for one view, used to size the row table.
#define MD_MAX_VROWS 1024

// Capacity of the parsed document, in logical lines. This is the number
// tools/embed_changelog.py checks the changelog against, so raising it here is
// what stops the Settings viewer dropping the tail of a long changelog.
#define MD_MAX_LINES 384

typedef enum {
    MD_K_BLANK = 0,
    MD_K_PARA,
    MD_K_H1, MD_K_H2, MD_K_H3, MD_K_H4, MD_K_H5, MD_K_H6,
    MD_K_BULLET,
    MD_K_ORDERED,
    MD_K_QUOTE,
    MD_K_CODE,
    MD_K_RULE
} md_kind_t;

typedef enum {
    MD_S_NONE = 0,
    MD_S_BOLD,
    MD_S_ITALIC,
    MD_S_CODE,
    MD_S_STRIKE
} md_style_t;

typedef struct {
    uint16_t start;   // Index into md_line_t::text.
    uint16_t len;
    uint8_t  style;
} md_span_t;

typedef struct {
    char      text[MD_TEXT_MAX];
    md_span_t spans[MD_MAX_SPANS];
    uint8_t   span_count;
    uint8_t   kind;
    uint8_t   depth;      // List nesting level; 0 for non-list blocks.
    uint16_t  ordinal;    // Number for an ordered-list item.
} md_line_t;

// One wrapped row of a logical line, produced at draw time because the usable
// width is only known once the window is laid out.
typedef struct {
    int16_t line;     // Index into the parsed line table.
    int16_t start;    // First character of the logical line on this row.
    int16_t len;      // Characters on this row.
    uint8_t height;   // Pixels this row occupies.
} md_vrow_t;

typedef enum {
    MD_VIEW_CHANGELOG = 0,
    MD_VIEW_ROADMAP = 1
} md_view_t;

// Parsing. md_parse replaces any previously loaded document.
void md_parse(const char* text);

int  md_line_count(void);
// Index of the first line of the roadmap section, or -1 when the document has no
// roadmap heading. The Settings view uses it to split changelog from roadmap.
int  md_roadmap_start(void);
// True when at least one line was dropped because MD_TEXT_MAX or MD_MAX_VROWS was
// reached, so the caller can warn instead of silently truncating.
int  md_was_truncated(void);

// Drawing. `scroll` is in pixels and is clamped in place to the document height.
// The document is measured and wrapped as part of the call.
void md_draw(int x, int y, int w, int h, int view, int* scroll);

// Total height in pixels of the wrapped document for a view at the given width.
int  md_content_height(int w, int view);

// Applies a wheel delta (in notches) to a scroll position and returns the new
// clamped value. Kept separate so the Settings hit test and the renderer agree.
int  md_scroll_by(int w, int view, int scroll, int wheel_notches);

#endif
