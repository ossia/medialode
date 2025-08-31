# Quickstart Guide

This document explains how to set up, build, and run medialode locally.
It covers the server, workers, and CLI tools, with examples for each.

---

1. Build

Clone and build the project:

```bash
git clone git@github.com:ossia/medialode.git medialode
cd medialode
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

This produces binaries under `./build/app/`.

2. Architecture Overview

- libmgr-server
Central orchestrator. Manages the SQLite database and delegates scan tasks.

- Workers (`libmgr-<type>-worker`)
Specialized processes that analyze files (image, audio, video, script).
Workers connect to the server and receive file scan requests.

- CLI Tools

  `scan-file-cli` → Request a worker to scan a specific file.

  `scan-folders-cli` → Recursively scan folders into the DB.

  `libmgr-cli` → Query the DB (list types, list files, etc).

### Why this design?

- Server provides durability and concurrency.

- Workers provide modularity and extensibility.

- CLI provides a user-facing entry point.

3. Running the System

All components run in separate terminals.
The order is always: server → worker(s) → client/CLI.

3.1 Run the Server

```bash
./build/app/libmgr-server
```

By default, the server:

- Listens on `127.0.0.1:8081`.

- Stores data in `medialode.db` (SQLite).

3.2 Run a Worker

Workers connect to the server and handle scan requests.
Each worker focuses on a specific type (`image`, `audio`, `video`, `script`).

Example: Image Worker

```bash
./build/app/libmgr-image-worker/libmgr-image-worker --host 127.0.0.1 --port 8081
```

Example: Audio Worker

```bash
./build/app/libmgr-audio-worker/libmgr-audio-worker --host 127.0.0.1 --port 8081
```

Example: Video Worker

```bash
./build/app/libmgr-video-worker/libmgr-video-worker --host 127.0.0.1 --port 8081
```

Example: Script Worker

```bash
./build/app/libmgr-script-worker/libmgr-script-worker --host 127.0.0.1 --port 8081
```

3.3 Run CLI Tools
Scan a single file

Send a `ScanFileRequest` to the server:

```bash
./build/app/scan-file-cli --host 127.0.0.1 --port 8081 <path_to_file>
```

Recursively scan a folder

```bash
./build/app/scan-folders-cli <path_to_directory>
```

List supported types

```bash
./build/app/libmgr-cli/libmgr-cli list types
```

Expected output:

```
Available types:
  - image
  - audio
  - video
  - script
```

List files in a folder (with optional type filter)

Query the DB for files:

```bash
./build/app/libmgr-cli/libmgr-cli list files --folder <path_to_folder>
```

Filter by type:

```bash
./build/app/libmgr-cli/libmgr-cli list files --type audio --folder <path_to_folder>
```

Scan and list in one step

Force a fresh recursive scan before listing:

```bash
./build/app/libmgr-cli/libmgr-cli list files --type audio --folder <path_to_folder> --scan
```

4. Database

Data is stored in `medialode.db` (SQLite).
You can inspect it manually:

```bash
sqlite3 medialode.db
sqlite> .tables
sqlite> SELECT path, kind FROM files LIMIT 10;
```

This is useful to debug what files and metadata have been inserted.

5. Putting it All Together (Example Run)

   1. Start server:

   ```bash
   ./build/app/libmgr-server
   ```

    2. Start audio worker:

   ```bash
   ./build/app/libmgr-audio-worker/libmgr-audio-worker
   ```

    3. Scan a directory:

   ```bash
   ./build/app/libmgr-cli/libmgr-cli list files --type audio --folder ~/Music --scan
   ```

    4. Query results:

   ```bash
   ./build/app/libmgr-cli/libmgr-cli list files --type audio --folder ~/Music
   ```

