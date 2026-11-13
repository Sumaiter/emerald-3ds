#!/usr/bin/env python3
"""Select real menu functions for the native regression harness.

MinGW retains references from unrelated main-menu data even with gc-sections.
Keep the tested function bodies verbatim; only rendering and the return to the
ordinary main menu are substituted by probes in starter_menu_test.c.
"""
import argparse
from pathlib import Path
import re


def function(source, name):
    match = re.search(r"^(?:static )?[^\n;{}]*\b" + re.escape(name)
                      + r"\([^;{}]*\)\n\{", source, re.M)
    if match is None:
        raise ValueError(f"Missing engine function: {name}")
    start = source.index("{", match.start())
    depth, end = 1, start + 1
    # The selected engine bodies contain no braces inside strings/comments.
    while depth and end < len(source):
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    if depth:
        raise ValueError(f"Unbalanced engine function: {name}")
    return source[match.start():end] + "\n"


def generate(root):
    source = (root / "src/main_menu.c").read_text(encoding="utf-8")
    names = (
        "CB2_MainMenu", "CB2_InitJoinGameMenu", "Task_StarterSelection",
        "CB2_StarterSelectionActive", "CB2_InitStarterSelection",
        "JoinGameHighlight", "OpenJoinPassword", "JoinSelectedGame",
        "ContinueJoinedGame", "Task_JoinGameSearch",
    )
    headers = "\n".join(re.findall(r'^#include "[^\n]+"', source, re.M))
    layout = source[source.index("#define MENU_LEFT "):source.index("static const struct WindowTemplate sWindowTemplates_MainMenu")]
    labels = source[source.index("static const u8 sJoinGameTextEn"):source.index("static u8 sPortMenuDebugTaskId")]
    # labels starts just inside PLATFORM_3DS's conditional.
    labels = "#ifdef PLATFORM_3DS\n" + labels
    starters = source[source.index("static const u8 sStarterTitleEn"):source.index("static void JoinGameHighlight(u8 selected);")]
    declarations = "\n".join(body[:body.index("{")].rstrip() + ";"
                             for body in (function(source, n) for n in names))
    return ("/* Generated from the staged engine; do not edit. */\n"
            + headers + "\n" + layout + labels + starters + declarations
            + "\nstatic bool8 sJoining;\nstatic struct CtrCoopFoundGame sJoiningGame;\n"
            + "static void JoinGamePrint(u8, const u8 *);\n"
            + "static void InitJoinGameMenu(void);\nstatic u32 InitMainMenu(bool8);\n"
            + "\n".join(function(source, n) for n in names)
            # Raw PORT_BRIDGE task offsets describe the 32-bit ARM ABI. The
            # native scheduler uses the same source's field-based C branch.
            + '\n#undef PORT_BRIDGE\n#include "' + (root / "src/task.c").resolve().as_posix() + '"\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    content = generate(args.source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(content, encoding="utf-8")


if __name__ == "__main__":
    main()
