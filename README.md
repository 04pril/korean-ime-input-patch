# KartRider Korean IME Input Patch

KartRider/PopKart client-side Korean IME input patch.

This repository preserves the minimal DLL patch that makes completed Korean IME composition reach the client as `WM_CHAR`.

## What Works

- Korean input that was previously blocked can now enter the client.
- Entered Hangul can be copied back out as real Unicode text.
- The patch is packaged as a `d3dx9_27.dll` proxy loaded next to `KartRider.exe`.

## What This Does Not Fix

- Korean glyph rendering / tofu boxes.
- Font, RHO, BML, or translator bridge changes.
- Anti-cheat bypassing.

## Files

- `src/d3dx9_27_ime_only.cpp`: input-only proxy source.
- `src/d3dx_forwarders.inc`: D3DX9 export forwarders.
- `src/d3dx9_27.def`: export definition file.
- `scripts/build.ps1`: rebuilds the x86 DLL with Visual Studio Build Tools.
- `release/d3dx9_27.dll`: prebuilt input-only proxy DLL.
- `release/KartRider_KoreanInputOnly_DLL.zip`: deployable package without third-party binaries.
- `docs/session-summary.md`: notes from the investigation.

## Install

1. In the game folder, keep the original D3DX DLL as `d3dx9_27_real.dll`.
2. Copy `release/d3dx9_27.dll` into the game folder as `d3dx9_27.dll`.
3. Start `KartRider.exe`.

The repo intentionally does not include the original Microsoft/game-provided `d3dx9_27_real.dll`.

## Build

Run from this repository root:

```powershell
.\scripts\build.ps1
```

The output is written to `build\d3dx9_27.dll` and copied to `release\d3dx9_27.dll`.

## Notes

This is a preservation repo for a private-server/client-mod workflow. It should stay private unless all redistributed files are reviewed for licensing.
