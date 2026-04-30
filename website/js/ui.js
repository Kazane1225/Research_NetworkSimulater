// ── Icon rendering ─────────────────────────────────────────────
// Lucide is loaded via CDN in index.html (script tag before this file).
// Replace all <i data-lucide="..."> placeholders with inline SVGs.
if (typeof lucide !== 'undefined') lucide.createIcons();

// ── Canvas sizing ─────────────────────────────────────────────
const canvas     = document.getElementById('canvas');
const canvasWrap = document.getElementById('canvas-wrap');

function resizeCanvas() {
    const dpr = window.devicePixelRatio || 1;
    const w   = canvasWrap.clientWidth;
    const h   = canvasWrap.clientHeight;
    // Set buffer to device pixels so WebGL/SDL draws at full physical resolution.
    canvas.width  = Math.round(w * dpr);
    canvas.height = Math.round(h * dpr);
    // Keep CSS display size equal to the logical (CSS) pixel size so the canvas
    // fills the container without overflowing regardless of DPR or browser zoom.
    canvas.style.width  = w + 'px';
    canvas.style.height = h + 'px';
}
window.addEventListener('resize', resizeCanvas);
resizeCanvas();

// ── Toolbar scroll-fade hint ──────────────────────────────────
const toolbar = document.getElementById('toolbar');
function checkToolbarScroll() {
    const atEnd = toolbar.scrollLeft + toolbar.clientWidth >= toolbar.scrollWidth - 2;
    toolbar.classList.toggle('scrolled-end', atEnd);
}
toolbar.addEventListener('scroll', checkToolbarScroll);
window.addEventListener('resize', checkToolbarScroll);
checkToolbarScroll();

// Emscripten Module entry point (must be set before index.js loads)
function _showError(msg) {
    const el = document.createElement('div');
    el.style.cssText = 'position:fixed;inset:0;display:flex;align-items:center;justify-content:center;z-index:9999;background:rgba(0,0,0,.8)';
    el.innerHTML = `<pre style="background:#1c2128;color:#f85149;border:1px solid #f85149;border-radius:8px;padding:20px;max-width:80%;white-space:pre-wrap;font-size:13px">${msg}</pre>`;
    document.body.appendChild(el);
    el.addEventListener('click', () => el.remove());
}
window.addEventListener('error', e => _showError(`JS Error: ${e.message}\n${e.filename}:${e.lineno}`));

var Module = {
    canvas,
    onAbort:  msg => _showError(`WASM Abort:\n${msg}`),
    onExit:   code => { if (code !== 0) _showError(`WASM terminated (exit code ${code})\nCheck the browser console (F12) for SDL / WebGL errors from [wasm]`); },
    printErr: msg => { console.error('[wasm]', msg); },
};

// ── Synthetic keyboard dispatch ───────────────────────────────
// Emscripten registers its listener on window (capture phase).
// Dispatching on window triggers it just like a real keystroke.
function pressKey(key, code, extra) {
    window.dispatchEvent(
        new KeyboardEvent('keydown', Object.assign(
            { key, code, bubbles: true, cancelable: true }, extra))
    );
}

// ── Algorithm state (mirrors C-side `algorithm`) ──────────────
let algoState = 0; // 0 = none, 1 = stabilizing, 2 = terminating
const algoButtons = [
    document.getElementById('algo-none'),
    document.getElementById('algo-stab'),
    document.getElementById('algo-term'),
];
const stepBtn = document.getElementById('btn-step');

// ── Algorithm explanation banner ───────────────────────────
const algoBanner     = document.getElementById('algo-banner');
const algoBannerTitle = document.getElementById('algo-banner-title');
const algoBannerDesc  = document.getElementById('algo-banner-desc');
const abBoundLabel    = document.getElementById('ab-bound-label');

const ALGO_INFO = [
    null, // 0 = None
    {
        title: 'Stabilizing Counting Algorithm',
        desc:  'Agents broadcast their history trees each round. The algorithm finds ‘exposed pairs’ — two agents that observed each other — and propagates anonymity estimates from the leader outward. It may output wrong guesses at first, but is guaranteed to stabilise on the correct count n by round 2n − 2.',
        boundLabel: 'Stabilizes by round',
        bound: n => n > 0 ? `2n − 2 = ${2 * n - 2}` : '—',
    },
    {
        title: 'Terminating Counting Algorithm',
        desc:  'A stronger variant: the algorithm explicitly halts and signals “done” when it is certain the count is correct. It builds complete ‘isles’ of counted agents before announcing the result, so no incorrect output is ever committed to.',
        boundLabel: 'Terminates by round',
        bound: n => n > 0 ? `3n − 3 = ${3 * n - 3}` : '—',
    },
];

