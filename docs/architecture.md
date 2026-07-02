# Architecture diagrams

Mermaid and PlantUML diagrams for the BR ESW relay controller solution.

Full solution write-up (overview, assumptions, timing, demo, build):
[BR_ESW_Relay_Controller_Solution.txt](BR_ESW_Relay_Controller_Solution.txt)

**PlantUML sources:** [docs/plantuml/](plantuml/) — render with
[PlantUML](https://plantuml.com/), VS Code PlantUML extension, or
`java -jar plantuml.jar docs/plantuml/*.puml`.

---

## Module decomposition

Three software layers: application injects configuration and drives the periodic
task; `relay_control` owns the global FSM and one reusable `RelayInstance` per
relay; `relay_io` provides DPO/DI access. Exactly one HAL implementation
(`relay_io_hw.c` or `relay_io_sim.c`) is linked at build time.

<details>
<summary>Mermaid</summary>

```mermaid
flowchart TB
    subgraph App["Application (main.c)"]
        SCH[scheduler]
        CFG[config table<br/>kRelayConfig]
    end

    subgraph Control["relay_control"]
        RH[relay_controller<br/>global FSM]
        RI[relay_instance x N<br/>reusable component]
    end

    subgraph HAL["relay_io (HAL)"]
        IF[relay_io.h<br/>function interface]
        HW[relay_io_hw.c<br/>real target]
        SIM[relay_io_sim.c<br/>simulated plant]
    end

    CFG -->|injected at Init| RH
    SCH -->|every 5 ms| RH
    RH --> RI
    RI --> IF
    IF -.linked impl: target.-> HW
    IF -.linked impl: host.-> SIM
```

</details>

<details>
<summary>PlantUML — <code>plantuml/module_decomposition.puml</code></summary>

Source: [module_decomposition.puml](plantuml/module_decomposition.puml)

</details>

| Layer | Module | Responsibility |
|-------|--------|----------------|
| Application | `main.c`, `scheduler` | Config table, `RelayIo_Init`, schedule, generate requests |
| Controller | `relay_controller` | Global state machine, error reaction |
| Component | `relay_instance` | Per-relay command + supervision |
| HAL | `relay_io` | DPO/DI access; one impl linked (hw or sim) |

---

## Scheduling and timing

Each call to `Scheduler_RunTask()` represents **one 5 ms task period**: it invokes
`RelayController_Process()` once and advances the tick counter. One task period
equals one execution of the per-relay flowchart below.

On the **host demo**, `main.c` calls `Scheduler_RunTask()` inside a loop as fast
as the CPU allows. Ticks are **logical** time steps (tick 0, 1, 2, …), where
each tick stands for 5 ms — not wall-clock delay.

On the **target MCU**, the same function would be called from a **hardware timer
interrupt** or a **periodic RTOS task** (e.g. every 5 ms). The scheduler module
is a cyclic executive; the RTOS or timer provides real-time triggering.

Timing supervision (30 ms fault window, bounce filtering) uses **cycle counters**
derived from `kRelayTaskPeriodMs` in `common.h` — no runtime millisecond timers
are required inside the controller.

---

## Controller state diagram

Global controller FSM. Faults latch the controller into **ERROR**. The only
recovery path is **Re-Init** — a new call to `RelayController_Init()`, which
resets controller state to **NORMAL** and re-initialises all instances.

<details>
<summary>Mermaid</summary>

```mermaid
stateDiagram-v2
    direction LR

    [*] --> NORMAL : Init

    NORMAL --> ERROR : fault latched
    ERROR --> NORMAL : Re-Init

    note right of NORMAL
        Each Process() cycle:
        - handle OPEN/CLOSE requests
        - supervise feedback
    end note

    note right of ERROR
        Each Process() cycle:
        - force all relays OPEN
        - block CLOSE requests
        Exit only via Re-Init
    end note
```

</details>

<details>
<summary>PlantUML — <code>plantuml/controller_state.puml</code></summary>

Open [controller_state.puml](plantuml/controller_state.puml).

</details>

---

## Per-relay execution flow

One task cycle inside `RelayController_Process()`. When the controller is in
**ERROR**, every relay receives an effective **OPEN** command regardless of the
requested state. Fault detection compares the **applied** command against
sampled feedback every cycle; six **consecutive** mismatch cycles (30 ms) are
required before latching. Any matching sample clears the mismatch run, so
alternating bounce (wrong, right, wrong, right, …) cannot accumulate into a
false fault.

<details>
<summary>Mermaid</summary>

```mermaid
flowchart TD
    A[Start cycle] --> B{Controller in ERROR?}
    B -->|Yes| C[Force effective request = OPEN]
    B -->|No| D[Use requested command]
    C --> E{Command changed?}
    D --> E
    E -->|Yes| F[Compute DPO from NO/NC type\nWrite DPO via HAL\nReset mismatch window]
    E -->|No| G[Read DI feedback]
    F --> G
    G --> H[Convert DI to contact state]
    H --> I{Feedback matches applied command?}
    I -->|Yes| J[Clear consecutive mismatch counter]
    I -->|No| K[Increment consecutive mismatch counter]
    K --> L{Mismatch >= 30 ms?}
    L -->|Yes, cmd OPEN| M[Latch fault: WELDED]
    L -->|Yes, cmd CLOSE| N[Latch fault: CONSTANTLY_OPEN]
    L -->|No| O[Wait next cycle]
    M --> P[Controller enters ERROR]
    N --> P
    J --> Q[End cycle]
    O --> Q
```

</details>

<details>
<summary>PlantUML — <code>plantuml/per_relay_flow.puml</code></summary>

Open [per_relay_flow.puml](plantuml/per_relay_flow.puml).

</details>
