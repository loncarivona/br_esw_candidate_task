# BR ESW Relay Controller

Relay controller solution in C.

## Build

```bash
make
./relay_controller_demo
```

## Documentation

- Write-up: `docs/BR_ESW_Relay_Controller_Solution.txt`
- Diagrams: `docs/architecture.md`

Demo runs WELDED then CONSTANTLY_OPEN fault scenarios (re-init between them).

## Layout

```
src/
  relay_io/        DPO/DI layer (sim or hw)
  relay_control/   relay_instance + relay_controller
  scheduler/
  main.c
docs/
```

Relay table is in `main.c`. Call `RelayIo_Init()` before `RelayController_Init()`.

For target build, implement `relay_io_hw.c` and set `RELAY_IO_SRC` in the Makefile.
