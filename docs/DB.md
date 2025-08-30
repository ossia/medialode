# Database Schema

The server (`libmgr-server`) maintains a SQLite database.

## Pragmas
- WAL mode enabled (`PRAGMA journal_mode = WAL`)
- `synchronous = NORMAL`
- Foreign keys enabled

## Tables

### folders
| Column    | Type    | Notes                    |
|-----------|---------|--------------------------|
| path      | TEXT PK | Absolute path to folder |
| last_scan | INTEGER | Epoch seconds            |

### files
| Column      | Type    | Notes                                    |
|-------------|---------|------------------------------------------|
| path        | TEXT PK | Absolute file path                       |
| folder_path | TEXT    | Parent folder                            |
| mtime       | INTEGER | Modification time (epoch seconds)        |

Indexes:
- `CREATE INDEX idx_files_folder ON files(folder_path);`
