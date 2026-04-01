# Research_NetworkSimulater

A web-based simulator for **anonymous dynamic networks**, compiled to WebAssembly via Emscripten and served through a local Node.js HTTP server.

---

## Quick Start

If the project has already been built, just run this from the project root:

```bat
serve.bat
```

Then open **http://localhost:8000/** in your browser.

---

## Prerequisites

| Tool | Purpose |
|------|---------|
| [Emscripten](https://emscripten.org/) | Compile C sources to WebAssembly |
| SDL3 libraries | Expected at `C:\Emscripten\libs\sdl3minimal\prefix` |
| [Node.js](https://nodejs.org/) | Run the local development server |

---

## How to Launch

All commands should be run from the **project root**.

### 1. Build

Compiles the C sources to WebAssembly and places output files in `website/`.

```bat
build.bat
```

> The server starts and the browser opens automatically on a successful build.

To build **without** opening the browser:

```bat
build.bat --no-open
```

### 2. Serve only (skip rebuild)

If `website/` already contains a build, start the server directly:

```bat
serve.bat
```

> **Note:** `serve.bat` exits with an error if `website\index.html` is missing — run `build.bat` first.

---

## Project Structure

```
build.bat     — Emscripten build script
serve.bat     — Local server launcher
serve.js      — Node.js HTTP server
website/      — Build output (served to the browser)
  index.html
  index.js
  index.wasm
  index.data
```
