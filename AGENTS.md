# C++ Code Style

- Do not use `auto`. Write explicit types for variables, return values, and lambda parameters.
- Keep formatting compact. Keep short declarations, calls, and argument lists on one line. Wrap long lines only when it improves readability, and use blank lines to separate logical steps.
- Match the surrounding code. Use four spaces for indentation and put opening braces on their own line.
- Keep error handling at the boundary that can act on it. Validate external input and check I/O results where they matter. Let errors propagate through intermediate code and avoid repeated checks or blanket catches.
- Use postfix increment and decrement: `index++` and `index--`.
- Do not use anonymous or nested helper namespaces. Use private class methods for helpers that belong to a class.
- Put Qt styles in QSS resource files. Do not embed style sheets as string literals in C++ code.
- Split UI initialization into focused private `initXXX()` methods. Keep constructors and widget factory methods short and use them to coordinate the initialization steps.
- Follow `ConfigManager` and `AppearanceManager` for JSON persistence. Keep settings in a typed config struct, load them with the existing simdjson helpers, and use `newBuilder()` and `writeJsonFile()` to save formatted JSON atomically. Create defaults only when the file is missing and replace in-memory settings only after a successful parse.
- Add settings and configuration UI in `SettingPage`. Use its sections and row helpers to expose controls, and delegate persistence to the corresponding configuration manager.
- Use `switch` when behavior or values differ by enum member. Avoid enum-based `if` chains and ternary expressions for these branches.

## Class Responsibilities and Interfaces

- Read the existing owner class and its callers before adding a feature. Put state and operations on the class that owns them. Use private member methods for work that uses that class's state.
- Use `ntrt::` for shared application objects and application-wide actions, following `ntrt::configManager`, `ntrt::dataMonitor`, and `ntrt::quitApp()`. Keep feature state and logic inside the owning classes.
- Use a `Manager` for state and its lifecycle, such as loading, saving, and updating settings in `ConfigManager` and `AppearanceManager`. Let the manager own its subcomponents and expose them through focused accessors such as `ConfigManager::getAppearanceManager()`.
- Keep monitoring and scheduling in `DataMonitor`. Follow its `start()`, `stop()`, and `getMarketData()` interface. Keep fetching, caching, and snapshot lookup in `MarketData`.
- Use a `Client` for a live network connection and its lifecycle. Keep endpoint or channel definitions separate, following `RequestTemplate` for addresses and parameter rules. Put exchange-specific messages, parsing, and protocol behavior in the exchange implementation behind a common interface, following `AbstractApi` and `OKXApi`.
- Keep mutable members private. Use `getX() const` for reads, `isX()` for boolean state, and `setX(...)` for setting a value. Return a read-only reference or view when callers only need to inspect owned state. Use named operations such as `addInstrument()`, `removeInstrument()`, and `moveInstrument()` for changes with their own meaning.
