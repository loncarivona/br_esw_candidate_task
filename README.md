# BR ESW Relay Controller

Relay controller solution in C for up to 10 configurable relay instances.

## Overview

Each relay is driven via a DPO output and supervised through a DI feedback input.
A periodic 5 ms task applies open/close requests, compares feedback against the
applied command, and latches faults when a mismatch persists for more than 30 ms:

- **WELDED** — command OPEN, feedback still closed
- **CONSTANTLY_OPEN** — command CLOSE, feedback still open

Any fault moves the controller into **ERROR**: all relays are forced open, close
requests are blocked, and recovery requires re-initialization.

The host demo runs two scenarios back-to-back (WELDED on relay 0, then
CONSTANTLY_OPEN on relay 1, with controller re-init between them).

## Prerequisites

- Demo: `gcc`, `make`
- Unit tests: additionally `cmake`, `g++`, and network access (first test run
  downloads Google Test)

## Build and run

Two demo builds:

| Target | Output | Logging |
|--------|--------|---------|
| `make prod` | `build/prod/relay_controller_demo` | off (no stdio, zero log overhead) |
| `make debug` | `build/debug/relay_controller_demo` | on (`RELAY_LOG_*` via stdio on host) |

`make` defaults to **prod**. Unit tests always use the prod library (no logging).

Production / target-style build:

```bash
make prod
./build/prod/relay_controller_demo
```

Host demo with tick-by-tick trace output:

```bash
make debug
./build/debug/relay_controller_demo
```

Or build and run in one step:

```bash
make run-prod
make run-debug
```

`make run` is an alias for `make run-debug`.

## Unit tests

Google Test unit tests

```bash
make test
```

The first run downloads Google Test into `third_party/googletest/` (gitignored).
Requires `cmake`, `g++`, and network access.

Or with CMake directly:

```bash
bash scripts/fetch_googletest.sh
cmake -S . -B build/test -DBUILD_DEMO=OFF
cmake --build build/test
ctest --test-dir build/test --output-on-failure
```

## Documentation

- Write-up: `docs/BR_ESW_Relay_Controller_Solution.txt`
- Diagrams: `docs/architecture.md` (Mermaid + PlantUML in `docs/plantuml/`)

## Layout

| Topic | Location |
|-------|----------|
| Design, assumptions, exam mapping | `docs/BR_ESW_Relay_Controller_Solution.txt` |
| Module / state / flow diagrams | `docs/architecture.md`, `docs/plantuml/` |
| Application logic | `src/relay_control/`, `src/scheduler/`, `src/main.c` |
| HAL (simulated plant / target stub) | `src/relay_io/` |
| Unit tests | `test/` |

```
src/
  relay_io/        DPO/DI layer (sim or hw)
  relay_control/   relay_instance + relay_controller + relay_log.h
  scheduler/
  main.c
docs/
```

Relay table is in `main.c`. Call `RelayIo_Init()` before `RelayController_Init()`.

For target build, implement `relay_io_hw.c` and set `RELAY_IO_SRC` in the Makefile.
