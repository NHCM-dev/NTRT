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
| Child-window creation setup, frame interaction, geometry constraints and per-window menu action | `ui/WorkspaceWindow` |
| Instrument list, chart and book depth content | `ui/panels/InstrumentListPanel`, `ui/panels/ChartPanel`, `ui/panels/BookDepthPanel` |
| Settings dialog and setting rows | `ui/dialog/SettingsDialog`, `ui/pages/SettingPage` |
| Typed settings and persistence | `data/persistence/ConfigManager`, `data/persistence/AppearanceManager`, `data/persistence/WorkspaceManager` |
| Background subscriptions and lifecycle | `monitor/DataMonitor` |
| Quotes, order books, candles and icon caches | `monitor/MarketData`, `monitor/CandlesManager`, `monitor/InstrumentIconManager` |
| Common exchange interface and OKX behavior | `api/AbstractApi`, `api/impl/OKXApi`, `api/impl/OKXTickerStream`, `api/impl/OKXBookStream` |
| Network lifecycle and request definitions | `network/` |
| Candle types, timeframes and parsing | `trade/TradeDef.h`, `trade/TradeDataDef.h`, `trade/parser/` |

These are discovery hints, not permission to create missing components. Inspect existing account abstractions before extending them; do not invent exchange authentication or trading behavior as part of unrelated work.

## Organize shared types by domain

Before adding a struct or enum, inspect the existing domain definitions and its consumers. Keep trading enums and identities in `trade/TradeDef.h`, raw ticker/candle/book records in `trade/TradeDataDef.h`, and published snapshots plus read interfaces in `trade/TradeSnapshotDef.h`. Use `data/config/*ConfigDef.h` for persisted settings and workspace state, `data/core/*Def.h` for shared exchange, appearance, time-zone and workspace enums, and the corresponding `network/NetworkDef.h`, `ui/chart/ChartDef.h`, or `monitor/DataSnapshotDef.h` for other domains. Keep configuration persistence and live-data lifecycle in their managers, and class-private subscription/cache/layout details in their owner classes.

Definition headers must compile independently and directly include the defining headers of referenced types. Keep domain dependencies focused and avoid unnecessary manager, client, or concrete-widget dependencies. Do not use class, struct, or enum forward declarations, including for Qt types, pointers, references, or private worker types. Use includes instead; compile-time savings are not an exception. Retain declarations only when technically required, such as friendship where including the owner would create an unavoidable cycle, and explain each exception at the declaration. Keep cohesive types together rather than creating a header for every small type or an application-wide catch-all. Move snapshot query implementations with their read interfaces, migrate all callers, remove superseded headers, and update explicit build/test source lists.

## Extract focused utilities

When scanning or changing code, move suitable independent logic into focused `XXXUtil` files in `src/main/util`, even if it currently has only one caller. Inspect existing utilities and callers first; extend a matching domain-level Util such as `TradeUtil`, `NumberUtil`, `QtUtil`, or `PaintUtil`. Avoid implementation- or feature-specific names such as `BookUtil`; use a broader, cohesive domain name only when no existing Util fits. Exchange protocol utilities such as `OKXBookUtil` may remain specialized. Prefer compact call sites and a single implementation for parsing, conversion, formatting, calculations, file operations, drawing primitives, and generic Qt operations. Pass dependencies explicitly, keep exchange protocol helpers separate, and retain state and lifecycle policy on the owning classes. Avoid catch-all collections and wrappers that add no clarity. Update callers, build inputs and relevant behavioral checks together.

## Preserve the desktop interaction model

The workspace uses `QMdiArea` with child windows, not detachable docking windows. Do not reintroduce Qt Advanced Docking System merely because older code or previous requests mention it.

Keep panel content separate from window chrome. Workspace constructs and registers every tool through its common window creation path; WorkspaceWindow owns frame interaction, bounded geometry, normal-state geometry and its visibility/title menu action. Constrain the selected resize edges before applying geometry, preserving opposite edges and panel minimum sizes. Preserve requested rectangles across transient viewport changes during layout restoration. Tool windows may move, overlap, resize, minimize and maximize within the desktop. Titlebar close destroys a tool and removes its Window-menu entry; recreate it through the element actions. Window-menu actions toggle visibility of retained tools with synchronized checkmarks. Closing a tool releases its market-data subscriptions without stopping shared backend monitoring. Settings remains a separate reusable dialog.

AppearanceManager owns display scale (25–400%, relative to system scaling), applied through Qt before QApplication construction and changed on restart. Persist workspace rectangles and table headers in logical coordinates without multiplying by display scale. Use WorkspaceWindow::setWindowGeometry() for programmatic placement; viewport and appearance constraints must preserve requested normal rectangles for subsequent saves and restoration at another scale. Restart must flush settings and workspace layout successfully before launching the replacement process, and the exiting session must perform no later settings or layout writes.

