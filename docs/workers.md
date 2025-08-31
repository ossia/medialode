# Workers

Workers extract type-specific metadata for files.
They connect to the server over WebSocket.

---

## Why Workers?

- **Crash isolation**: one worker crash doesn’t kill the system.
- **Extensibility**: workers can be in any language (C++, Python, etc.).
- **Specialization**: each worker handles only one type. (Work in Progress)

---

## Script Worker

- Scans `.py`, `.js`, etc.
- Validates syntax.
- Populates `script_metadata`.

---

## Image Worker

- Scans `.png`, `.jpg`, etc.
- Extracts width, height, channels.
- Populates `image_metadata`.

---

## Audio Worker

- Scans `.mp3`, `.wav`, etc.
- Extracts duration, sample rate, channels, bitrate.
- Populates `audio_metadata`.

---

## Video Worker
- Will scan `.mp4`, `.mkv`, etc.
- Extract duration, codec, resolution, framerate.
- Populate `video_metadata`.

---

## Running Workers

```bash
./build/app/libmgr-script-worker/libmgr-script-worker --host 127.0.0.1 --port 8081
```
```bash
./build/app/libmgr-audio-worker/libmgr-audio-worker --host 127.0.0.1 --port 8081
```
```bash
./build/app/libmgr-image-worker/libmgr-image-worker --host 127.0.0.1 --port 8081
```
```bash
./build/app/libmgr-video-worker/libmgr-video-worker --host 127.0.0.1 --port 8081
```
