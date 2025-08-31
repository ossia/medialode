# Database Schema

The server (`libmgr-server`) maintains a SQLite database.

## Pragmas
- WAL mode enabled (`PRAGMA journal_mode = WAL`)
- `synchronous = NORMAL`
- Foreign keys enabled

---

## Why SQLite?

- Lightweight, portable, easy to debug.
- Safe concurrent writes with WAL mode.
- No external dependencies.

---

## Tables

### `folders`
Tracks watched folders.
- `path TEXT PRIMARY KEY`
- `last_scan INTEGER`

### `files`
All discovered files.
- `path TEXT PRIMARY KEY`
- `folder_path TEXT`
- `mtime INTEGER`
- `kind TEXT` (`image`, `audio`, `video`, `script`)

**Notes on `kind`:**
- Normally set by **workers** (e.g. `"image"`, `"audio"`, `"video"`, `"script"`).
- If missing, the server **infers** the type from metadata fields:
  - `width/height/channels` → image
  - `duration/sample_rate` → audio
  - `duration/codec/width` → video
  - `language/syntax_ok` → script

### `image_metadata`
- `path TEXT PRIMARY KEY`
- `width, height, channels`

### `audio_metadata`
- `path TEXT PRIMARY KEY`
- `duration, sample_rate, channels, bitrate, format`

### `video_metadata`
- `path TEXT PRIMARY KEY`
- `duration, width, height, codec, framerate`

### `script_metadata`
- `path TEXT PRIMARY KEY`
- `language, syntax_ok, error_message`

---

## Relationships

- Metadata tables reference `files(path)` (ON DELETE CASCADE).
- Deleting a file entry automatically removes its metadata.

---

## Workers

- Each worker specializes in one type of file:
  `image-worker`, `audio-worker`, `video-worker`, `script-worker`.
- Workers connect to the server via WebSocket and handle `ScanFileRequest`.
- Workers extract metadata and return it as JSON.
- This separation keeps the server simple and allows workers to crash/restart independently.

---

## Workflow

1. **ScanFoldersRequest** populates `files` and `folders`.
2. **ScanFileRequest** is delegated to the appropriate worker (image, audio, video, script).
   - The server always upserts into the `files` table first.
   - Based on `kind`, it then upserts into the relevant metadata table (`*_metadata`).
   - Finally, `files.kind` is updated.
3. CLI or API queries the DB via **ListFilesRequest** and similar messages.

