#ifndef GAMEROS_MARKDOWN_H
#define GAMEROS_MARKDOWN_H

#include <stdint.h>

// Markdown renderer for the Settings changelog.

// Capacity of a logical line, in characters; wraps at word boundaries.
#define MD_TEXT_MAX 200

// Inline spans recorded per line.
#define MD_MAX_SPANS 12

// Upper bound on wrapped visual rows for one view.
#define MD_MAX_VROWS 1024

// Capacity of the parsed document, in logical lines.
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

// One wrapped row of a logical line, built at draw time.
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
// First line of the roadmap section, or -1 when there is no roadmap heading.
int  md_roadmap_start(void);
// True when a line was dropped because MD_TEXT_MAX or MD_MAX_VROWS was reached.
int  md_was_truncated(void);

// Drawing. `scroll` is in pixels and is clamped in place.
void md_draw(int x, int y, int w, int h, int view, int* scroll);

// Total height in pixels of the wrapped document for a view.
int  md_content_height(int w, int view);

// Applies a wheel delta (notches) to a scroll position, returning the new
// clamped value.
int  md_scroll_by(int w, int view, int scroll, int wheel_notches);

#endif
