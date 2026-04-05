# Usage Guide — Anonymous Dynamic Networks Simulator

This document explains how to use the simulator after it has been built and launched.
For build and launch instructions see [README.md](README.md).

---

## Interface Overview

The simulator window is divided into two panels separated by a draggable vertical divider:

| Panel | Description |
|-------|-------------|
| **Left — Network panel** | Displays the dynamic network graph. Agents are shown as nodes; interactions (directed edges) for the current round are shown as red arrows. |
| **Right — History tree panel** | Displays the history tree (view) of all agents. Each level corresponds to a round. Nodes are colour-coded according to the active counting algorithm. |

Drag the vertical divider left or right to resize the panels.

---

## Getting Started

1. Launch the app (`serve.bat` or `build.bat`) and open **http://localhost:8000/** in your browser.
2. A default network is loaded automatically.
3. Use the **Up / Down arrow keys** (or mouse wheel) to step through rounds.
4. Press **TAB** to choose a counting algorithm; press **SPACE** to run it step by step.
5. Press **H** at any time to display the built-in command reference overlay.

---

## Mouse Controls

### General
| Action | Effect |
|--------|--------|
| Left-click on agent or history node | Select / deselect it |
| Left-click away from everything | Deselect current selection |
| Mouse wheel up | Go to previous round |
| Mouse wheel down | Go to next round |

### Network Panel
| Action | Effect |
|--------|--------|
| Left-click on agent + drag | Draw a new directed link from that agent |
| Right-click on agent + drag | Move the agent |
| Right-click away from agents | Create a new agent at that position |

#### Modifiers while drawing / deleting a link
| Key held | Effect |
|----------|--------|
| **Ctrl** or **Q** | Delete the link instead of creating it |
| **Shift** | Make the link two-way (bidirectional) |
| **Alt** | Apply the change to **all** rounds, not just the current one |
| **Caps Lock** (toggle) | Reverse the default direction: two-way becomes the default; Shift makes it one-way |

### History Tree Panel
| Action | Effect |
|--------|--------|
| Left-click on a node | Select / highlight the anonymity class it belongs to |

---

## Keyboard Shortcuts

### Navigation
| Key | Action |
|-----|--------|
| `↑` / `↓` | Go to previous / next round |
| `←` / `→` | Select previous / next anonymity class in current view |
| `ESC` | Deselect agents and clear view selection |

### Network Editing
| Key | Action |
|-----|--------|
| `DEL` or `U` | Delete the selected agent(s) |
| `+` | Insert a new round after the current round |
| `-` | Delete the current round |
| `Backspace` | Delete **all links** in the current round |
| `0`–`9` | Set the **input** of the selected agent (0 = leader `L`) |
| `O` | Toggle outdegree awareness of agents |
| `G` | Snap all agents to the grid |
| `L` | Load a network from a file |
| `S` | Save the current network to a file |

### Display Options
| Key | Action |
|-----|--------|
| `A` | Toggle arrowheads on edges |
| `D` | Cycle red-edge / outdegree drawing mode (all → selected view only → hidden) |
| `B` | Toggle node shape between square and round |
| `C` | Toggle grey highlight bar on the current level in the history tree |

### Algorithms
| Key | Action |
|-----|--------|
| `TAB` | Cycle through counting algorithms: **None → Stabilizing → Terminating → None** |
| `Space` | Execute one step of the active algorithm (requires an agent or history node to be selected) |

### Other
| Key | Action |
|-----|--------|
| `H` | Show / hide the command reference overlay |

---

## Counting Algorithms

Press **TAB** to switch between algorithms. The active mode is shown in a brief on-screen message.

### No algorithm (default)
The history tree is drawn without any algorithm colouring.

### Stabilizing counting algorithm
Each visible history-tree node is highlighted:
- **Yellow** — not yet guessed
- **Green** — guessed correctly (guess equals true anonymity)
- **Red** — guessed incorrectly

### Terminating counting algorithm
Additional colour states:
- **Yellow** — not yet guessed
- **Cyan** — initial-level node with a guess
- **Orange** — guessed (intermediate)
- **Green** — counted (final correct answer)

#### Step-by-step execution
With an algorithm active and an agent (or history node) selected, press **Space** repeatedly to advance the algorithm one step at a time. A message shows the current step number.

---

## Agent Inputs

Each agent has an integer input value visible inside its node:

| Label | Meaning |
|-------|---------|
| `L` | Leader (input = 0); leaders are always sorted to the front |
| `1`, `2`, … | Non-leader agent with the given input value |

Press a digit key **0–9** while an agent is selected to change its input. Input `0` marks the agent as a leader.

---

## Loading and Saving Networks

### Save
Press **S** to save the current network. The file will be named `Network.txt`.

### Load
Press **L** to open a file picker and load a `.txt` network file.

### File Format
Network files are plain text with the following structure:

```
entity (<input>, <x>, <y>)
...
round <n>
inter (<from>, <to>, <multiplicity>)
...
```

- `entity` lines define agents: **input** (0 = leader), **x/y** position in world coordinates.
- `round` headers group interactions by round number (1-based).
- `inter` lines define a directed interaction: **from** agent index, **to** agent index, **multiplicity**.

The `networks/` directory contains many ready-made example files that can be loaded directly.

---

## Built-in Example Networks

The `networks/` folder includes a variety of example topologies. Notable ones:

| File | Description |
|------|-------------|
| `TerminatingWorstCase.txt` | Worst-case scenario for the terminating counting algorithm |
| `LowerBound2n.txt` | Lower-bound construction requiring ~2n rounds |
| `LowerBound3n2.txt` | Lower-bound construction requiring ~3n/2 rounds |
| `BoldiVigna.txt` | Boldi–Vigna bipartite anonymous network |
| `Circus.txt` | Circus-style topology |
| `Sunflower.txt` | Sunflower topology |
| `StabilizingCounterexample*.txt` | Counterexamples to naive stabilising approaches |

---

## Reference

For the theoretical background, see:  
<https://arxiv.org/abs/2404.02673>
