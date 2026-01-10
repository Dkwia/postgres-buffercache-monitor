PostgreSQL Buffer Cache Monitor Extension
=====================================
## What the module does

- Runs as a **background worker** started via `shared_preload_libraries`
- Periodically iterates over all buffer descriptors (`NBuffers`)
- Reads buffer metadata (valid, dirty, pin count, usage count)
- Writes the collected information to a log file
- Adds a timestamp to each dump

Example output (simplified):
```
=== Buffer dump at 2026-01-07 08:30:00 ===
buf=0 valid=1 dirty=0 pin=0 usage=3
buf=1 valid=1 dirty=1 pin=1 usage=5
...
```

---

## How it is loaded

The module is loaded as a **preload library**, not via `CREATE EXTENSION`.

In `postgresql.conf`:

```conf
shared_preload_libraries = 'my_buffer_dump'
buffer_dump.interval = 5
```

---

## Build

This project uses a **manual build** (not PGXS), which avoids issues in
PostgreSQL dev branches.

Edit `Makefile` if needed, then run:

```bash
make
```

This produces:

```
my_buffer_dump.so
```

---

## Installation
Copy the library to PostgreSQL’s library directory:
```bash
cp my_buffer_dump.so /home/bo/opt/pgsql/lib/
```

Verify that the magic block exists:
```bash
nm my_buffer_dump.so | grep magic
```

Expected output:
```
Pg_magic_func
```

---

## Runtime behavior

* The worker starts automatically when PostgreSQL starts
* It runs continuously in the background
* Output file (by default):

```
/tmp/buffer_states.log
```

---
