---
name: ntrt-development
description: Develop and maintain the NTRT C++/Qt trading terminal, including its desktop workspace, market data, settings, build validation, and requested Git commits. Use for work in the NTRT repository; do not apply its project conventions to unrelated applications.
---

# NTRT Development

## Start with the current project

The usual checkout is `D:/CppWorks/NTRT`. Use the user's current checkout when it differs. Read that checkout's `AGENTS.md` before editing; it is the source of truth for code style, ownership, UI conventions, encoding, and documentation scope. Do not copy this skill into source directories or append it to AGENTS.md.

Inspect the working tree and the owning class together with its callers. Repository paths and interfaces have changed during development; search the current tree rather than recreating older structures from conversation history. Preserve unrelated and concurrent user edits, including staged changes.

Choose the smallest complete change that addresses the request. A question about feasibility or recommendations is not authorization to implement the feature.

## Route changes to their owners

| Concern | Starting points under `src/main` |
| --- | --- |
| Application lifecycle and shared services | `NTRT.h`, `NTRT.cpp` |
| Navigation, menus, application actions | `ui/MainWindow` |
| Desktop layout, child-window visibility and arrangement | `ui/Workspace` |
| Child-window titlebar, frame, resizing and hover behavior | `ui/WorkspaceWindow` |
| Instrument list and chart content | `ui/panels/InstrumentListPanel`, `ui/panels/ChartPanel` |
| Settings dialog and setting rows | `ui/dialog/SettingsDialog`, `ui/pages/SettingPage` |
| Typed settings and persistence | `data/ConfigManager`, `data/AppearanceManager` |
| Background subscriptions and lifecycle | `monitor/DataMonitor` |
| Quotes, candles and icon caches | `monitor/MarketData`, `monitor/CandlesManager`, `monitor/InstrumentIconManager` |
| Common exchange interface and OKX behavior | `api/AbstractApi`, `api/impl/OKXApi`, `api/impl/OKXTickerStream` |
| Network lifecycle and request definitions | `network/` |
| Candle types, timeframes and parsing | `trade/TradeDataDef.h`, `trade/CandleTimeframe.h`, `trade/parser/` |

These are discovery hints, not permission to create missing components. Inspect existing account abstractions before extending them; do not invent exchange authentication or trading behavior as part of unrelated work.

## Preserve the desktop interaction model

The workspace uses `QMdiArea` with child windows, not detachable docking windows. Do not reintroduce Qt Advanced Docking System merely because older code or previous requests mention it.

Keep panel content separate from window chrome. Tool windows may move, overlap, resize, minimize and maximize within the desktop. Titlebar close destroys a tool and removes its Window-menu entry; recreate it through the element actions. Window-menu actions toggle visibility of retained tools with synchronized checkmarks. Closing a tool releases its chart subscriptions without stopping shared backend monitoring. Settings remains a separate reusable dialog.

For window interaction changes, check the affected transitions: active/inactive, background-click deselection, hide/reopen, minimize/restore, move/resize, or arrangement. Preserve the usable resize hit area independently of the thin visible outline. Keep frame rendering consistent with titlebar geometry and repaint clipping; do not fix a paint problem by continually repainting the entire application.

For table changes, update column indices, delegates, visibility settings, width calculations, headers and tooltips together. Reuse cells and cached pixmaps; avoid recreating every item on periodic refresh. Skip unnecessary UI refresh for hidden/minimized panels while maintaining backend updates.

## Preserve market-data semantics

- Quote price and daily-candle close are different data sources. Displayed daily percentage change uses `(latest ticker price - daily open) / daily open * 100`, not the last REST candle close.
- Daily volume comes from the selected daily candle. Keep spot versus derivative base-volume units correct and use the actual quote-volume field.
- Trading-data time zone defaults to UTC. Calendar-bar requests and lookup boundaries must agree with the saved setting. Keep OKX protocol names in the exchange implementation.
- Keep instrument, exchange, instrument type and timeframe identities distinct. Switching a series or time zone must not allow a completed old request to overwrite the new cache.
- Treat cached pointers and spans as borrowed data. Inspect their validity and thread constraints before retaining them across updates or passing them to another thread. Queries should not silently trigger UI-owned requests.

When changing persistence, preserve existing configurations, parse into temporary typed state, and replace live settings only after success. Failed saves must not leave UI and backend settings disagreeing. Use the existing JSON helpers and atomic file writer.

## Validate with the intended build

Use Release for ordinary builds and tests. This project's Release presets intentionally omit compiler optimization switches while keeping `NDEBUG` and Release runtime libraries. Do not add `-O2`, `-O3`, `/O2`, sanitizers, profiling instrumentation, or Debug settings without a task-specific reason and user authorization where needed.

```shell
cmake --preset release
cmake --build --preset release
```

When relevant tests need enabling:

```shell
cmake --preset release -DNTRT_BUILD_TESTS=ON
cmake --build --preset release
ctest --test-dir build/release --output-on-failure
```

Use tests that exercise changed behavior: query boundaries, timeframe isolation, asynchronous cancellation, configuration compatibility, or other meaningful invariants. Do not create tests that merely repeat implementation text. Small styling changes generally need a build and visual inspection rather than new unit tests.

Distinguish compilation from successful linking and packaging. A running `NTRT.exe` can prevent replacement on Windows. Do not report a successful build when linking failed, terminate the user's app without authorization, or repeatedly retry an unchanged locked output. Report the limitation and whether visual verification was actually performed. Never claim a measured performance improvement without measurements.

## Finish within scope

Write all text explicitly as UTF-8 and verify modified text decodes cleanly. Avoid locale-dependent shell output when writing documents or configuration.

Do not create task reports, API guides, README files or design notes under `src/`. Do not append task history to README.md or AGENTS.md. Change documentation only when requested or to minimally correct essential instructions made inaccurate by the change; report routine work and verification in chat.

Commit only when requested. Inspect recent subjects and match their short lowercase English style, for example `adjust workspace design and ui performance`. Review the staged diff, include the requested work's new files and deletions, run relevant checks, and verify the resulting commit and working tree. Do not push unless asked.

Reply concisely in the user's language: state what changed, what was verified, and any concrete remaining limitation. Do not turn a failed link or an unverified visual change into a claim that the user has already seen the fix.
