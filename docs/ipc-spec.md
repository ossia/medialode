# IPC Protocol Specification

This document describes the **JSON-over-WebSocket** messages exchanged between:

- **libmgr-server** (central database + orchestrator)
- **libmgr-workers** (scanner processes: image, audio, video, script)
- **CLI clients** (e.g. `libmgr-cli`, `scan-file-cli`)

All messages are JSON objects with a required `"type"` field.

---

## Why WebSocket + JSON?

- **WebSocket** provides a persistent, bidirectional channel between server, workers, and clients.
- **JSON** keeps the protocol human-readable and easy to debug.
- The system is designed so workers can crash/restart independently without blocking the server.

---

## Ping

- **Purpose**: Keepalive between server, client, and worker.

- **PingRequest**

```json
{ "type": "PingRequest" }
```

- **PingResponse**

```json
{ "type": "PingResponse", "message": "pong" }
```

## Worker Identity

### WorkerHello

- Sent by workers on connect.
- Identifies the worker and authenticates with the server.

```json
{
  "type": "WorkerHello",
  "role": "scan-worker",
  "token": "changeme",
  "name": "worker-1"
}
```

### Fields:

- role — always "scan-worker".

- token — must match server’s LIBMGR_WORKER_TOKEN.

- name — worker identifier (for logs, debugging).

## Client Identity

### ClientHello

- Sent by CLI or other clients.
- Currently only carries an optional token.

```json
{
  "type": "ClientHello",
  "token": ""
}
```

## Folder Scan

- Purpose: Populate folders and files tables with recursive scan.

- When used: CLI or API asks server to discover new files.

- ScanFoldersRequest

```json
{
  "type": "ScanFoldersRequest",
  "paths": ["/tmp", "/home/user/Music"]
}
```

- ScanFoldersResponse

```json
{
  "type": "ScanFoldersResponse",
  "scanned_folders": 123,
  "scanned_files": 4567
}
```

## File Scan

- Purpose: Ask a worker to extract metadata for a single file.

- When used: Server → worker after ScanFileRequest.

- ScanFileRequest

```json
{
  "type": "ScanFileRequest",
  "request_id": "abcd123",
  "path": "/tmp/example.wav"
}
```

- FileScanned

```json
{
  "type": "FileScanned",
  "request_id": "abcd123",
  "path": "/tmp/example.wav",
  "size": 102400,
  "mtime": 1717776000
}
```

Worker-specific metadata fields (examples):

- Image: width, height, channels

- Audio: duration, sample_rate, channels, bitrate, format

- Video: duration, width, height, codec, framerate

- Script: language, syntax_ok, error_message

- FileError

If worker cannot process file:

```json
{
  "type": "FileError",
  "request_id": "abcd123",
  "path": "/tmp/missing.txt",
  "error": "file not found"
}
```

## Type Discovery

- Purpose: Tell clients which kinds of files the system knows about.

- When used: CLI list types.

- ListTypesRequest

```json
{ "type": "ListTypesRequest" }
```

- ListTypesResponse

```json
{
  "type": "ListTypesResponse",
  "types": ["image", "audio", "video", "script"]
}
```

## File Listing

- Purpose: Query the server DB for known files.

- When used: CLI list files --type <kind> --folder <path>.

- ListFilesRequest

```json
{
  "type": "ListFilesRequest",
  "kind": "audio",
  "folder": "/home/user/Music"
}
```

### Fields:

- kind (optional) — filter by type (image/audio/video/script).

- folder (optional) — restrict to a specific folder path.

- ListFilesResponse

```json
{
  "type": "ListFilesResponse",
  "files": [
    "/home/user/Music/track1.mp3",
    "/home/user/Music/track2.wav"
  ]
}
```

## Summary of Workflow

1. Client connects → sends `ClientHello`.

2. Workers connect → send `WorkerHello` with token.

3. Client requests folder scan → `ScanFoldersRequest`.

4. Server populates DB and delegates individual files to workers with `ScanFileRequest`.

5. Workers analyze files → return `FileScanned` (or `FileError`).

6. Client queries known types → `ListTypesRequest`.

7. Client queries known files → `ListFilesRequest`.
