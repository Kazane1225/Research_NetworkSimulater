// ── English locale ─────────────────────────────────────────────────────────
// All user-visible English strings. Add new keys here, then mirror in ja.js.

const LOCALE_EN = {

    algoInfo: [
        null,
        {
            title:      'Stabilizing Counting Algorithm',
            desc:       'Agents exchange vistas each round. The algorithm finds the first non-branching level and uses red-edge multiplicity ratios to propagate anonymity estimates from the leader. Early guesses may be wrong, but the output is guaranteed to stabilise on the correct count n by round 2n \u2212 2.',
            boundLabel: 'Stabilizes by round',
            bound:      n => n > 0 ? `2n \u2212 2 = ${2 * n - 2}` : '\u2014',
        },
        {
            title:      'Terminating Counting Algorithm',
            desc:       'A stronger variant: the algorithm halts only after a correctness certificate (guessers, heavy nodes, and verified isles/cuts). It never commits an incorrect answer. The cost is a later bound: termination by round 3n \u2212 3.',
            boundLabel: 'Terminates by round',
            bound:      n => n > 0 ? `3n \u2212 3 = ${3 * n - 3}` : '\u2014',
        },
    ],

    tutorial: [
        { title: 'Welcome to the Tour',   text: 'A quick walkthrough of the main UI areas. Press Next to continue, \u2190 Prev to go back, or \u2715 to close at any time.',                                                                                                                                                                                                                                                          sel: null           },
        { title: 'The Canvas',            text: 'The main area is split in two. Left half: the network \u2014 circles are agents, anonymous computing entities with no unique ID. Right half: the History Tree \u2014 records what each agent has observed round by round. Two agents are indistinguishable if and only if their history trees are identical.',                                                                          sel: '#canvas-wrap' },
        { title: 'Dynamic Rounds',        text: 'Edges can change every round \u2014 that\u2019s what makes the network dynamic. Use \u25b2 / \u25bc in the toolbar or the scroll wheel on the canvas to step through rounds and watch the topology evolve.',                                                                                                                                                                          sel: '#btn-prev-round' },
        { title: 'Counting Algorithms',   text: 'Activate Stabilizing or Terminating to run a counting algorithm. A banner appears below the toolbar with a description of the algorithm and its theoretical round bound.',                                                                                                                                                                                                            sel: '.algo-group'  },
        { title: 'Executing Steps',       text: 'Select an agent or history-tree node, then press \u25b6 Step (or Space) to run one algorithm step. History-tree nodes change colour as the algorithm deduces the network size.',                                                                                                                                                                                                      sel: '#btn-step'    },
        { title: 'Loading Networks',      text: 'Click Load to open an example from the networks/ folder. DefaultNetwork2.txt is a good starting point; BoldiVigna.txt is a well-known example from the research literature.',                                                                                                                                                                                                         sel: '#btn-load'    },
        { title: 'Network Stats',         text: 'The Network Stats panel (bottom-left) shows live information: agent count n, round count T, anonymity classes, and how many agents are uniquely identified so far.',                                                                                                                                                                                                                  sel: '#net-stats'   },
        { title: 'All Done!',             text: 'You\u2019re ready to explore! Open Glossary for key term definitions and Help (H) for keyboard shortcuts. Try loading different networks and comparing how both algorithms behave.',                                                                                                                                                                                                   sel: null           },
    ],

    guidedStab: [
        { title: 'Stabilizing vs Terminating',                    body: 'Learn focuses on what makes the two counting algorithms different. We start with Stabilizing: it may show provisional (even wrong) guesses, then settle on the correct n. It never announces \u201cdone\u201d. Tour covers the UI if you need a refresher.',                                                                 action: null,                                                        highlight: null            },
        { title: 'This Demo Network',                             body: 'Six agents, five dynamic rounds. L is the unique leader (input 0); the others start identical. Early on, some agents stay indistinguishable, so any count based on incomplete evidence can only be a guess.',                                                                                                               action: null,                                                        highlight: null            },
        { title: 'Enable Stabilizing',                            body: 'Click Stabilizing. The banner shows the guarantee: the output stabilises by round 2n \u2212 2 (here at most 10). Stabilising means the value stops changing \u2014 not that the algorithm halts.',                                                                                                                       action: 'Click the Stabilizing button',                              highlight: '.algo-group'   },
        { title: 'Select an Agent',                               body: 'Left-click any agent on the left. The algorithm runs on that agent\u2019s vista (its partial history tree).',                                                                                                                                                                                                        action: 'Left-click any agent in the left panel',                    highlight: null            },
        { title: 'Take One Step',                                 body: 'Press \u25b6 Step (or Space). The algorithm looks for a non-branching level and spreads anonymity estimates from the leader using red-edge ratios. Watch the History Tree colours change.',                                                                                                                             action: 'Press \u25b6 Step or Space',                                highlight: '#btn-step'     },
        { title: 'Provisional Guesses',                           body: 'L turns green (anonymity 1). Symmetric groups often start yellow \u2014 guesses, not proven facts. Red means an incorrect guess. Stabilizing is allowed to be wrong before it settles; that is the defining risk. Check the colour legend (bottom-right).',                                              action: null,                                                        highlight: '#node-legend'  },
        { title: 'Why Not Halt Here?',                            body: 'A vista can look \u201cfinished\u201d while still missing agents or links. Stopping as soon as a plausible n appears can lock in a wrong answer. Stabilizing therefore keeps going until the value stops changing \u2014 it has no halt certificate.',                                                                      action: null,                                                        highlight: null            },
        { title: 'Step Until It Settles',                         body: 'Keep pressing \u25b6 Step. When the History Tree root turns green and shows 6, the guess has stabilised on n = 6. The algorithm still does not halt; it simply will not change that answer later (within the bound).',                                                                                                 action: 'Keep pressing \u25b6 Step until the root shows 6 in green', highlight: '#btn-step'     },
        { title: 'Stabilized \u2014 No \u201cDone\u201d Signal', body: 'Stabilizing reached the correct count, but never proved it was safe to stop. Open the Terminating tab to see the stronger guarantee: halt only after a certificate, at the cost of a later round bound (3n \u2212 3).',                                                                                                   action: null,                                                        highlight: null            },
    ],

    guidedTerm: [
        { title: 'Terminating: Halt With a Proof',                body: 'Terminating only outputs n when evidence proves the count is correct, then stops. Unlike Stabilizing, it never commits a wrong answer. The trade-off: it may need more rounds (bound 3n \u2212 3 instead of 2n \u2212 2). Same 6-agent network.',                                                                          action: null,                                                        highlight: null            },
        { title: 'Enable Terminating',                            body: 'Click Terminating. The banner updates to the 3n \u2212 3 bound (at most 15 rounds for n = 6).',                                                                                                                                                                                                                     action: 'Click the Terminating button',                              highlight: '.algo-group'   },
        { title: 'Select an Agent',                               body: 'Left-click any agent. The algorithm searches that agent\u2019s vista for verifiable structure \u2014 not just a plausible number.',                                                                                                                                                                                  action: 'Left-click any agent',                                      highlight: null            },
        { title: 'Watch Uncommitted Colours',                     body: 'Press \u25b6 Step. Cyan marks an early, uncommitted guess; orange marks intermediate refinement. Nodes do not turn red: the algorithm refuses to lock in an incorrect answer. Match colours with the legend (bottom-right).',                                                                                          action: 'Press \u25b6 Step or Space',                                highlight: '#btn-step, #node-legend' },
        { title: 'Certificates, Not Just Guesses',                body: 'Terminating builds evidence step by step: guessers seed anonymity estimates, heavy nodes check consistency, and verified isles or cuts act as a certificate. Only then may the root turn green and halt.',                                                                                                           action: null,                                                        highlight: '#label-history' },
        { title: 'Step Until It Halts',                           body: 'Keep pressing \u25b6 Step. When the root turns green with 6, the algorithm has terminated: the answer is provably correct and will not change.',                                                                                                                                                                     action: 'Press \u25b6 Step until the root turns green with n = 6',  highlight: '#btn-step'     },
        { title: 'Compare the Two Guarantees',                    body: 'Stabilizing: may be wrong early; settles by 2n \u2212 2; no halt signal. Terminating: never commits a wrong answer; halts with a certificate; bound 3n \u2212 3. Same counting problem, different promises. Switch tabs anytime to compare.',                                                                            action: null,                                                        highlight: null            },
    ],

    commentary: {
        uniqueIdentified: (du, unique, n) => `+${du} agent${du > 1 ? 's' : ''} uniquely identified  (${unique} / ${n})`,
        classesChanged:   (from, to)       => `Anonymity classes: ${from} \u2192 ${to}`,
        rootGuessChanged: (was, now, ok)   => `Root guess: ${was} \u2192 ${now}${ok ? '  \u2713 correct!' : ''}`,
        noChange:         'No observable change \u2014 try selecting a different agent.',
    },

    ui: {
        // Toolbar labels
        tbRound:     'Round',
        tbAlgorithm: 'Algorithm',
        tbEdit:      'Edit',
        // Algo buttons
        btnNone:        'None',
        btnStabilizing: 'Stabilizing',
        btnTerminating: 'Terminating',
        // Icon buttons (text part only)
        btnStep:     'Step',
        btnAgent:    'Agent',
        btnDeselect: 'Deselect',
        btnLoad:     'Load',
        btnSave:     'Save',
        btnOptions:  'Options',
        btnGlossary: 'Glossary',
        btnTour:     'Tour',
        btnLearn:    'Learn',
        btnHelp:     'Help',
        // Options panel
        optOutdegree:   'Toggle outdegree awareness',
        optArrows:      'Toggle arrowheads',
        optRoundNodes:  'Toggle round / square nodes',
        optHighlight:   'Toggle current-level highlight',
        optRedEdge:     'Cycle red-edge draw mode',
        optGrid:        'Snap agents to grid',
        optCaps:        'Two-way links by default (Caps Lock)',
        optStudentMode: 'Student mode',
        // Canvas labels
        labelNetwork: 'Network',
        labelHistory: 'History Tree',
        // Node legend
        legendTitle:   'Node colours',
        legYellow:     'Not yet guessed',
        legGreen:      'Correctly guessed / counted',
        legStabRed:    'Incorrectly guessed',
        legTermCyan:   'Initial-level guess',
        legTermOrange: 'Intermediate guess',
        legTagStab:    '(Stabilizing)',
        legTagTerm:    '(Terminating)',
        // Network stats panel
        statsTitle:   'Network Stats',
        statsAgents:  'Agents',
        statsLeaders: 'Leaders',
        statsRounds:  'Rounds',
        statsLinks:   'Links (this round)',
        statsClasses: 'Anonymity classes',
        statsUnique:  'Uniquely identified',
        // Algorithm banner
        bannerHint:    'Select an agent or history-tree node, then press <kbd>Space</kbd> (or <strong>\u25b6 Step</strong>) to execute one step.',
        bannerUniq:    'Uniquely identified',
        bannerAgents:  'agents',
        bannerClasses: 'Anonymity classes',
        // Tutorial (Tour) panel
        tutPrev: '\u2190 Prev',
        tutNext: 'Next \u2192',
        tutDone: '\u2713 Done',
        // Guided (Learn) panel
        guidedBadge: '\ud83c\udf93 Learn',
        guidedSkip:  'Skip action \u2192',
        guidedNext:  'Next \u2192',
        guidedPrev:  '\u2190 Prev',
        guidedDone:  '\u2713 Done',
        // Misc messages
        wasmNotReady: 'WASM not yet ready \u2014 please wait a moment and try again.',
        // Help modal
        helpTitle: 'Anonymous Dynamic Networks \u2014 Controls',
        helpClose: '\u2715 Close',
        helpContent: `<h3>Mouse \u2014 Network Panel</h3>
<table>
<tr><td>Left-click + drag on agent</td><td>Draw a new directed link from that agent</td></tr>
<tr><td>Right-click + drag on agent</td><td>Move the agent</td></tr>
<tr><td>Right-click on empty area</td><td>Create a new agent</td></tr>
<tr><td>Left-click on agent / node</td><td>Select / deselect</td></tr>
<tr><td>Mouse wheel</td><td>Change current round</td></tr>
</table>
<h3>Modifiers while drawing or deleting a link</h3>
<table>
<tr><td>Ctrl  or  Q</td><td>Delete the link instead of creating it</td></tr>
<tr><td>Shift</td><td>Make the link two-way (bidirectional)</td></tr>
<tr><td>Alt</td><td>Apply the change to all rounds</td></tr>
<tr><td>Caps Lock</td><td>Toggle: two-way becomes the default direction</td></tr>
</table>
<h3>Navigation</h3>
<table>
<tr><td>\u2191 / \u2193</td><td>Previous / next round</td></tr>
<tr><td>\u2190 / \u2192</td><td>Previous / next anonymity class in current view</td></tr>
<tr><td>ESC</td><td>Deselect agents and clear view selection</td></tr>
</table>
<h3>Network Editing</h3>
<table>
<tr><td>Del  or  U</td><td>Delete selected agent(s)</td></tr>
<tr><td>+</td><td>Insert a new round after the current round</td></tr>
<tr><td>\u2212</td><td>Delete the current round</td></tr>
<tr><td>Backspace</td><td>Delete all links in the current round</td></tr>
<tr><td>0 \u2013 9</td><td>Set input of selected agent (0 = leader L)</td></tr>
<tr><td>L</td><td>Load network from file</td></tr>
<tr><td>S</td><td>Save network to file</td></tr>
<tr><td>G</td><td>Snap all agents to grid</td></tr>
</table>
<h3>Counting Algorithms  (TAB cycles, Space steps)</h3>
<table>
<tr><td>Tab</td><td>Cycle: None \u2192 Stabilizing \u2192 Terminating \u2192 None</td></tr>
<tr><td>Space</td><td>Execute one algorithm step (select an agent or history node first)</td></tr>
</table>
<table style="margin-top:6px">
<tr><td style="color:#ffff00">Yellow node</td><td>Not yet guessed</td></tr>
<tr><td style="color:#80ff80">Green node</td><td>Correctly guessed / counted</td></tr>
<tr><td style="color:#ff8080">Red node</td><td>Incorrectly guessed (Stabilizing)</td></tr>
<tr><td style="color:#00cfcf">Cyan node</td><td>Initial-level guess (Terminating)</td></tr>
<tr><td style="color:#ffc040">Orange node</td><td>Intermediate guess (Terminating)</td></tr>
</table>
<h3>Display Options</h3>
<table>
<tr><td>O</td><td>Toggle outdegree awareness of agents</td></tr>
<tr><td>A</td><td>Toggle arrowheads on edges</td></tr>
<tr><td>D</td><td>Cycle red-edge draw mode: all \u2192 selected view \u2192 hidden</td></tr>
<tr><td>B</td><td>Toggle round / square nodes</td></tr>
<tr><td>C</td><td>Toggle grey highlight bar on current level</td></tr>
<tr><td>H</td><td>Show this reference</td></tr>
</table>
<div id="help-ref">Theoretical background: <a href="https://arxiv.org/abs/2404.02673" target="_blank" rel="noopener">arxiv.org/abs/2404.02673</a></div>`,
        // Glossary modal
        glossaryTitle: 'Anonymous Dynamic Networks \u2014 Glossary',
        glossaryClose: '\u2715 Close',
        glossaryContent: `<div class="gl-entry"><div class="gl-term">Agent</div><div class="gl-def">A computing entity in the network. Agents have no unique identifiers \u2014 they are <em>anonymous</em>. Each agent starts with an integer <em>input</em> value (0 = leader).</div></div>
<div class="gl-entry"><div class="gl-term">Leader</div><div class="gl-def">A special agent with input 0, shown as <strong>L</strong> in the simulator. Counting algorithms use the leader as a reference point to determine the total number of agents <em>n</em>.</div></div>
<div class="gl-entry"><div class="gl-term">Dynamic Network</div><div class="gl-def">A network whose communication links can change every round. Unlike static networks, edges are not permanent \u2014 they may be controlled by an adversary. The key challenge is that agents cannot rely on a fixed topology.</div></div>
<div class="gl-entry"><div class="gl-term">Round</div><div class="gl-def">One time step of communication. In each round a set of directed links is active and agents exchange messages only along those links. Use <strong>\u2191 / \u2193</strong> or the scroll wheel to browse rounds in the simulator.</div></div>
<div class="gl-entry"><div class="gl-term">Interaction (Link)</div><div class="gl-def">A directed edge from agent A to agent B in a given round: A sends a message to B. The <em>multiplicity</em> counts how many parallel copies of the message are sent, which matters for the counting algorithms.</div></div>
<div class="gl-entry"><div class="gl-term">History Tree</div><div class="gl-def">A tree that records everything an agent has observed so far. The root is the initial state; each level adds one more round of observations. Two agents are <em>indistinguishable</em> if and only if they share the same history tree. The right panel of the simulator visualises this tree.</div></div>
<div class="gl-entry"><div class="gl-term">Anonymity Class</div><div class="gl-def">A group of agents with <em>identical</em> history trees \u2014 they cannot tell each other apart. The size of the class is its <em>anonymity</em>. An anonymity of 1 means the agent is uniquely identifiable by its history.</div></div>
<div class="gl-entry"><div class="gl-term">Counting Problem</div><div class="gl-def">The task of determining the total number of agents <em>n</em>. Because agents are anonymous they cannot simply count themselves \u2014 they must deduce <em>n</em> from the patterns of messages received over multiple rounds.</div></div>
<div class="gl-entry"><div class="gl-term">Stabilizing Algorithm</div><div class="gl-def">Eventually outputs the correct answer and <em>never changes it again</em>, but may output wrong values before it settles. It has no halt signal. Guaranteed to stabilise by round 2n \u2212 2. Select <strong>Stabilizing</strong>.</div></div>
<div class="gl-entry"><div class="gl-term">Terminating Algorithm</div><div class="gl-def">Outputs the correct answer and then <em>halts</em> with a correctness certificate. Stronger than stabilizing: once it stops, the answer is proven correct. Bound 3n \u2212 3. Select <strong>Terminating</strong>.</div></div>
<div class="gl-entry"><div class="gl-term">Outdegree Awareness</div><div class="gl-def">An optional capability (toggle with <strong>O</strong>) where each agent also knows how many messages it <em>sent</em> in the previous round. This extra information can help the algorithm make faster or more accurate guesses.</div></div>
<div class="gl-entry"><div class="gl-term">Non-Branching Level</div><div class="gl-def">A History Tree level where every visible node has exactly one child. On such a level, red-edge multiplicity ratios equal anonymity ratios, so the stabilizing algorithm can propagate estimates from the leader.</div></div>
<div class="gl-entry"><div class="gl-term">Isle / Cut (Terminating)</div><div class="gl-def">Verified structure in a vista used as a correctness certificate. Terminating builds guesses via guessers and heavy nodes, then commits only after an isle or cut confirms the count.</div></div>
<div id="glossary-ref">Theoretical background: <a href="https://arxiv.org/abs/2404.02673" target="_blank" rel="noopener">arxiv.org/abs/2404.02673</a></div>`,
    },
};
