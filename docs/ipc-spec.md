# IPC Protocol Specification

## Overview

The media library uses **JSON-over-WebSocket** for communication between clients and the server.
All messages are sent as JSON objects with the following structure:

`{  "type":  "<MessageType>",  "data":  { ... }  }`

-   **type**: String identifying the message type.

-   **data**: Object containing message-specific payload.


----------

## Message Types

### 1. **PingRequest**

**Purpose:** Check if the server is alive.

**Direction:** Client → Server

**Example:**

`{  "type":  "PingRequest",  "data":  {  "message":  "ping"  }  }`

----------

### 2. **PingResponse**

**Purpose:** Response to `PingRequest`.

**Direction:** Server → Client

**Example:**

`{  "type":  "PingResponse",  "data":  {  "message":  "pong"  }  }`

----------

### 3. **ScanFoldersRequest**

**Purpose:** Request the server to scan specific folders for media files.

**Direction:** Client → Server

**Fields:**

-   `folders`: Array of strings (absolute paths to scan).


**Example:**

`{  "type":  "ScanFoldersRequest",  "data":  {  "folders":  [  "/home/user/Music",  "/home/user/Videos"  ]  }  }`

----------

### 4. **ScanFoldersResponse**

**Purpose:** Acknowledgement that the scan has started.

**Direction:** Server → Client

**Fields:**

-   `status`: String, one of:

    -   `"ok"` (accepted)

    -   `"error"` (invalid request)


**Example:**

`{  "type":  "ScanFoldersResponse",  "data":  {  "status":  "ok"  }  }`

----------

### 5. **ScanProgressEvent**

**Purpose:** Notify the client about scan progress.

**Direction:** Server → Client

**Fields:**

-   `folder`: String (the folder currently being scanned).

-   `progress`: Integer (percentage, 0–100).


**Example:**

`{  "type":  "ScanProgressEvent",  "data":  {  "folder":  "/home/user/Music",  "progress":  42  }  }`

----------

## Connection Lifecycle

1.  **Client connects** to `ws://<host>:<port>`.

2.  **Server sends a greeting** (optional, currently skipped).

3.  Client sends **PingRequest** or **ScanFoldersRequest**.

4.  Server responds with corresponding message (`PingResponse`, `ScanFoldersResponse`) or emits **progress events**.

5.  **Either party can close the connection** at any time.

----------

## Error Handling

-   If a request is invalid, the server replies with:

`{  "type":  "ErrorResponse",  "data":  {  "code":  "<ErrorCode>",  "message":  "<Error description>"  }  }`

Example:

`{  "type":  "ErrorResponse",  "data":  {  "code":  "INVALID_PAYLOAD",  "message":  "Missing 'folders' field in ScanFoldersRequest"  }  }`
