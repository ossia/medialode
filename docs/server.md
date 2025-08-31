# Server (`libmgr-server`)

---

## What is the Server?

The **server** is the central coordinator:
- Hosts a WebSocket endpoint for clients & workers.
- Manages SQLite database persistence.
- Dispatches scan requests to workers.
- Routes responses back to clients.

---

## Why a Central Server?

- Keeps DB consistent (workers never write directly).
- Allows multiple clients/workers simultaneously.
- Clean separation of responsibilities.

---

## How it Works

1. **Startup**
   - Opens SQLite DB.
   - Creates necessary tables.
   - Listens on WebSocket (default `localhost:8081`).

2. **Connections**
   - Workers send `WorkerHello`.
   - Clients send `ClientHello`.

3. **Requests**
   - Client: `ScanFoldersRequest` → server scans filesystem → DB.
   - Client: `ScanFileRequest` → forwarded to worker → worker returns `FileScanned` → DB updated.
   - Client: `ListFilesRequest` → query DB → return file list.

4. **Keep-alive**
   - Server pings clients/workers every 15s.
   - Closes idle sockets.

---

## Running the Server

```bash
./build/app/libmgr-server
```