function syncBanner() {
    if (algoState === 0) {
        algoBanner.classList.remove('visible');
        return;
    }
    const info = ALGO_INFO[algoState];
    algoBannerTitle.textContent = info.title;
    algoBannerDesc .textContent = info.desc;
    abBoundLabel   .textContent = info.boundLabel;
    algoBanner.classList.add('visible');
}

// ── Node-colour legend ────────────────────────────────────
const nodeLegend   = document.getElementById('node-legend');
const legStabItems = document.querySelectorAll('.leg-stab');
const legTermItems = document.querySelectorAll('.leg-term');

function syncLegend() {
    nodeLegend.classList.toggle('visible', algoState !== 0);
    legStabItems.forEach(el => el.style.display = algoState === 1 ? '' : 'none');
    legTermItems.forEach(el => el.style.display = algoState === 2 ? '' : 'none');
}

function syncAlgoUI() {
    algoButtons.forEach((b, i) => b.classList.toggle('active', i === algoState));
    stepBtn.disabled = (algoState === 0);
    syncLegend();
    syncBanner();
}

function cycleAlgoTo(target) {
    while (algoState !== target) {
        pressKey('Tab', 'Tab');
        algoState = (algoState + 1) % 3;
    }
    syncAlgoUI();
}

algoButtons.forEach((btn, i) =>
    btn.addEventListener('click', () => cycleAlgoTo(i))
);

// Keep algoState in sync when user presses TAB themselves
// (capture phase, before Emscripten, so we read the key before it's consumed)
window.addEventListener('keydown', e => {
    if (e.isTrusted && e.code === 'Tab') {
        algoState = (algoState + 1) % 3;
        syncAlgoUI();
    }
}, true);

// ── Toolbar button → key mappings ─────────────────────────────
document.getElementById('btn-prev-round')  .addEventListener('click', () => pressKey('ArrowUp',    'ArrowUp'));
document.getElementById('btn-next-round')  .addEventListener('click', () => pressKey('ArrowDown',  'ArrowDown'));
document.getElementById('btn-add-round')   .addEventListener('click', () => pressKey('+',          'Equal'));
document.getElementById('btn-del-round')   .addEventListener('click', () => pressKey('-',          'Minus'));
document.getElementById('btn-clear-round') .addEventListener('click', () => pressKey('Backspace',  'Backspace'));
document.getElementById('btn-step')        .addEventListener('click', () => pressKey(' ',          'Space'));
document.getElementById('btn-delete-agent').addEventListener('click', () => pressKey('Delete',     'Delete'));
document.getElementById('btn-deselect')    .addEventListener('click', () => pressKey('Escape',     'Escape'));
document.getElementById('btn-load')        .addEventListener('click', () => pressKey('l',          'KeyL'));
document.getElementById('btn-save')        .addEventListener('click', () => pressKey('s',          'KeyS'));

// ── Options popover ───────────────────────────────────────────
const optPanel = document.getElementById('options-panel');

document.getElementById('btn-options').addEventListener('click', e => {
    e.stopPropagation();
    optPanel.classList.toggle('open');
});
document.addEventListener('click', () => optPanel.classList.remove('open'));
optPanel.addEventListener('click', e => e.stopPropagation());

// Close options panel on Escape
window.addEventListener('keydown', e => {
    if (e.code === 'Escape' && optPanel.classList.contains('open')) {
        optPanel.classList.remove('open');
    }
});

document.getElementById('opt-outdegree')  .addEventListener('click', () => pressKey('o', 'KeyO'));
document.getElementById('opt-arrows')     .addEventListener('click', () => pressKey('a', 'KeyA'));
document.getElementById('opt-round-nodes').addEventListener('click', () => pressKey('b', 'KeyB'));
document.getElementById('opt-highlight')  .addEventListener('click', () => pressKey('c', 'KeyC'));
document.getElementById('opt-red-edge')   .addEventListener('click', () => pressKey('d', 'KeyD'));
document.getElementById('opt-grid')       .addEventListener('click', () => pressKey('g', 'KeyG'));
document.getElementById('opt-caps')       .addEventListener('click', () => {
    // Emscripten tracks CapsLock internally via webCapsTwoWayDefault,
    // so just dispatching the keydown is enough.
    pressKey('CapsLock', 'CapsLock');
});

