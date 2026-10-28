#!/usr/bin/env python3
"""Generate src/menu_text_data.h, the menu's translated text.

Usage: build-menu-text.py [--check | --missing]

Collects every piece of text marked with T("...") or N_("...") in src/
(see src/menu_text.h) and its translation from data/menu-text-uk.json, an
object of English text and its Ukrainian translation. Stops with an error
when a marked piece has no translation, a translation is no longer used, a
translation's printf conversions (%d, %s, ...) differ from the English
ones, or it has a character the menu's font does not have
(tools/build-ui-font.py).

  --check    only check that menu_text_data.h is current
  --missing  print the marked text that has no translation yet, as JSON
"""
import importlib.util
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
TRANSLATIONS = ROOT / "data/menu-text-uk.json"
OUTPUT = ROOT / "src/menu_text_data.h"

# A marker, then one or more adjacent string literals.
MARKED = re.compile(r'\b(?:T|N_)\(\s*((?:"(?:[^"\\\n]|\\.)*"\s*)+)\)')
LITERAL = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
CONVERSION = re.compile(r"%[-+ #0]*[0-9*]*(?:\.[0-9*]+)?[hlL]*[diouxXeEfgGcsp%]")
ESCAPES = {"n": "\n", "t": "\t", "\\": "\\", '"': '"', "'": "'", "r": "\r", "0": "\0"}


def unescape(body):
    """The text of a C string literal's body (UTF-8, simple escapes)."""
    out = bytearray()
    i = 0
    raw = body.encode("utf-8")
    while i < len(raw):
        if raw[i] == ord("\\"):
            nxt = chr(raw[i + 1])
            if nxt in "01234567":
                digits = re.match(rb"[0-7]{1,3}", raw[i + 1 : i + 4]).group()
                out.append(int(digits, 8))
                i += 1 + len(digits)
                continue
            if nxt not in ESCAPES:
                sys.exit(f"Unsupported escape \\{nxt} in marked text: {body}")
            out += ESCAPES[nxt].encode()
            i += 2
            continue
        out.append(raw[i])
        i += 1
    return out.decode("utf-8")


def strip_comments(source):
    """The source with comments blanked out, keeping string literals."""
    out = []
    i = 0
    while i < len(source):
        if source.startswith("/*", i):
            end = source.index("*/", i + 2) + 2
            out.append(" " * (end - i))
            i = end
        elif source.startswith("//", i):
            end = source.find("\n", i)
            end = len(source) if end < 0 else end
            out.append(" " * (end - i))
            i = end
        elif source[i] == '"':
            match = re.compile(r'"(?:[^"\\\n]|\\.)*"').match(source, i)
            out.append(match.group())
            i = match.end()
        elif source[i] == "'":
            match = re.compile(r"'(?:[^'\\\n]|\\.)*'").match(source, i)
            out.append(match.group())
            i = match.end()
        else:
            out.append(source[i])
            i += 1
    return "".join(out)


def marked_text():
    """Every marked piece of text in the menu's source, with where it is."""
    found = {}
    for path in sorted((ROOT / "src").glob("*.[ch]")):
        if path.name in ("menu_text.h", "menu_text_data.h"):
            continue
        source = strip_comments(path.read_text(encoding="utf-8"))
        for match in MARKED.finditer(source):
            text = "".join(unescape(m) for m in LITERAL.findall(match.group(1)))
            line = source.count("\n", 0, match.start()) + 1
            found.setdefault(text, f"{path.relative_to(ROOT).as_posix()}:{line}")
    return found


def font_characters():
    spec = importlib.util.spec_from_file_location("build_ui_font", ROOT / "tools/build-ui-font.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return set(module.CHARACTERS)


def check(marked, translations):
    problems = []
    characters = font_characters()
    for text, where in sorted(marked.items()):
        if text not in translations:
            problems.append(f"{where}: no translation for {text!r}")
    for text, translated in sorted(translations.items()):
        if text not in marked:
            problems.append(f"unused translation: {text!r}")
            continue
        if CONVERSION.findall(text) != CONVERSION.findall(translated):
            problems.append(f"{marked[text]}: conversions differ in {translated!r}")
        unknown = sorted({c for c in translated if c not in characters and c not in "\n"})
        if unknown:
            problems.append(f"{marked[text]}: not in the font: {''.join(unknown)!r} in {translated!r}")
        if not translated.strip():
            problems.append(f"{marked[text]}: empty translation for {text!r}")
    return problems


def c_literal(text):
    """`text` as a C string literal using only ASCII (octal escapes)."""
    out = ['"']
    for byte in text.encode("utf-8"):
        c = chr(byte)
        if c in '"\\':
            out.append("\\" + c)
        elif c == "\n":
            out.append("\\n")
        elif 32 <= byte < 127:
            out.append(c)
        else:
            out.append("\\%03o" % byte)
    out.append('"')
    return "".join(out)


def generate(marked, translations):
    keys = sorted(marked, key=lambda text: text.encode("utf-8"))
    lines = [
        "/* Generated by tools/build-menu-text.py from data/menu-text-uk.json:",
        " * the menu's marked text (sorted by its UTF-8 bytes) and its Ukrainian",
        " * translation. Do not edit; see src/menu_text.h. */",
        "#define MENU_TEXT_COUNT %d" % len(keys),
        "static const char *const menu_text_english[MENU_TEXT_COUNT] = {",
    ]
    lines += ["    " + c_literal(key) + "," for key in keys]
    lines += ["};", "static const char *const menu_text_ukrainian[MENU_TEXT_COUNT] = {"]
    lines += ["    " + c_literal(translations[key]) + "," for key in keys]
    lines += ["};"]
    return "\n".join(lines) + "\n"


def main():
    mode = sys.argv[1] if len(sys.argv) > 1 else None
    if mode not in (None, "--check", "--missing"):
        sys.exit(__doc__)
    marked = marked_text()
    translations = json.loads(TRANSLATIONS.read_text(encoding="utf-8"))
    if mode == "--missing":
        missing = {text: "" for text in sorted(marked) if text not in translations}
        print(json.dumps(missing, ensure_ascii=False, indent=2))
        return
    problems = check(marked, translations)
    if problems:
        sys.exit("\n".join(problems))
    generated = generate(marked, translations)
    if mode == "--check":
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != generated:
            sys.exit("src/menu_text_data.h is out of date; run tools/build-menu-text.py")
        print(f"Menu text: {len(marked)} marked pieces, all translated; data current")
        return
    OUTPUT.write_text(generated, encoding="utf-8", newline="\n")
    print(f"Wrote {OUTPUT.relative_to(ROOT)}: {len(marked)} pieces of text")


if __name__ == "__main__":
    main()
