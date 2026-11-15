# Agent Startup and Git Tracking

- At the start of every task in this repository, including follow-up tasks and newly opened chats, read the current on-disk `AGENTS.md` and repository-root `SKILL.md` before planning or editing. Read any additional applicable scoped `AGENTS.md` and relevant skill `SKILL.md` files before working in their scope. Do not rely solely on conversation history or previously cached copies.
- Re-read these instructions when they change during a task. Reading the latest instructions does not mean rewriting them after every task; preserve the documentation scope rules below.
- Whenever creating files, check whether they belong in version control. Always add necessary project source files, tests, runtime resources, and requested documentation to Git tracking with explicit paths. Keep generated build outputs, caches, temporary files, secrets, and intentionally ignored local files untracked; do not force-add ignored files without a concrete reason and authorization.
- Preserve existing staged and unrelated changes. Adding new project files to Git tracking does not authorize committing or pushing; commit and push only when requested.

# C++ Code Style

- Do not use `auto`. Write explicit types for variables, return values, and lambda parameters.
- Keep formatting compact. Keep short declarations, calls, and argument lists on one line. Wrap long lines only when it improves readability, and use blank lines to separate logical steps.
- Match the surrounding code. Use four spaces for indentation and put opening braces on their own line.
- Keep error handling at the boundary that can act on it. Validate external input and check I/O results where they matter. Let errors propagate through intermediate code and avoid repeated checks or blanket catches.
- Use postfix increment and decrement: `index++` and `index--`.
- Do not use anonymous or nested helper namespaces. Use private class methods for helpers that depend on their owning class state; extract independent utility logic into focused `XXXUtil` files under `src/main/util`.
- Put Qt styles in QSS resource files. Do not embed style sheets as string literals in C++ code.
- Split UI initialization into focused private `initXXX()` methods. Keep constructors and widget factory methods short and use them to coordinate the initialization steps.
- Follow `ConfigManager` and `AppearanceManager` for JSON persistence. Keep settings in a typed config struct, load them with the existing simdjson helpers, and use `newBuilder()` and `writeJsonFile()` to save formatted JSON atomically. Create defaults only when the file is missing and replace in-memory settings only after a successful parse.
- Add settings and configuration UI in `SettingPage`. Use its sections and row helpers to expose controls, and delegate persistence to the corresponding configuration manager.
- Use `switch` when behavior or values differ by enum member. Avoid enum-based `if` chains and ternary expressions for these branches.

## Utility Extraction

- Scan the affected code and callers for logic that belongs in `XXXUtil`: parsing, conversion, formatting, calculations, file operations, drawing primitives, and generic Qt operations. Extract suitable logic even when it has only one caller.
- Reuse a suitable existing Util or create a focused `XXXUtil`; keep callers compact and remove duplicate implementations. Do not collect unrelated helpers into a catch-all Util or wrap simple calls without making their intent clearer.
- Utilities take their dependencies as arguments. Keep feature state, configuration persistence policy, subscriptions, caches, scheduling, and lifecycle operations in their owning classes. Keep exchange-specific protocol helpers separate from general utilities. Update all callers and relevant build/test targets when extracting code.

## Class Responsibilities and Interfaces

- Use object-oriented design. Encapsulate related state and behavior in the owning class and keep responsibilities and interfaces focused.
- Read the existing owner class and its callers before adding a feature. Put state and operations on the class that owns them. Use private member methods for work that uses that class's state.
- Use `ntrt::` for shared application objects and application-wide actions, following `ntrt::configManager`, `ntrt::dataMonitor`, and `ntrt::quitApp()`. Keep feature state and logic inside the owning classes.
- Use a `Manager` for state and its lifecycle, such as loading, saving, and updating settings in `ConfigManager` and `AppearanceManager`. Let the manager own its subcomponents and expose them through focused accessors such as `ConfigManager::getAppearanceManager()`.
- Keep monitoring and scheduling in `DataMonitor`. Follow its `start()`, `stop()`, and `getMarketData()` interface. Keep fetching, caching, and snapshot lookup in `MarketData`.
- Integrate real-time data through the background `DataMonitor` and keep live data in memory in its owned data components. Store data that can be persisted under the `app` directory, with persistence handled by the owning data component or manager.
- Keep UI classes responsible for reading data and displaying it. Do not perform API calls, collect data, or own data-fetching and monitoring logic in the UI; access data through the owning backend interfaces.
- Use a `Client` for a live network connection and its lifecycle. Keep endpoint or channel definitions separate, following `RequestTemplate` for addresses and parameter rules. Put exchange-specific messages, parsing, and protocol behavior in the exchange implementation behind a common interface, following `AbstractApi` and `OKXApi`.
- Keep mutable members private. Use `getX() const` for reads, `isX()` for boolean state, and `setX(...)` for setting a value. Return a read-only reference or view when callers only need to inspect owned state. Use named operations such as `addInstrument()`, `removeInstrument()`, and `moveInstrument()` for changes with their own meaning.

