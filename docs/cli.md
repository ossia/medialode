# CLI (`libmgr-cli`)

The CLI provides a simple way to interact with the server.

---

## Why a CLI?

- Developers can test functionality quickly.
- Serves as a reference client implementation.
- Enables scripting & automation.

---

## Commands

### List supported types

```bash
./build/app/libmgr-cli/libmgr-cli list types
```

### Output

```
Available types:
  - image
  - audio
  - video
  - script
```

### List Files from DB

```bash
./build/app/libmgr-cli/libmgr-cli list files --type <image/audio/video/script> --folder <path>
```

### Scan a directory first and then list the files

```bash
./build/app/libmgr-cli/libmgr-cli list files --type <image/audio/video/script> --folder <path> --scan
```

### Options:

`--type <kind>`: filter by kind (image, audio, etc.)

`--folder <path>`: folder to query

`--scan`: request rescan before listing
