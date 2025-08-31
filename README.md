# Medialode

Medialode is a modular **media library manager** built around workers, a central server, and a simple CLI.  
It scans directories, extracts metadata (image, audio, video, scripts), and stores it in a **SQLite database**.  
The system is designed to be extensible: new workers can be added to support additional file types.

---

## Features

- **Recursive folder scanning** with metadata extraction  
- **Image support** (width, height, channels)  
- **Audio support** (duration, sample rate, channels, bitrate, format)  
- **Video support** (duration, resolution, codec, framerate)  
- **Script support** (language detection, syntax validation, error reporting)  
- **Central server + SQLite DB** with metadata tables  
- **Workers** for distributed file processing  
- **CLI tools** to scan and query the media library  

---

## Architecture

Medialode is built from **3 main components**:

1. **libmgr-server**  
   - Central orchestrator  
   - Hosts a SQLite database (`medialode.db`)  
   - Accepts WebSocket connections from workers and clients  

2. **Workers** (`libmgr-*-worker`)  
   - Specialized processes (image, audio, video, script)  
   - Handle file-specific metadata extraction  
   - Communicate with server over JSON-over-WebSocket  

3. **CLI tools**  
   - `scan-file-cli`: send a file to be scanned  
   - `scan-folders-cli`: scan entire directories recursively  
   - `libmgr-cli`: general-purpose CLI for listing types, files, etc.  

---

## Quickstart

See [docs/quickstart.md](docs/quickstart.md) to get started.

## Other Documentation

See:
- [docs/binaries.md](docs/binaries.md) for the binaries
- [docs/cli.md](docs/cli.md) for the CLI
- [docs/DB.md](docs/DB.md) for the database schema
- [docs/ipc-spec.md](docs/ipc-spec.md) for the message protocol
- [docs/server.md](docs/server.md) for the server
- [docs/workers.md](docs/workers.md) for the workers

## Current State and Next Steps

Refer to [docs/roadmap.md](docs/roadmap.md)