## UI Layout

- Use a top menu bar and movable Workspace tabs for navigation. Each `Workspace` remains a QFrame with a `QMdiArea` desktop background and independent `QMdiSubWindow` child windows.
- Tools live inside the desktop: allow moving, overlapping, resizing, minimizing and maximizing, but never detach them into native top-level OS windows. Do not use floating docking containers for workspace tools.
- `Workspace` owns child-window construction and layout coordination; `MainWindow` owns navigation, menus and application actions. Keep panel content in `ui/panels` separate and reusable.
- Give each child window a stable unique objectName. Titlebar close destroys the child window and its content, removes its Window-menu entry, and releases its chart subscriptions. Shared backend monitoring continues independently. Window menu checkmarks follow visibility for retained windows.
- Provide Reset Layout, Stack Windows and Sort Window actions in the Window menu.
- Settings remains a standalone reusable `SettingsDialog` containing `SettingPage`; keep settings sections and row helpers in `SettingPage`.
- UI panels only read backend data. DataMonitor's lifecycle is independent of window visibility. Keep styling in resource QSS files and support palette changes.
- Every UI change must support both Light and Dark themes, including runtime switching. Use palette roles for theme-dependent colors; keep foregrounds, backgrounds, borders, hover/selection states, and chart colors consistent. Invalidate resolved QSS palettes and custom render caches when the theme changes; do not leave widgets using colors from the previous theme.
- Verify Light → Dark → Light switching on existing, hidden, and newly created UI. Cover menus, Workspace tabs, child-window chrome, tables, inputs, settings, and OpenGL charts; check contrast and actual rendered output, not only the application palette.

## Git Commits

- When asked to stage and commit, inspect the working tree, staged changes, and recent commit messages first. Stage all files belonging to the requested work, including new files and deletions, while keeping unrelated changes out of the commit.
- Match the existing commit style: a short, lowercase English subject describing the concrete change, following messages such as `added flash on price change`, `fix missing file`, and `basic settings page impl`. Do not add Conventional Commits prefixes unless the repository starts using them or the user requests them.
- Run relevant checks before committing and report any checks blocked by the environment accurately. Inspect the final staged diff, commit the requested changes, then verify the resulting commit and working tree. Report the commit hash and subject. Do not push unless requested.

## Build and Performance

- Use the `release` CMake configure/build preset for everyday CLion runs and validation: `cmake --preset release`, `cmake --build --preset release`. Use Debug only when debugging is explicitly required.
- Reuse UI items and avoid recreating table cells on periodic refresh. Skip refresh for hidden/minimized tool windows; backend monitoring must continue independently.

- Release presets intentionally omit compiler optimization switches (such as -O2, -O3 or /O2) per user preference. Keep NDEBUG and Release runtime libraries; do not add optimization flags without an explicit request.

## File Encoding and Documentation Scope

- All project text files must use UTF-8. Always specify UTF-8 explicitly when reading or writing files; never rely on the Windows locale, GBK, ANSI, or shell redirection defaults. Preserve binary assets as binary.
- Preserve readable text and normal line endings. Do not silently replace decoding errors, introduce mojibake, or duplicate carriage returns. Verify modified files decode as UTF-8 before finishing.
- Documentation changes are not a routine completion step. Do not append task summaries, implementation walkthroughs, transient limitations, verification logs, or feature-by-feature notes to README.md, AGENTS.md, or other documents after every task. Put task results in the chat response.
- Change documentation only when explicitly requested, or when a change directly makes existing essential setup, usage, or architecture instructions incorrect. In the latter case, correct the relevant existing passage with the smallest necessary edit; do not add unrelated sections.
- Do not create README files, design notes, API guides, task reports, or other explanatory documents under src/. Source directories are for implementation and required runtime resources. Do not generate a new document elsewhere as a substitute unless requested.
- Keep README.md short: project purpose, prerequisites, build and test commands. Do not duplicate source-level details or use it as a changelog.
- Keep AGENTS.md focused on durable contributor instructions. Add rules only when requested or needed to correct an existing instruction; do not turn individual implementation choices into new repository-wide rules.
