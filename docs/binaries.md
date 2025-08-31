# Binaries Overview

This project builds several executables.

---

## Server

- **`libmgr-server`**
- Central coordinator and DB manager.
- Args: `<dbfile> <port>`

---

## Workers

- **`libmgr-script-worker`**
- **`libmgr-audio-worker`**
- **`libmgr-image-worker`**
- **`libmgr-video-worker`** (planned)

Each connects to server and handles its own file type.

---

## CLI

- **`libmgr-cli`**
- User-facing CLI for listing types and files.

---

## Utilities

- **`scan-file-cli`**
- Directly scan one file (bypassing server) for debugging.