// ── Help modal ────────────────────────────────────────────────
const helpModal = document.getElementById('help-modal');

function openHelp()  { helpModal.classList.add('open');    document.body.classList.add('modal-open'); }
function closeHelp() { helpModal.classList.remove('open'); document.body.classList.remove('modal-open'); }

document.getElementById('btn-help') .addEventListener('click', openHelp);
document.getElementById('help-close').addEventListener('click', closeHelp);
helpModal.addEventListener('click', e => { if (e.target === helpModal) closeHelp(); });

// Intercept H key: show HTML modal instead of the C-side text overlay.
// Because this listener registers before Emscripten (WASM inits later),
// stopImmediatePropagation prevents the C callback from ever seeing H.
window.addEventListener('keydown', e => {
    if (!e.isTrusted) return;
    if (e.code === 'KeyH' && !e.ctrlKey && !e.altKey && !e.metaKey) {
        e.stopImmediatePropagation();
        helpModal.classList.toggle('open');
    }
    // Escape also closes help modal
    if (e.code === 'Escape' && helpModal.classList.contains('open')) {
        closeHelp();
    }
}, true);

// ── Glossary modal ────────────────────────────────────────
const glossaryModal = document.getElementById('glossary-modal');

function openGlossary()  { glossaryModal.classList.add('open');    document.body.classList.add('modal-open'); }
function closeGlossary() { glossaryModal.classList.remove('open'); document.body.classList.remove('modal-open'); }

document.getElementById('btn-glossary')  .addEventListener('click', openGlossary);
document.getElementById('glossary-close').addEventListener('click', closeGlossary);
glossaryModal.addEventListener('click', e => { if (e.target === glossaryModal) closeGlossary(); });

window.addEventListener('keydown', e => {
    if (e.code === 'Escape' && glossaryModal.classList.contains('open')) {
        e.stopImmediatePropagation();
        closeGlossary();
    }
}, true);
// ── Network stats panel ───────────────────────────────
const statAgents  = document.getElementById('stat-agents');
const statLeaders = document.getElementById('stat-leaders');
const statRounds  = document.getElementById('stat-rounds');
const statLinks   = document.getElementById('stat-links');
const statClasses = document.getElementById('stat-classes');
const statUnique  = document.getElementById('stat-unique');

function updateNetStats() {
    if (typeof Module._GetNumAgents !== 'function') return;
    const n       = Module._GetNumAgents();
    const leaders = Module._GetNumLeaders();
    const rounds  = Module._GetNumRounds();
    const r       = Module._GetCurrentRound();   // 0-based; -1 = no rounds
    const links   = Module._GetCurrentRoundLinks();
    const classes = Module._GetNumAnonymityClasses();
    const unique  = Module._GetNumUniqueAgents();

    statAgents .textContent = n > 0 ? n : '—';
    statLeaders.textContent = n > 0 ? leaders : '—';
    statRounds .textContent = n > 0 ? rounds  : '—';
    statLinks  .textContent = r >= 0 ? `${links}  (round ${r + 1})` : '—';
    statClasses.textContent = classes > 0 ? classes : '—';
    statUnique .textContent = n > 0 ? unique  : '—';

    // Update banner live stats
    if (algoState !== 0 && ALGO_INFO[algoState]) {
        document.getElementById('ab-agents') .textContent = n > 0 ? n : '?';
        document.getElementById('ab-unique') .textContent = unique;
        document.getElementById('ab-classes').textContent = classes > 0 ? classes : '—';
        document.getElementById('ab-bound')  .textContent = ALGO_INFO[algoState].bound(n);
    }
}

setInterval(updateNetStats, 400);

