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

## Workflow

1. **ScanFoldersRequest** populates `files` and `folders`.
2. **ScanFileRequest** → workers → metadata tables.
3. CLI queries DB via **ListFilesRequest**.

