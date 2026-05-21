# LeafEdit: Text Editor for all!

> **Note: Since this software is under development, not all features are supported.**

An **immediate-mode** GUI text editor. Very simple and lightweight. It supports a directory tree and binary hex editing. LeafEdit is **open-source**. If something is missing, feel free to send a PR!

This text editor prioritizes the latest versions of Ubuntu and Fedora. Windows and macOS support is not currently on our roadmap.

## Build

Need packages: SDL3, FreeType, OpenGL3

Need tools: CMake, Make, [Just](https://just.systems/)

```bash
$ just setup
$ just # running program
```

or use Cmake

```bash
$ cmake -S . -B build
$ cmake --build build
$ ./build/leafedit
```

## History

- 2026-05-18 - Repository created. Project started!
- 2026-05-20 - Project made public.