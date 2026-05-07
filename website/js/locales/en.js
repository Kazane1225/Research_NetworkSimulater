// ── English locale ─────────────────────────────────────────────────────────
// All user-visible English strings. Add new keys here, then mirror in ja.js.

const LOCALE_EN = {

    algoInfo: [
        null,
        {
            title:      'Stabilizing Counting Algorithm',
            desc:       'Agents broadcast their history trees each round. The algorithm finds \u2018exposed pairs\u2019 \u2014 two agents that observed each other \u2014 and propagates anonymity estimates from the leader outward. It may output wrong guesses at first, but is guaranteed to stabilise on the correct count n by round 2n \u2212 2.',
            boundLabel: 'Stabilizes by round',
            bound:      n => n > 0 ? `2n \u2212 2 = ${2 * n - 2}` : '\u2014',
        },
        {
            title:      'Terminating Counting Algorithm',
            desc:       'A stronger variant: the algorithm explicitly halts and signals \u201cdone\u201d when it is certain the count is correct. It builds complete \u2018isles\u2019 of counted agents before announcing the result, so no incorrect output is ever committed to.',
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
        { title: 'Welcome \u2014 Let\u2019s Count!',              body: 'We\u2019ve loaded a small example network: 6 agents across 5 rounds. Your goal is to understand how the Stabilizing counting algorithm works \u2014 all without knowing any agent\u2019s identity.',                                                                                                                     action: null,                                                        highlight: null            },
        { title: 'Agents are Anonymous',                          body: 'The circles in the left panel are agents. They have no names or IDs. The only exception is the L (leader), which has a special starting input of 0. All other agents start with input 1 and are completely identical at first.',                                                                                        action: null,                                                        highlight: null            },
        { title: 'Round 1: A Ring',                               body: 'The network starts as a ring: L \u2014 1 \u2014 2 \u2014 3 \u2014 4 \u2014 5 \u2014 L. Agents 1 & 5 are symmetric (both next to L), and agents 2, 3, 4 are also symmetric. The algorithm will have to resolve this ambiguity!',                                                                                           action: null,                                                        highlight: null            },
        { title: 'Advance to Round 2',                            body: 'Press \u2193 or scroll the mouse wheel on the canvas to go to Round 2. Notice which agents the leader L is now connected to \u2014 the topology is already different!',                                                                                                                                                action: 'Press \u2193 or scroll to advance',                         highlight: '#btn-next-round' },
        { title: 'The Network is Dynamic!',                       body: 'The links changed \u2014 a different set of connections is active. This is what makes the network dynamic: an adversary can rewire it each round. Notice the History Tree (right panel) grew one level.',                                                                                                              action: null,                                                        highlight: null            },
        { title: 'Advance to Round 3',                            body: 'Press \u2193 once more to reach Round 3. The leader L now connects to agents it did not reach before \u2014 this is how the network stays unpredictable.',                                                                                                                                                           action: 'Press \u2193 or scroll to advance',                         highlight: '#btn-next-round' },
        { title: 'Five Dynamic Rounds',                           body: 'This network has 5 rounds \u2014 advance all the way to Round 5 using \u2193. The algorithm needs the full picture to start identifying agents. The History Tree grows one level per round, and observation-pattern differences make agents distinguishable.',                                                          action: 'Press \u2193 or scroll to reach Round 5',                  highlight: '#btn-next-round' },
        { title: 'Enable the Stabilizing Algorithm',              body: 'Time to count! Click the Stabilizing button in the toolbar. A description banner will appear explaining the algorithm and its theoretical guarantee.',                                                                                                                                                              action: 'Click the Stabilizing button',                              highlight: '.algo-group'   },
        { title: 'Select an Agent',                               body: 'Left-click any agent (circle) in the left panel to select it. The algorithm needs a starting view to work from.',                                                                                                                                                                                                   action: 'Left-click any agent in the left panel',                    highlight: null            },
        { title: 'Execute One Step',                              body: 'Press \u25b6 Step in the toolbar (or Space). The algorithm will look for exposed pairs \u2014 agents that mutually observed each other \u2014 and start assigning anonymity estimates.',                                                                                                                              action: 'Press \u25b6 Step or Space',                                highlight: '#btn-step'     },
        { title: 'Colors Changed!',                               body: 'Look at the History Tree: L turns green immediately (it is unique). Agents 1 & 5 and agents 2, 3 & 4 start YELLOW \u2014 the algorithm recognises they are indistinguishable in round 1, so it can only guess. Keep stepping to watch it resolve the ambiguity.',                                                    action: null,                                                        highlight: null            },
        { title: 'Keep Stepping Until Done',                      body: 'Press \u25b6 Step several more times. Watch the root node of the History Tree \u2014 when it turns green and shows \u201c6\u201d, the algorithm has stabilized and correctly determined n = 6!',                                                                                                                       action: 'Keep pressing \u25b6 Step until the root node turns green', highlight: '#btn-step'     },
        { title: '\ud83c\udf89 n = 6 Counted Successfully!',      body: 'The root node shows 6 \u2014 the stabilizing algorithm has correctly determined n = 6! Try the Terminating tab above to see a stronger variant with a different guarantee.',                                                                                                                                         action: null,                                                        highlight: null            },
    ],

    guidedTerm: [
        { title: 'Terminating \u2014 Stronger Guarantee',         body: 'Unlike Stabilizing (which may briefly show wrong values), the Terminating algorithm halts explicitly and only ever outputs the correct answer. Cost: it terminates by round 3n \u2212 3 instead of 2n \u2212 2. Same 6-agent network is loaded.',                                                                        action: null,                                                        highlight: null            },
        { title: 'Enable Terminating Algorithm',                  body: 'Click the Terminating button in the toolbar. The banner will update to show the 3n \u2212 3 bound. For n = 6 that is at most 15 rounds.',                                                                                                                                                                           action: 'Click the Terminating button',                              highlight: '.algo-group'   },
        { title: 'Select an Agent',                               body: 'Left-click any agent in the left panel. The algorithm analyses the selected agent\u2019s history tree to look for verifiable evidence of the network size.',                                                                                                                                                         action: 'Left-click any agent',                                      highlight: null            },
        { title: 'First Step \u2014 Watch the Colours',           body: 'Press \u25b6 Step. Look at the History Tree colours: cyan = initial-level guess (uncommitted), orange = intermediate refinement. Key point: no node ever turns red \u2014 the algorithm never commits to a wrong answer.',                                                                                           action: 'Press \u25b6 Step or Space',                                highlight: '#btn-step'     },
        { title: 'Building Isles',                                body: 'The Terminating algorithm builds \u201cisles\u201d \u2014 groups of agents whose combined evidence mutually confirms the count. Orange nodes are isles being refined. Only when an isle is fully verified does the root commit (turn green). Keep stepping.',                                                           action: null,                                                        highlight: null            },
        { title: 'Step to Termination',                           body: 'Press \u25b6 Step repeatedly. Each step propagates evidence from newly seen agents. When the algorithm is certain, the root turns green and halts \u2014 no further step will ever change it.',                                                                                                                        action: 'Press \u25b6 Step until the root turns green with n = 6',  highlight: '#btn-step'     },
        { title: '\ud83c\udf89 Terminated \u2014 n = 6!',         body: 'The Terminating algorithm has halted with the provably correct answer n = 6. Unlike Stabilizing, no incorrect output ever appeared. Switch back to the Stabilizing tab and compare the number of steps and round bounds.',                                                                                          action: null,                                                        highlight: null            },
    ],

    commentary: {
        uniqueIdentified: (du, unique, n) => `+${du} agent${du > 1 ? 's' : ''} uniquely identified  (${unique} / ${n})`,
        classesChanged:   (from, to)       => `Anonymity classes: ${from} \u2192 ${to}`,
        rootGuessChanged: (was, now, ok)   => `Root guess: ${was} \u2192 ${now}${ok ? '  \u2713 correct!' : ''}`,
        nodesGuessed:     (dg, total)      => `${dg} node${dg > 1 ? 's' : ''} assigned a guess  (${total} total)`,
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
        guidedSkip:  'Skip \u2192\u2192',
        guidedNext:  'Next \u2192',
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
<div class="gl-entry"><div class="gl-term">Stabilizing Algorithm</div><div class="gl-def">An algorithm that eventually outputs the correct answer and <em>never changes it again</em>, but may output wrong values before it stabilises. The simulator\u2019s stabilizing algorithm is guaranteed to stabilise by round 2n \u2212 2. Select it with the <strong>Stabilizing</strong> button.</div></div>
<div class="gl-entry"><div class="gl-term">Terminating Algorithm</div><div class="gl-def">An algorithm that outputs the correct answer and then <em>halts</em>, explicitly signalling that it is done. This is a stronger guarantee than stabilizing: once it stops, the answer is provably correct. Select it with the <strong>Terminating</strong> button.</div></div>
<div class="gl-entry"><div class="gl-term">Outdegree Awareness</div><div class="gl-def">An optional capability (toggle with <strong>O</strong>) where each agent also knows how many messages it <em>sent</em> in the previous round. This extra information can help the algorithm make faster or more accurate guesses.</div></div>
<div class="gl-entry"><div class="gl-term">Exposed Pair</div><div class="gl-def">Two agents A and B such that A observes B and B observes A with matching multiplicities. Exposed pairs are the key mechanism the stabilizing algorithm uses to propagate anonymity estimates across the network.</div></div>
<div id="glossary-ref">Theoretical background: <a href="https://arxiv.org/abs/2404.02673" target="_blank" rel="noopener">arxiv.org/abs/2404.02673</a></div>`,
    },
};
