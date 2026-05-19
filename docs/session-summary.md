# Session Summary

## Goal

Fix Korean input in `KartRider.exe` / PopKart client without using a text bridge.

## Result

The final preserved patch only addresses the input side.

The client originally blocked normal Korean IME input. The working DLL hooks the message loop and, when Windows IME produces `GCS_RESULTSTR`, posts the completed Hangul syllables back to the game window as `WM_CHAR`. After this change, Korean text such as `가나다` reaches the client and can be copied out as real Unicode text.

## Current Limitation

Rendering is still separate. The game may still display tofu boxes for Hangul because the internal UI/font renderer does not resolve those glyphs in the active font path. The input-only DLL does not modify fonts, RHO archives, BML files, bitmap font atlases, or FreeType.

## Investigation Notes

- `WM_CHAR` receives real Unicode Hangul values such as `0xAC00`, `0xB098`, and `0xB2E4`.
- `Data/gui_font.rho` and `Data/aaa.pk` are opened by the client.
- Changing `font@*.bml` default fonts and embedding Korean TTF/bitmap fonts did not resolve tofu rendering.
- The FreeType proxy did not log calls during the tested input/render path, suggesting the chat/input renderer does not use the expected TTF route.
- Some Chinese glyphs were visible, which points to the render issue being font/path/encoding selection rather than the IME layer.

## Files Deliberately Excluded

- Full game client files.
- RHO/RHO5/PK data archives.
- Original `d3dx9_27_real.dll`.
- Experimental logging/probing DLL builds.

This repo is meant to preserve the working input patch and enough source to rebuild it.