// ── UI Tour (existing tutorial) ─────────────────────────
const TUTORIAL_STEPS = [
    {
        title: 'Welcome',
        text:  'This short tour introduces the key ideas behind anonymous dynamic networks. Use Next / Prev to navigate, or close the panel at any time. Click the Tour button in the toolbar to toggle this panel.',
        sel:   null,
    },
    {
        title: 'Step 1 — Agents',
        text:  'The left panel shows the network. Each circle is an agent — a computing entity with no unique identifier. They are completely anonymous: no agent has a name or fixed ID.',
        sel:   '#label-network',
    },
    {
        title: 'Step 2 — The History Tree',
        text:  'The right panel shows the history tree: a record of everything each agent has observed, round by round. Two agents are indistinguishable if and only if their history trees are identical.',
        sel:   '#label-history',
    },
    {
        title: 'Step 3 — Dynamic Rounds',
        text:  'The network topology can change every round — this is what makes it dynamic. Use ▲ / ▼ or the scroll wheel on the canvas to browse rounds and see how the edges evolve over time.',
        sel:   '#btn-prev-round',
    },
    {
        title: 'Step 4 — Loading a Network',
        text:  'Click the Load button to open an example from the networks/ folder. DefaultNetwork2.txt is a good starting point; BoldiVigna.txt is a classic from the research literature.',
        sel:   '#btn-load',
    },
    {
        title: 'Step 5 — Counting Algorithms',
        text:  'Select Stabilizing or Terminating from the Algorithm buttons. A description banner will appear below the toolbar explaining how the algorithm works and its theoretical round bound.',
        sel:   '.algo-group',
    },
    {
        title: 'Step 6 — Stepping Through',
        text:  'Left-click an agent or a history-tree node to select it, then press ▶ Step (or Space) to run one algorithm step. Watch nodes change colour in the History Tree as the algorithm deduces the count.',
        sel:   '#btn-step',
    },
    {
        title: 'Step 7 — Network Stats',
        text:  'The Network Stats panel (bottom-left of the canvas) shows live information: agent count n, round count T, anonymity classes, and how many agents are already uniquely identified.',
        sel:   '#net-stats',
    },
    {
        title: 'All Done!',
        text:  "You're ready to explore! Open the Glossary for key term definitions and Help for all keyboard shortcuts. Try loading different networks and comparing how the two algorithms behave.",
        sel:   null,
    },
];

let tutStep = 0;
let tutOpen = false;
let tutHighlightEl = null;

const tutPanel     = document.getElementById('tutorial-panel');
const tutIndicator = document.getElementById('tutorial-indicator');
const tutTitleEl   = document.getElementById('tutorial-title');
const tutBodyEl    = document.getElementById('tutorial-body');
const tutPrevBtn   = document.getElementById('tutorial-prev');
const tutNextBtn   = document.getElementById('tutorial-next');
const tutCloseBtn  = document.getElementById('tutorial-close');
const tutToggleBtn = document.getElementById('btn-tutorial');

function clearTutHighlight() {
    if (tutHighlightEl) {
        tutHighlightEl.classList.remove('tutorial-highlight');
        tutHighlightEl = null;
    }
}

function renderTutStep() {
    const s = TUTORIAL_STEPS[tutStep];
    tutIndicator.textContent = `${tutStep + 1} / ${TUTORIAL_STEPS.length}`;
    tutTitleEl.textContent   = s.title;
    tutBodyEl.textContent    = s.text;
    tutPrevBtn.disabled      = (tutStep === 0);
    tutNextBtn.textContent   = tutStep === TUTORIAL_STEPS.length - 1 ? '✓ Done' : 'Next →';
    clearTutHighlight();
    if (s.sel) {
        const el = document.querySelector(s.sel);
        if (el) { el.classList.add('tutorial-highlight'); tutHighlightEl = el; }
    }
}

function openTutorial() {
    tutStep = 0;
    tutOpen = true;
    tutPanel.classList.add('open');
    tutToggleBtn.classList.add('active');
    renderTutStep();
}

function closeTutorial() {
    tutOpen = false;
    tutPanel.classList.remove('open');
    tutToggleBtn.classList.remove('active');
    clearTutHighlight();
}

tutToggleBtn.addEventListener('click', () => { if (tutOpen) closeTutorial(); else openTutorial(); });
tutCloseBtn .addEventListener('click', closeTutorial);
tutPrevBtn  .addEventListener('click', () => { if (tutStep > 0) { tutStep--; renderTutStep(); } });
tutNextBtn  .addEventListener('click', () => {
    if (tutStep < TUTORIAL_STEPS.length - 1) { tutStep++; renderTutStep(); }
    else closeTutorial();
});

window.addEventListener('keydown', e => {
    if (tutOpen && e.code === 'Escape') { e.stopImmediatePropagation(); closeTutorial(); }
}, true);

