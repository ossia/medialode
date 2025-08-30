# IPC Protocol Specification

This document describes the JSON-over-WebSocket messages exchanged between:

- **libmgr-server** (central database + orchestrator)
- **libmgr-scan-worker** (scanner processes)
- **CLI clients** (e.g. `scan-folders-cli`)

All messages are JSON objects with a required `"type"` field.

---

## Ping

- **PingRequest**

```json
{ "type": "PingRequest" }
```

- **PingResponse**
```json
{ "type": "PingResponse", "message": "pong" }
```

## Worker Identity

- **WorkerHello**

Sent by workers on connect.

```json
{
  "type": "WorkerHello",
  "role": "scan-worker",
  "token": "changeme",
  "name": "worker-1"
}
```

## Fields:

- role — currently "scan-worker".

- token — must match server’s LIBMGR_WORKER_TOKEN.

- name — worker identifier (for logs).

## ClientHello

Sent optionally by CLI clients.

```json
{
  "type": "ClientHello",
  "token": ""
}
```

## Folder Scan

- **ScanFoldersRequest**

Sent by clients.

```json
{
  "type": "ScanFoldersRequest",
  "paths": ["/tmp", "/home/user/Music"]
}
```

- **ScanFoldersResponse**

Reply from server after inserting into DB.

```json
{
  "type": "ScanFoldersResponse",
  "scanned_folders": 123,
  "scanned_files": 4567
}
```

## File Scan

- **ScanFileRequest**

Sent by server → worker.

```json
{
  "type": "ScanFileRequest",
  "request_id": "abcd123",
  "path": "/tmp/example.wav"
}
```
- **FileScanned**

Reply from worker.

```json
{
  "type": "FileScanned",
  "request_id": "abcd123",
  "path": "/tmp/example.wav",
  "size": 102400,
  "mtime": 1717776000
}
```

- **FileError**

Reply from worker on error.

```json
{
  "type": "FileError",
  "request_id": "abcd123",
  "path": "/tmp/missing.txt",
  "error": "file not found"
}
```
