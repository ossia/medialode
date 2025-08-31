# Roadmap

## Completed

1.	Initialize repository & CI
    - Set up the medialode repo
    -	Confirm CMake + CI config (Linux/macOS/Windows builds)
    -	Verify “hello world” build across all OSes
2.	Vendor/Fetch Boost and other dependencies
3.	Define IPC protocol spec
	-	Draft JSON-over-WebSocket messages for DB queries & scanner commands
	-	Add a Markdown “IPC API” doc under docs/
4.	SQLite server skeleton
	-	Create libmgr-server binary target
	-	Open a UNIX/websocket listener (Boost.Beast) on startup
	-	Respond to a ping RPC with { "status": "ok" }
5.	Folder-scan orchestration
	-	Add “scan-folders” RPC: takes list of paths, stores in DB table
	-	Persist scan status in a folders table (path, last_scan)
6.	WAL mode & basic schema
	-	Enable PRAGMA journal_mode = WAL on DB open
	-	Define tables
7.	Generic scanner process framework
	-	Make a libmgr-scan-worker binary that:
	-	Connects to server via WebSocket
	-	Listens for scan-file commands
	-	Returns file-scanned with metadata or file-error
8.	Image scanner worker
	-	On scan-file, if extension belongs to {png,jpg,…}, extract dimensions
9.	Audio scanner worker
	-	For audio files: extract duration, sample rate
10.	Video scanner worker
	-	For video files: extract duration, resolution
11.	Language/script scanner worker
	-	For .js, .glsl, etc: attempt a dry-run parse to catch syntax errors
	-	Report syntax OK / error message
CLI client
12.	Basic CLI scaffolding
	-	Create libmgr-cli binary with subcommands
13.	Implement list types
	-	libmgr-cli list types → prints supported media types
14.	Implement list files
	-	libmgr-cli list files --type audio --folder /path
15.	Graceful shutdown & persistence
	-	Ensure server on SIGINT cleanly stops workers + commits DB
16.	Documentation & examples
	-	Create a README with quickstarts for server & CLI

## Next Steps
17.	Plugin scanner worker
	-	Wrap existing vstpuppet logic for VST2/VST3
	-	Extract param counts, GUI screenshot, plugin metadata
18.	Worker supervision & crash handling
	-	Server must spawn one worker per media type
	-	Monitor child process exit; on crash mark file as bad and respawn worker
19.	Work queue & batching
	-	Implement a Redis-like queue in SQLite:
	-	CREATE TABLE queue (file_id, worker_type, status);
	-	Server enqueues new/changed files; workers pull next item
20.	Implement search tags
	-	libmgr-cli search --tag livecoding
21.	Real-time folder watching
	-	Move from manual scan-folders to a persistent watch mode
	-	On new FS events enqueue new scan jobs
22.	Caching & lazy-load previews
    -   Capture previews and thumbnails for various media files and store them in a previews DB
	-	Don’t regenerate previews if thumbnail exists and file unchanged
24. Update the documentation and README accordingly