// ── Interactive Guided Tutorial ────────────────────────────────────────────
// Steps with trigger=null require the user to click Next.
// Steps with a trigger function auto-advance when the condition becomes true.
// Steps with waitForStep=true also require the user to press ▶ Step / Space.
let guidedStepFired = false;  // set to true when user presses Step/Space

const GUIDED_STEPS = [
    {
        title: 'Welcome — Let\'s Count!',
        body:  'We\'ve loaded a small example network: 6 agents across 5 rounds. Your goal is to understand how the counting algorithm works — all without knowing any agent\'s identity.',
        action: null,
        trigger: null,
    },
    {
        title: 'Agents are Anonymous',
        body:  'The circles in the left panel are agents. They have no names or IDs. The only exception is the L (leader), which has a special starting input of 0. All other agents start with input 1 and are completely identical at first.',
        action: null,
        trigger: null,
    },
    {
        title: 'Round 1: A Ring',
        body:  'The network starts as a ring: L — 1 — 2 — 3 — 4 — 5 — L. Notice that agents 1 and 5 are in symmetric positions (both sit next to L), and agents 2, 3, 4 are also symmetric. This matters — the algorithm will have to figure out who is who!',
        action: null,
        trigger: null,
    },
    {
        title: 'Advance to Round 2',
        body:  'Press ↓ or scroll the mouse wheel on the canvas to go to Round 2. Notice which agents the leader L is now connected to — the topology is already different!',
        action: 'Press ↓ or scroll to advance',
        trigger: () => typeof Module._GetCurrentRound === 'function' && Module._GetCurrentRound() >= 1,
    },
    {
        title: 'The Network is Dynamic!',
        body:  'The links changed — a different set of connections is active. This is what makes the network dynamic: an adversary can rewire it each round. Notice the History Tree (right panel) grew one level.',
        action: null,
        trigger: null,
    },
    {
        title: 'Advance to Round 3',
        body:  'Press ↓ once more to reach Round 3. The leader L now connects to agents it did not reach before — this is how the network stays unpredictable.',
        action: 'Press ↓ or scroll to advance',
        trigger: () => typeof Module._GetCurrentRound === 'function' && Module._GetCurrentRound() >= 2,
    },
    {
        title: 'Five Dynamic Rounds',
        body:  'This network has 5 rounds — feel free to browse all of them with ↓. The History Tree grows one level per round. Agents with different observation sequences have different tree shapes, making them distinguishable.',
        action: null,
        trigger: null,
    },
    {
        title: 'Enable the Stabilizing Algorithm',
        body:  'Time to count! Click the Stabilizing button in the toolbar. A description banner will appear below explaining the algorithm and its theoretical guarantee.',
        action: 'Click the Stabilizing button',
        trigger: () => algoState === 1,
    },
    {
        title: 'Select an Agent',
        body:  'Left-click any agent (circle) in the left panel to select it. The algorithm needs a starting view to work from.',
        action: 'Left-click any agent in the left panel',
        trigger: () => typeof Module._GetSelectedEntity === 'function' && Module._GetSelectedEntity() >= 0,
    },
    {
        title: 'Execute One Step',
        body:  'Press ▶ Step in the toolbar (or Space). The algorithm will look for exposed pairs — agents that mutually observed each other — and start assigning anonymity estimates.',
        action: 'Press ▶ Step or Space',
        trigger: () => guidedStepFired,
    },
    {
        title: 'Colors Changed!',
        body:  'Look at the History Tree: L turns green immediately (it is unique). But agents 1 & 5 and agents 2, 3 & 4 start YELLOW — the algorithm recognises they are indistinguishable in round 1, so it can only guess. Keep stepping to watch it resolve the ambiguity.',
        action: null,
        trigger: null,
    },
    {
        title: 'Keep Stepping Until Done',
        body:  'Press ▶ Step several more times. Watch the root node of the History Tree — when it turns green and shows "6", the algorithm has stabilized and correctly determined n = 6!',
        action: 'Keep pressing ▶ Step until the root node shows 6 and turns green',
        trigger: () => typeof Module._GetRootGuess === 'function' &&
                       typeof Module._GetNumAgents === 'function' &&
                       Module._GetNumAgents() > 0 &&
                       Module._GetRootGuess() === Module._GetNumAgents(),
    },
    {
        title: '🎉 n = 6 Counted Successfully!',
        body:  'The root node shows 6 — the stabilizing algorithm has correctly determined n = 6! The green nodes show the equivalence classes it counted. Now try: load a bigger network, switch to the Terminating algorithm, or open the Glossary for more concepts.',
        action: null,
        trigger: null,
    },
];

