# Contributing to vulkify

:+1::tada: First off, thanks for taking the time to contribute! :tada::+1:

The following is a set of guidelines for contributing to repositories hosted in the [vulkify Organization](https://github.com/vulkify) on GitHub. These are mostly guidelines, not rules. Use your best judgment, and feel free to propose changes to this document in a pull request.

#### Table Of Contents

[Code of Conduct](#code-of-conduct)

[What should I know before I get started?](#what-should-i-know-before-i-get-started)
  * [Philosophy](#philosophy)
  * [Library Development](#library-development)

[How Can I Contribute?](#how-can-i-contribute)
  * [Reporting Bugs](#reporting-bugs)
  * [Suggesting Enhancements](#suggesting-enhancements)
  * [Pull Requests](#pull-requests)

[Styleguides](#styleguides)
  * [Git Commit Messages](#git-commit-messages)
  * [C++ Styleguide](#c-styleguide)
  * [Documentation Styleguide](#documentation-styleguide)

## Code of Conduct

This project and everyone participating in it is governed by the [vulkify Code of Conduct](https://github.com/vulkify/vulkify/CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

* [Github Discussions](https://github.com/vulkify/vulkify/discussions)
* [vulkify FAQ](https://github.com/vulkify/vulkify/wiki/FAQ)

## What should I know before I get started?

### Philosophy

`vulkify` is designed to be lightweight, simple, performant, and following modern best practices. It builds everything from source to have maximum target platform coverage without having to ship/require third-party binaries. It uses the subset of C++20 currently supported by CMake and the three major compilers (GCC, Clang, MSVC): no coroutines, ranges, or modules. It packs all external dependencies into an archive to circumvent git submodules / CMake FetchContent calls, and for the CMake configure step to be super fast. It provides a number of useful default presets in `cmake/CMakePresets.json` - not the root directory so as to make it optional for users / IDEs that pick such files up automatically. Its API is primarily focused on ease-of-use, but also supports advanced usage and provides such facilities. Contributors are urged to align with these ideas / examples.

### Library Development

While users of `vulkify` are mostly protected from incorrect / invalid states, library contributors need more guardrails:

1. Vulkan validation layers: Use Vulkan Configurator and enable all validation types (except Best Practices). Keep it running while developing / debugging `vulkify`. You will need to have Vulkan SDK in `PATH` (or export `VULKAN_SDK` etc).
1. UBSan: ideally, work with UB Sanitizer enabled to catch inadvertent bugs. ASan will unfortunately trigger lots of false positives from the video driver, windowing system, etc.

## How Can I Contribute?

### Reporting Bugs

WIP

### Suggesting Enhancements

WIP

### Pull Requests

WIP

## Styleguides

### Git Commit Messages

* Use the present tense ("Add feature" not "Added feature")
* Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
* Limit the first line to 72 characters or less
* Reference issues and pull requests liberally after the first line
* When only changing documentation, include `[ci skip]` in the commit title
* Consider starting the commit message with an applicable emoji:
    * :art: `:art:` when improving the format/structure of the code
    * :racehorse: `:racehorse:` when improving performance
    * :memo: `:memo:` when writing docs
    * :penguin: `:penguin:` when fixing something on Linux
    * :apple: `:apple:` when fixing something on macOS
    * :checkered_flag: `:checkered_flag:` when fixing something on Windows
    * :bug: `:bug:` when fixing a bug
    * :fire: `:fire:` when removing code or files
    * :green_heart: `:green_heart:` when fixing the CI build
    * :white_check_mark: `:white_check_mark:` when adding tests

### C++ Styleguide

All C++ code is formatted with [clang-format](https://clang.llvm.org/docs/ClangFormat.html).

* Use west const (`int const` vs `const int`)
* Minimize includes in headers
  * Use forward declarations where possible except at boundary code (user should not have to include another header to use an API)
* Avoid platform-dependent code

### Documentation Styleguide

* Follow existing Doxygen style doc comments
* Boundary code should have doc-comments unless code is self-documenting
* Doxygen integration and docs generation pending