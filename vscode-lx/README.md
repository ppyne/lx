# Lx VS Code Extension

Provides syntax highlighting, snippets, and basic completions for Lx.

## Install locally

1) Open VS Code.
2) Run **Extensions: Install from Location...**
3) Select the `vscode-lx` folder from this repository.

## Build a .vsix

The simplest way is to use `vsce`:

```sh
cd vscode-lx
npm install -g @vscode/vsce
vsce package
```

This produces `lx-0.1.0.vsix`, which you can install with
**Extensions: Install from VSIX...**.

## Features

- Syntax highlighting for `.lx` files
- Basic code snippets (`if`, `for`, `foreach`, `function`, `switch`)
- Completions for built-in functions and keywords
