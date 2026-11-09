# C++ Code Style

- Do not use `auto`. Write explicit types for variables, return values, and lambda parameters.
- Keep formatting compact. Keep short declarations, calls, and argument lists on one line. Wrap long lines only when it improves readability, and use blank lines to separate logical steps.
- Match the surrounding code. Use four spaces for indentation and put opening braces on their own line.
- Keep error handling at the boundary that can act on it. Validate external input and check I/O results where they matter. Let errors propagate through intermediate code and avoid repeated checks or blanket catches.
- Use postfix increment and decrement: `index++` and `index--`.
- Do not use anonymous or nested helper namespaces. Use private class methods for helpers that belong to a class.
- Put Qt styles in QSS resource files. Do not embed style sheets as string literals in C++ code.
- Split UI initialization into focused private `initXXX()` methods. Keep constructors and widget factory methods short and use them to coordinate the initialization steps.
