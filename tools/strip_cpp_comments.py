import os
import sys


def strip_cpp_comments(text: str) -> str:
    OUT, SLASH, LINE, BLOCK, STRING, CHAR, ESC = range(7)
    state = OUT
    quote = ""
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]

        if state == OUT:
            if c == "/":
                state = SLASH
            elif c == '"' or c == "'":
                quote = c
                out.append(c)
                state = STRING if c == '"' else CHAR
            else:
                out.append(c)
            i += 1
            continue

        if state == SLASH:
            if c == "/":
                state = LINE
                i += 1
            elif c == "*":
                state = BLOCK
                i += 1
            else:
                out.append("/")
                state = OUT
            continue

        if state == LINE:
            if c == "\n":
                out.append("\n")
                state = OUT
            i += 1
            continue

        if state == BLOCK:
            if c == "*" and i + 1 < n and text[i + 1] == "/":
                state = OUT
                i += 2
            else:
                # Preserve newlines so line numbers stay stable.
                if c == "\n":
                    out.append("\n")
                i += 1
            continue

        if state == STRING or state == CHAR:
            out.append(c)
            if c == "\\":
                state = ESC
                i += 1
                continue
            if c == quote:
                state = OUT
            i += 1
            continue

        if state == ESC:
            # Escape inside string/char literal
            if i < n:
                out.append(text[i])
                i += 1
            state = STRING if quote == '"' else CHAR
            continue

    if state == SLASH:
        out.append("/")

    return "".join(out)


def should_skip_path(path: str) -> bool:
    norm = path.replace("\\", "/")
    if "/Win32/" in norm:
        return True
    if "/__history/" in norm:
        return True
    if "/.cursor/" in norm:
        return True
    return False


def iter_targets(root: str):
    for dirpath, dirnames, filenames in os.walk(root):
        for name in filenames:
            if not (name.endswith(".cpp") or name.endswith(".h")):
                continue
            p = os.path.join(dirpath, name)
            if should_skip_path(p):
                continue
            yield p


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else os.getcwd()
    changed = 0
    for p in iter_targets(root):
        try:
            with open(p, "r", encoding="utf-8", errors="ignore") as f:
                src = f.read()
            stripped = strip_cpp_comments(src)
            if stripped != src:
                with open(p, "w", encoding="utf-8", newline="") as f:
                    f.write(stripped)
                changed += 1
                print("stripped:", os.path.relpath(p, root))
        except Exception as e:
            print("error:", p, e, file=sys.stderr)
    print("done, files changed:", changed)


if __name__ == "__main__":
    main()