For window interaction changes, check the affected transitions: active/inactive, background-click deselection, hide/reopen, minimize/restore, move/resize, or arrangement. Preserve the usable resize hit area independently of the thin visible outline. Keep frame rendering consistent with titlebar geometry and repaint clipping; do not fix a paint problem by continually repainting the entire application.

Every enabled button must provide visible hover feedback, including text/icon buttons, toolbar actions, tab controls and custom titlebar controls. Keep checked buttons visibly responsive on hover without losing their selected indication; disabled buttons must not show interactive feedback. Define Qt widget states in resource QSS using palette roles, and check selector precedence so local styles do not override hover/pressed states. When fixing missing feedback, inspect similar controls and verify normal, hovered, pressed, checked and disabled states in both Light and Dark themes, including runtime switching.

For table changes, update column indices, delegates, visibility settings, width calculations, headers and tooltips together. Reuse cells and cached pixmaps; avoid recreating every item on periodic refresh. Skip unnecessary UI refresh for hidden/minimized panels while maintaining backend updates.

Keep chart input routing in ChartView and transform state in ChartViewport. Price-scale operations must preserve the time transform; distinguish scrolling/panning from pinching/zooming before applying input deltas. With AutoScale disabled, dragging the price plot pans its manual price range vertically without changing its scale; volume-pane drags remain horizontal.

## Format numeric data consistently

Inspect `NumberUtil`, `PaintUtil` and `ui/widgets/NumericWidgets` before adding numeric rendering. Use shared decimal formatting for controls, table values and full-value tooltips; remove redundant trailing zeros and binary floating-point residue while preserving supported small nonzero values. Verify decimal aggregation on both sides of a tick boundary, including large price-to-step ratios. Do not use raw `g16` tooltips or fixed-decimal controls that expose implementation precision to the user.

Latest-price widgets share Instrument List's full-flash and changed-digit phases, global enable setting and palette-based painting. Test enabled, disabled, expired and missing/stale quotes. Quantity and cumulative-depth bars use consistent opacity and relative lengths; specify each normalization basis. Buy/sell balance uses both sides within the selected raw depth, independent of displayed rows or one-sided layout. Empty books show unavailable percentages. Verify actual bars, flash text and percentages through Light → Dark → Light, including hidden and newly created panels.

## Preserve market-data semantics

- Order book windows share reference-counted subscriptions through `DataMonitor` and `MarketData`. Keep sequence validation and snapshot recovery in the exchange implementation; aggregate price buckets with general trading utilities. Persist per-window instrument, aggregation and display options through `WorkspaceManager` while preserving old layouts.
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

Distinguish compilation from successful linking and packaging. A running `NTRT.exe` can prevent replacement on Windows. Do not report a successful build when linking failed, terminate the user's app without authorization, or repeatedly retry an unchanged locked output. Report the limitation and whether visual verification was actually performed. Never claim a measured performance improvement without measurements.

## Finish within scope

Write all text explicitly as UTF-8 and verify modified text decodes cleanly. Avoid locale-dependent shell output when writing documents or configuration.

Do not create task reports, API guides, README files or design notes under `src/`. Do not append task history to README.md or AGENTS.md. Update relevant agent documentation when architecture or core maintenance contracts change; otherwise change documentation only when requested or to minimally correct essential instructions made inaccurate by the change; report routine work and verification in chat.

Commit only when requested. Inspect recent subjects and match their short lowercase English style, for example `adjust workspace design and ui performance`. Review the staged diff, include the requested work's new files and deletions, run relevant checks, and verify the resulting commit and working tree. Do not push unless asked.

Reply concisely in the user's language: state what changed, what was verified, and any concrete remaining limitation. Do not turn a failed link or an unverified visual change into a claim that the user has already seen the fix.

## Instrument dispatch and durable architecture

Workspace owns the `instRequiredWindow` registry and interaction ordering. Instrument List activation applies once to at most one Chart and one BookDepth in the current workspace. Exclude hidden, minimized and destroyed windows; prefer empty windows, then the most recently activated window of each type. Freeze targets before applying so activation cannot redirect the same dispatch. Register restored windows and preserve panel-owned subscription lifecycle.

When a change affects architecture, ownership, core interfaces or behavior that future maintenance depends on, update the relevant existing agent documentation in the same task. Record durable contracts and maintenance constraints only, not task logs or implementation walkthroughs. Preserve unrelated documentation and staged changes.

Order-book bucket quantities should accumulate in wider precision before conversion and use quantity formatting for cells and tooltips. Keep price precision separate. A deeper REST snapshot and a live WebSocket book have different timestamps and coverage; never stitch them into a purportedly synchronized live book without a supported synchronization protocol. Expose source limits and refresh cadence when adding depth sources.