let guidedStep = 0;
let guidedOpen = false;
let guidedHighlightEl = null;
let guidedPollTimer   = null;

const guidedPanel     = document.getElementById('guided-panel');
const guidedIndicator = document.getElementById('guided-indicator');
const guidedTitleEl   = document.getElementById('guided-title');
const guidedBodyEl    = document.getElementById('guided-body');
const guidedActionDiv = document.getElementById('guided-action');
const guidedActionTxt = document.getElementById('guided-action-text');
const guidedSkipBtn   = document.getElementById('guided-skip');
const guidedNextBtn   = document.getElementById('guided-next');
const guidedCloseBtn  = document.getElementById('guided-close');
const learnToggleBtn  = document.getElementById('btn-learn');

function clearGuidedHighlight() {
    if (guidedHighlightEl) {
        guidedHighlightEl.classList.remove('tutorial-highlight');
        guidedHighlightEl = null;
    }
}

function renderGuidedStep() {
    const s = GUIDED_STEPS[guidedStep];
    const isLast = guidedStep === GUIDED_STEPS.length - 1;

    guidedIndicator.textContent = `${guidedStep + 1} / ${GUIDED_STEPS.length}`;
    guidedTitleEl.textContent   = s.title;
    guidedBodyEl.textContent    = s.body;

    // Action prompt
    if (s.action) {
        guidedActionDiv.style.display = '';
        guidedActionTxt.textContent   = s.action;
    } else {
        guidedActionDiv.style.display = 'none';
    }

    // Footer buttons
    if (s.trigger) {
        guidedSkipBtn.style.display = '';
        guidedNextBtn.style.display = 'none';
    } else {
        guidedSkipBtn.style.display = 'none';
        guidedNextBtn.style.display = '';
        guidedNextBtn.textContent   = isLast ? '✓ Done' : 'Next →';
    }

    clearGuidedHighlight();
}

function advanceGuided() {
    guidedStepFired = false;
    if (guidedStep < GUIDED_STEPS.length - 1) {
        guidedStep++;
        renderGuidedStep();
    } else {
        closeGuided();
    }
}

function startGuidedPoll() {
    if (guidedPollTimer) return;
    guidedPollTimer = setInterval(() => {
        if (!guidedOpen) { clearInterval(guidedPollTimer); guidedPollTimer = null; return; }
        const s = GUIDED_STEPS[guidedStep];
        if (s.trigger && s.trigger()) advanceGuided();
    }, 300);
}

function openGuided() {
    if (typeof Module._TutorialLoadNetwork !== 'function') {
        alert('WASM not yet ready — please wait a moment and try again.');
        return;
    }
    // Close UI tour if open
    if (tutOpen) closeTutorial();

    // Load tutorial network and reset algorithm state
    Module._TutorialLoadNetwork();
    if (algoState !== 0) cycleAlgoTo(0);

    guidedStep = 0;
    guidedOpen = true;
    guidedStepFired = false;
    guidedPanel.classList.add('open');
    learnToggleBtn.classList.add('active');
    renderGuidedStep();
    startGuidedPoll();
}

function closeGuided() {
    guidedOpen = false;
    guidedPanel.classList.remove('open');
    learnToggleBtn.classList.remove('active');
    clearGuidedHighlight();
    if (guidedPollTimer) { clearInterval(guidedPollTimer); guidedPollTimer = null; }
}

learnToggleBtn.addEventListener('click', () => { if (guidedOpen) closeGuided(); else openGuided(); });
guidedCloseBtn.addEventListener('click', closeGuided);
guidedSkipBtn .addEventListener('click', advanceGuided);
guidedNextBtn .addEventListener('click', advanceGuided);

// Detect Step button press for waitForStep steps
function onStepPressed() { if (guidedOpen) guidedStepFired = true; }
document.getElementById('btn-step').addEventListener('click', onStepPressed);
window.addEventListener('keydown', e => {
    if (guidedOpen && e.isTrusted && e.code === 'Space') onStepPressed();
    if (guidedOpen && e.code === 'Escape') { e.stopImmediatePropagation(); closeGuided(); }
}, true);