#!/usr/bin/env python3
"""Embed a markdown changelog into main.c as a C string literal.

The in-OS "GamerOS Update" viewer reads C:/GamerOS/CHANGELOG.md, which is
written at boot from settings_app_changelog_text(). That string used to be
hand-maintained and drifted out of sync with docs/changelogs/.

This regenerates it from the markdown file so there is a single source of truth:

    python tools/embed_changelog.py

Replaces the body of settings_app_changelog_text() in the Settings app.
"""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# The changelog now lives with the Settings app that renders it.
MAIN_C = os.path.join(ROOT, "src", "ui", "apps", "settings", "settings_ui.c")
FS_H = os.path.join(ROOT, "src", "include", "fs.h")
MARKDOWN_H = os.path.join(ROOT, "src", "ui", "markdown", "markdown.h")

# The changelog the in-OS viewer should show. Build 1.570 is the current
# in-progress log; the older release notes stay in the repo for history.
DEFAULT_MD = os.path.join(ROOT, "docs", "changelogs", "changelog_2026-10-00.md")


def read_max_file_size():
    """Read MAX_FILE_SIZE from fs.h so this script cannot drift from the kernel."""
    with io.open(FS_H, "r", encoding="utf-8") as fh:
        for line in fh:
            m = re.match(r'^\s*#define\s+MAX_FILE_SIZE\s+(\d+)', line)
            if m:
                return int(m.group(1))
    raise RuntimeError("MAX_FILE_SIZE not found in %s" % FS_H)


def read_md_max_lines():
    """Read MD_MAX_LINES from the markdown module so this script cannot drift
    from the viewer capacity it is warning about."""
    with io.open(MARKDOWN_H, "r", encoding="utf-8") as fh:
        for line in fh:
            m = re.match(r'^\s*#define\s+MD_MAX_LINES\s+(\d+)', line)
            if m:
                return int(m.group(1))
    raise RuntimeError("MD_MAX_LINES not found in %s" % MARKDOWN_H)

START_RE = re.compile(
    r'^const char\* settings_app_changelog_text\(void\) \{$'
)
END_RE = re.compile(r'^\}$')


def escape(text):
    """Escape a line for embedding inside a C string literal."""
    out = text.replace('\\', '\\\\').replace('"', '\\"')
    return out


def to_c_literal(lines):
    """Render markdown lines as a sequence of adjacent C string literals."""
    body = []
    for line in lines:
        body.append('           "%s\\n"' % escape(line))
    return "\n".join(body)


def main():
    md_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_MD

    if not os.path.exists(md_path):
        print("error: changelog not found: %s" % md_path)
        return 1

    with io.open(md_path, "r", encoding="utf-8") as fh:
        raw = fh.read()

    raw = raw.replace("\r\n", "\n").replace("\r", "\n")
    lines = raw.split("\n")
    # A trailing newline yields a final empty element; drop it.
    if lines and lines[-1] == "":
        lines.pop()

    # HTML comments are authoring markers ("new entries go below this line"),
    # not content. The markdown viewer has no support for them, so they would
    # render as a paragraph of visible text.
    lines = [l for l in lines if not l.strip().startswith("<!--")]

    max_file_size = read_max_file_size()
    size_bytes = len(raw.encode("utf-8"))

    if size_bytes > max_file_size:
        print("error: changelog is %d bytes, MAX_FILE_SIZE is %d"
              % (size_bytes, max_file_size))
        return 1

    max_md_lines = read_md_max_lines()
    if len(lines) > max_md_lines:
        print("warning: %d lines exceeds SETTINGS_MD_MAX_LINES (%d); "
              "the viewer will drop the tail" % (len(lines), max_md_lines))

    with io.open(MAIN_C, "r", encoding="utf-8") as fh:
        src = fh.read().split("\n")

    # Locate the function body: from the signature to the first lone '}'.
    start = None
    for i, line in enumerate(src):
        if START_RE.match(line.strip()):
            start = i
            break
    if start is None:
        print("error: settings_app_changelog_text() not found in " + MAIN_C)
        return 1

    end = None
    for j in range(start + 1, len(src)):
        if END_RE.match(src[j].strip()):
            end = j
            break
    if end is None:
        print("error: could not find end of settings_app_changelog_text()")
        return 1

    literal = to_c_literal(lines)
    new_src = src[:start + 1] + ['    return ' + literal + ';'] + src[end:]
    # Drop a trailing ';' that may now be doubled by the splice.
    out = []
    for k, line in enumerate(new_src):
        if line.strip() == ";" and k > 0 and new_src[k - 1].rstrip().endswith(";"):
            continue
        out.append(line)

    with io.open(MAIN_C, "w", encoding="utf-8") as fh:
        fh.write("\n".join(out))

    print("embedded %d lines (%d bytes) from %s"
          % (len(lines), len(raw.encode("utf-8")), os.path.basename(md_path)))
    print("  -> %s" % os.path.relpath(MAIN_C, ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())