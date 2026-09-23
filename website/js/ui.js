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
    onRuntimeInitialized: () => {
        if (typeof syncThemeToWasm === 'function') syncThemeToWasm();
    },
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

// ALGO_INFO moved to i18n.js → getLocale().algoInfo

function syncBanner() {
    if (algoState === 0) {
        algoBanner.classList.remove('visible');
        return;
    }
    const info = getLocale().algoInfo[algoState];
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
const repeatCleanupFns = [];

function waitForAnimationFrame() {
    return new Promise(resolve => requestAnimationFrame(() => resolve()));
}

// True while the backend's background compute worker thread is still processing a previous
// recompute (see src/compute_job.c). Edits attempted while this is true are silently rejected by
// the C side (CanMutateNetwork() in events.c), so the auto-repeat loop below must wait for this
// to clear before firing the next repeat -- otherwise it fires anyway, the edit is rejected, the
// round count never changes, and waitForToolbarRoundChange() below would wait in vain and give
// up on the whole press-and-hold gesture even though the button is still held.
function isComputeJobBusy() {
    return typeof Module._IsComputeJobBusy === 'function' && !!Module._IsComputeJobBusy();
}

// Polls once per animation frame until the backend is idle, or `isStillHeld` becomes false (the
// button was released / a new press-and-hold gesture started in the meantime).
async function waitWhileComputeJobBusy(isStillHeld) {
    while (isStillHeld() && isComputeJobBusy()) {
        await waitForAnimationFrame();
    }
}

async function waitForToolbarRoundChange(readState, previousState, timeoutMs = 2000) {
    const deadline = performance.now() + timeoutMs;
    while (performance.now() < deadline) {
        if (readState() !== previousState) {
            await waitForAnimationFrame();
            return true;
        }
        await waitForAnimationFrame();
    }
    return false;
}

function bindToolbarKeyButton(id, key, code, opts = {}) {
    const btn = document.getElementById(id);
    const repeat = !!opts.repeat;
    const initialDelay = opts.initialDelay ?? 350;
    const interval = opts.interval ?? 75;
    const readRepeatState = opts.readRepeatState ?? null;
    let delayTimer = null;
    let intervalTimer = null;
    let activePointerId = null;
    let repeated = false;
    let repeatGeneration = 0;

    function trigger() {
        if (!btn.disabled) pressKey(key, code);
    }

    async function triggerUntilReleased(pointerId, generation) {
        const isStillHeld = () => activePointerId === pointerId && repeatGeneration === generation;
        while (isStillHeld()) {
            if (readRepeatState) {
                // Wait for any in-flight background recompute to finish before firing the next
                // repeat, so this press is guaranteed to be accepted rather than silently
                // rejected while busy (see isComputeJobBusy() above).
                await waitWhileComputeJobBusy(isStillHeld);
                if (!isStillHeld()) break;
            }
            const beforeState = readRepeatState ? readRepeatState() : null;
            trigger();
            repeated = true;
            if (!readRepeatState) {
                await new Promise(resolve => {
                    intervalTimer = setTimeout(resolve, interval);
                });
                intervalTimer = null;
                continue;
            }
            const changed = await waitForToolbarRoundChange(readRepeatState, beforeState);
            if (!changed) break;
        }
    }

    function stopRepeat() {
        repeatGeneration++;
        if (delayTimer !== null) {
            clearTimeout(delayTimer);
            delayTimer = null;
        }
        if (intervalTimer !== null) {
            clearTimeout(intervalTimer);
            intervalTimer = null;
        }
        if (repeated) btn.dataset.suppressClick = '1';
        repeated = false;
        activePointerId = null;
    }

    btn.addEventListener('click', () => {
        if (btn.dataset.suppressClick === '1') {
            delete btn.dataset.suppressClick;
            return;
        }
        trigger();
    });

    if (!repeat) return;

    btn.addEventListener('pointerdown', e => {
        if (e.button !== 0 || btn.disabled || activePointerId !== null) return;
        activePointerId = e.pointerId;
        repeated = false;
        const generation = ++repeatGeneration;
        delayTimer = setTimeout(() => {
            delayTimer = null;
            void triggerUntilReleased(e.pointerId, generation);
        }, initialDelay);
    });

    btn.addEventListener('pointerup', e => {
        if (e.pointerId === activePointerId) stopRepeat();
    });
    btn.addEventListener('pointerleave', e => {
        if (e.pointerId === activePointerId) stopRepeat();
    });
    btn.addEventListener('pointercancel', stopRepeat);
    repeatCleanupFns.push(stopRepeat);
}

window.addEventListener('blur', () => repeatCleanupFns.forEach(fn => fn()));

bindToolbarKeyButton('btn-prev-round', 'ArrowUp', 'ArrowUp', { repeat: true });
bindToolbarKeyButton('btn-next-round', 'ArrowDown', 'ArrowDown', { repeat: true });
bindToolbarKeyButton('btn-add-round', '+', 'Equal', {
    repeat: true,
    readRepeatState: () => typeof Module._GetNumRounds === 'function' ? Module._GetNumRounds() : null,
});
bindToolbarKeyButton('btn-del-round', '-', 'Minus', {
    repeat: true,
    readRepeatState: () => typeof Module._GetNumRounds === 'function' ? Module._GetNumRounds() : null,
});
bindToolbarKeyButton('btn-clear-round', 'Backspace', 'Backspace');
bindToolbarKeyButton('btn-step', ' ', 'Space');
bindToolbarKeyButton('btn-delete-agent', 'Delete', 'Delete');
bindToolbarKeyButton('btn-deselect', 'Escape', 'Escape');
bindToolbarKeyButton('btn-load', 'l', 'KeyL');
bindToolbarKeyButton('btn-save', 's', 'KeyS');

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

// ── Professor / Student mode ──────────────────────────────
const studentToggle = document.getElementById('opt-student-toggle');

function setStudentMode(enabled) {
    document.body.classList.toggle('student-mode', enabled);
    studentToggle.classList.toggle('on', enabled);
    localStorage.setItem('studentMode', enabled ? '1' : '0');
}

// Restore saved mode (default: professor = off)
setStudentMode(localStorage.getItem('studentMode') === '1');

document.getElementById('opt-student-mode').addEventListener('click', () => {
    setStudentMode(!document.body.classList.contains('student-mode'));
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
const netStatsPanel = document.getElementById('net-stats');
const netStatsShow  = document.getElementById('net-stats-show');
const netStatsToggle = document.getElementById('opt-net-stats-toggle');
const statAgents  = document.getElementById('stat-agents');
const statLeaders = document.getElementById('stat-leaders');
const statRounds  = document.getElementById('stat-rounds');
const statLinks   = document.getElementById('stat-links');
const statClasses = document.getElementById('stat-classes');
const statUnique  = document.getElementById('stat-unique');

function setNetStatsVisible(visible) {
    netStatsPanel.classList.toggle('hidden', !visible);
    netStatsShow.classList.toggle('visible', !visible);
    netStatsToggle.classList.toggle('on', visible);
    try { localStorage.setItem('netStatsVisible', visible ? '1' : '0'); } catch (_) {}
}

document.getElementById('net-stats-close').addEventListener('click', () => setNetStatsVisible(false));
netStatsShow.addEventListener('click', () => setNetStatsVisible(true));
document.getElementById('opt-net-stats').addEventListener('click', () => {
    setNetStatsVisible(netStatsPanel.classList.contains('hidden'));
});

// Restore preference (default: visible)
try {
    setNetStatsVisible(localStorage.getItem('netStatsVisible') !== '0');
} catch (_) {
    setNetStatsVisible(true);
}

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
    if (algoState !== 0 && getLocale().algoInfo[algoState]) {
        document.getElementById('ab-agents') .textContent = n > 0 ? n : '?';
        document.getElementById('ab-unique') .textContent = unique;
        document.getElementById('ab-classes').textContent = classes > 0 ? classes : '—';
        document.getElementById('ab-bound')  .textContent = getLocale().algoInfo[algoState].bound(n);
    }
}

setInterval(updateNetStats, 400);

// ── UI Tour (existing tutorial) ─────────────────────────
const TUTORIAL_STEPS = [
    {
        title: 'Welcome to the Tour',
        text:  'A quick walkthrough of the main UI areas. Press Next to continue, ← Prev to go back, or ✕ to close at any time.',
        sel:   null,
    },
    {
        title: 'The Canvas',
        text:  'The main area is split in two. Left half: the network — circles are agents, anonymous computing entities with no unique ID. Right half: the History Tree — records what each agent has observed round by round. Two agents are indistinguishable if and only if their history trees are identical.',
        sel:   '#canvas-wrap',
    },
    {
        title: 'Dynamic Rounds',
        text:  'Edges can change every round — that\'s what makes the network dynamic. Use ▲ / ▼ in the toolbar or the scroll wheel on the canvas to step through rounds and watch the topology evolve.',
        sel:   '#btn-prev-round',
    },
    {
        title: 'Counting Algorithms',
        text:  'Activate Stabilizing or Terminating to run a counting algorithm. A banner appears below the toolbar with a description of the algorithm and its theoretical round bound.',
        sel:   '.algo-group',
    },
    {
        title: 'Executing Steps',
        text:  'Select an agent or history-tree node, then press ▶ Step (or Space) to run one algorithm step. History-tree nodes change colour as the algorithm deduces the network size.',
        sel:   '#btn-step',
    },
    {
        title: 'Loading Networks',
        text:  'Click Load to open an example from the networks/ folder. DefaultNetwork2.txt is a good starting point; BoldiVigna.txt is a well-known example from the research literature.',
        sel:   '#btn-load',
    },
    {
        title: 'Network Stats',
        text:  'The Network Stats panel (bottom-left) shows live information: agent count n, round count T, anonymity classes, and how many agents are uniquely identified so far.',
        sel:   '#net-stats',
    },
    {
        title: 'All Done!',
        text:  'You\'re ready to explore! Open Glossary for key term definitions and Help (H) for keyboard shortcuts. Try loading different networks and comparing how both algorithms behave.',
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

function clampNum(v, lo, hi) { return Math.max(lo, Math.min(hi, v)); }

function positionTutPanel(targetEl) {
    const PANEL_W = 320;
    const PANEL_H = 230;   // conservative height estimate
    const GAP     = 14;
    const MARGIN  = 14;
    const vw = window.innerWidth;
    const vh = window.innerHeight;
    if (!targetEl) {
        Object.assign(tutPanel.style, { top: 'auto', left: 'auto', bottom: '20px', right: '20px' });
        delete tutPanel.dataset.arrow;
        return;
    }
    const r = targetEl.getBoundingClientRect();
    // Large element (canvas, full-width bar…): corner placement, no arrow
    if (r.width > vw * 0.4 || r.height > vh * 0.4) {
        Object.assign(tutPanel.style, { top: 'auto', left: 'auto', bottom: '20px', right: '20px' });
        delete tutPanel.dataset.arrow;
        return;
    }
    let top, left, arrow;
    // Try below
    if (r.bottom + GAP + PANEL_H < vh - MARGIN) {
        top   = r.bottom + GAP;
        left  = clampNum(r.left + r.width / 2 - PANEL_W / 2, MARGIN, vw - PANEL_W - MARGIN);
        arrow = 'top';
    // Try above
    } else if (r.top - GAP - PANEL_H > MARGIN) {
        top   = r.top - GAP - PANEL_H;
        left  = clampNum(r.left + r.width / 2 - PANEL_W / 2, MARGIN, vw - PANEL_W - MARGIN);
        arrow = 'bottom';
    // Try right
    } else if (r.right + GAP + PANEL_W < vw - MARGIN) {
        left  = r.right + GAP;
        top   = clampNum(r.top + r.height / 2 - PANEL_H / 2, MARGIN, vh - PANEL_H - MARGIN);
        arrow = 'left';
    // Fallback: left of target
    } else {
        left  = Math.max(MARGIN, r.left - GAP - PANEL_W);
        top   = clampNum(r.top + r.height / 2 - PANEL_H / 2, MARGIN, vh - PANEL_H - MARGIN);
        arrow = 'right';
    }
    Object.assign(tutPanel.style, { top: top + 'px', left: left + 'px', bottom: 'auto', right: 'auto' });
    tutPanel.dataset.arrow = arrow;
}

function renderTutStep() {
    const steps = getLocale().tutorial;
    const s     = steps[tutStep];
    tutIndicator.textContent = `${tutStep + 1} / ${steps.length}`;
    tutTitleEl.textContent   = s.title;
    tutBodyEl.textContent    = s.text;
    tutPrevBtn.disabled      = (tutStep === 0);
    const _tu = getLocale().ui;
    tutNextBtn.textContent   = tutStep === steps.length - 1 ? _tu.tutDone : _tu.tutNext;
    clearTutHighlight();
    delete tutPanel.dataset.arrow;
    if (s.sel) {
        const el = document.querySelector(s.sel);
        if (el) { el.classList.add('tutorial-highlight'); tutHighlightEl = el; positionTutPanel(el); }
        else     { positionTutPanel(null); }
    } else {
        positionTutPanel(null);
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
    Object.assign(tutPanel.style, { top: 'auto', left: 'auto', bottom: '20px', right: '20px' });
    delete tutPanel.dataset.arrow;
}

tutToggleBtn.addEventListener('click', () => { if (tutOpen) closeTutorial(); else openTutorial(); });
tutCloseBtn .addEventListener('click', closeTutorial);
tutPrevBtn  .addEventListener('click', () => { if (tutStep > 0) { tutStep--; renderTutStep(); } });
tutNextBtn  .addEventListener('click', () => {
    if (tutStep < getLocale().tutorial.length - 1) { tutStep++; renderTutStep(); }
    else closeTutorial();
});

window.addEventListener('keydown', e => {
    if (tutOpen && e.code === 'Escape') { e.stopImmediatePropagation(); closeTutorial(); }
}, true);

// ── Interactive Guided Tutorial ────────────────────────────────────────────
// Each step may have:
//   trigger   – () => bool : auto-advances when true (shows Skip, hides Next)
//   action    – string     : instruction shown in the action bar
//   highlight – CSS selector of the toolbar element to pulse-highlight
let guidedStepFired = false;  // set to true when user presses Step/Space

function rootGuessMatchesN() {
    return typeof Module._GetRootGuess === 'function' &&
           typeof Module._GetNumAgents === 'function' &&
           Module._GetNumAgents() > 0 &&
           Module._GetRootGuess() === Module._GetNumAgents();
}

function agentSelected() {
    return typeof Module._GetSelectedEntity === 'function' && Module._GetSelectedEntity() >= 0;
}

// Trigger functions (locale-independent). Indices must match guidedStab / guidedTerm.
const STAB_TRIGGERS = [
    null,                 // Stabilizing vs Terminating
    null,                 // This Demo Network
    () => algoState === 1,
    () => agentSelected(),
    () => guidedStepFired,
    null,                 // Provisional Guesses
    null,                 // Why Not Halt Here?
    () => rootGuessMatchesN(),
    null,                 // Stabilized — No “Done” Signal
];

const TERM_TRIGGERS = [
    null,                 // Terminating: Halt With a Proof
    () => algoState === 2,
    () => agentSelected(),
    () => guidedStepFired,
    null,                 // Certificates, Not Just Guesses
    () => rootGuessMatchesN(),
    null,                 // Compare the Two Guarantees
];

function buildGuidedSteps(localeSteps, triggers) {
    return localeSteps.map((s, i) => Object.assign({}, s, { trigger: triggers[i] || null }));
}

function getScenarios() {
    const L = getLocale();
    return [
        { name: 'Stabilizing', steps: buildGuidedSteps(L.guidedStab, STAB_TRIGGERS) },
        { name: 'Terminating', steps: buildGuidedSteps(L.guidedTerm, TERM_TRIGGERS) },
    ];
}

// Reload the tutorial network and clear algorithm state for a Learn scenario.
function prepareLearnNetwork() {
    if (typeof Module._TutorialLoadNetwork !== 'function') return false;
    Module._TutorialLoadNetwork();
    if (algoState !== 0) cycleAlgoTo(0);
    return true;
}

let guidedStep     = 0;
let guidedScenario = 0;
let guidedOpen     = false;
let guidedHighlightEls = [];
let guidedPollTimer   = null;

const guidedPanel     = document.getElementById('guided-panel');
const guidedIndicator = document.getElementById('guided-indicator');
const guidedTitleEl   = document.getElementById('guided-title');
const guidedBodyEl    = document.getElementById('guided-body');
const guidedActionDiv = document.getElementById('guided-action');
const guidedActionTxt = document.getElementById('guided-action-text');
const guidedPrevBtn   = document.getElementById('guided-prev');
const guidedSkipBtn   = document.getElementById('guided-skip');
const guidedNextBtn   = document.getElementById('guided-next');
const guidedCloseBtn  = document.getElementById('guided-close');
const learnToggleBtn  = document.getElementById('btn-learn');
const stepCommentaryEl = document.getElementById('step-commentary');
const stepToastEl      = document.getElementById('step-toast');

function clearGuidedHighlight() {
    guidedHighlightEls.forEach(el => {
        el.classList.remove('guided-highlight');
        // Legend visibility is owned by syncLegend(); leave it alone here.
    });
    guidedHighlightEls = [];
}

function applyGuidedHighlight(sel) {
    clearGuidedHighlight();
    if (!sel) return;
    const selectors = Array.isArray(sel)
        ? sel
        : String(sel).split(',').map(s => s.trim()).filter(Boolean);
    for (const s of selectors) {
        const el = document.querySelector(s);
        if (!el) continue;
        el.classList.add('guided-highlight');
        // Colour steps may mention the legend before/while it is shown.
        if (el.id === 'node-legend') el.classList.add('visible');
        guidedHighlightEls.push(el);
    }
}

function renderGuidedStep() {
    const steps  = getScenarios()[guidedScenario].steps;
    const s      = steps[guidedStep];
    const isLast = guidedStep === steps.length - 1;
    const Lui    = getLocale().ui;

    guidedIndicator.textContent = `${guidedStep + 1} / ${steps.length}`;
    guidedTitleEl.textContent   = s.title;
    guidedBodyEl .textContent   = s.body;
    guidedBodyEl.scrollTop      = 0;

    if (s.action) {
        guidedActionDiv.style.display = '';
        guidedActionTxt.textContent   = s.action;
    } else {
        guidedActionDiv.style.display = 'none';
    }

    guidedPrevBtn.disabled = (guidedStep === 0);

    if (s.trigger) {
        guidedSkipBtn.style.display = '';
        guidedSkipBtn.textContent   = Lui.guidedSkip;
        guidedNextBtn.style.display = 'none';
    } else {
        guidedSkipBtn.style.display = 'none';
        guidedNextBtn.style.display = '';
        guidedNextBtn.textContent   = isLast ? Lui.guidedDone : Lui.guidedNext;
    }

    stepCommentaryEl.style.display = 'none';
    applyGuidedHighlight(s.highlight);
}

function advanceGuided() {
    guidedStepFired = false;
    const steps = getScenarios()[guidedScenario].steps;
    if (guidedStep < steps.length - 1) {
        guidedStep++;
        renderGuidedStep();
    } else {
        closeGuided();
    }
}

function retreatGuided() {
    if (guidedStep <= 0) return;
    guidedStepFired = false;
    guidedStep--;
    renderGuidedStep();
}

function startGuidedPoll() {
    if (guidedPollTimer) return;
    guidedPollTimer = setInterval(() => {
        if (!guidedOpen) { clearInterval(guidedPollTimer); guidedPollTimer = null; return; }
        const s = getScenarios()[guidedScenario].steps[guidedStep];
        if (s.trigger && s.trigger()) advanceGuided();
    }, 300);
}

function openGuided() {
    if (!prepareLearnNetwork()) {
        alert(getLocale().ui.wasmNotReady);
        return;
    }
    if (tutOpen) closeTutorial();

    guidedStep      = 0;
    guidedOpen      = true;
    guidedStepFired = false;
    guidedPanel.classList.add('open');
    learnToggleBtn.classList.add('active');
    stepCommentaryEl.style.display = 'none';

    document.querySelectorAll('.guided-tab').forEach((tab, i) =>
        tab.classList.toggle('active', i === guidedScenario)
    );

    renderGuidedStep();
    startGuidedPoll();
}

function closeGuided() {
    guidedOpen = false;
    guidedPanel.classList.remove('open');
    learnToggleBtn.classList.remove('active');
    clearGuidedHighlight();
    stepCommentaryEl.style.display = 'none';
    if (guidedPollTimer) { clearInterval(guidedPollTimer); guidedPollTimer = null; }
    syncLegend();
}

learnToggleBtn.addEventListener('click', () => { if (guidedOpen) closeGuided(); else openGuided(); });
guidedCloseBtn.addEventListener('click', closeGuided);
guidedPrevBtn .addEventListener('click', retreatGuided);
guidedSkipBtn .addEventListener('click', advanceGuided);
guidedNextBtn .addEventListener('click', advanceGuided);

// ── Scenario tab switching ────────────────────────────────────────────────
document.querySelectorAll('.guided-tab').forEach((tab, i) => {
    tab.addEventListener('click', () => {
        if (i === guidedScenario) return;
        guidedScenario  = i;
        guidedStep      = 0;
        guidedStepFired = false;
        if (guidedOpen) prepareLearnNetwork();
        document.querySelectorAll('.guided-tab').forEach((t, j) =>
            t.classList.toggle('active', j === i)
        );
        if (guidedOpen) renderGuidedStep();
    });
});

// ── Step commentary / toast (feature #8) ─────────────────────────────────
let preStepState      = null;
let stepCommentaryTimer = null;
let stepToastTimer      = null;

function captureModuleState() {
    if (typeof Module._GetNumAgents !== 'function') return null;
    const n = Module._GetNumAgents();
    if (n === 0) return null;
    return {
        n,
        unique:    Module._GetNumUniqueAgents(),
        classes:   Module._GetNumAnonymityClasses(),
        rootGuess: typeof Module._GetRootGuess === 'function' ? Module._GetRootGuess() : -1,
    };
}

function buildCommentaryText(before, after) {
    if (!before || !after) return null;
    const C     = getLocale().commentary;
    const parts = [];
    const du    = after.unique - before.unique;
    if (du > 0) {
        parts.push(C.uniqueIdentified(du, after.unique, after.n));
    }
    if (after.classes !== before.classes && before.classes > 0) {
        parts.push(C.classesChanged(before.classes, after.classes));
    }
    if (after.rootGuess !== before.rootGuess && after.rootGuess > 0) {
        const was = before.rootGuess > 0 ? before.rootGuess : '?';
        parts.push(C.rootGuessChanged(was, after.rootGuess, after.rootGuess === after.n));
    }
    if (parts.length === 0) return C.noChange;
    return parts.join('\n');
}

function showStepCommentary(text) {
    stepCommentaryEl.textContent   = text;
    stepCommentaryEl.style.display = '';
    stepCommentaryEl.style.animation = 'none';
    void stepCommentaryEl.offsetHeight;
    stepCommentaryEl.style.animation = '';
}

function showStepToast(text) {
    stepToastEl.textContent = text.replace(/\n/g, '  ·  ');
    stepToastEl.classList.add('visible');
    clearTimeout(stepToastTimer);
    stepToastTimer = setTimeout(() => stepToastEl.classList.remove('visible'), 3500);
}

function onStepPressed() {
    if (guidedOpen) guidedStepFired = true;

    preStepState = captureModuleState();

    clearTimeout(stepCommentaryTimer);
    stepCommentaryTimer = setTimeout(() => {
        if (algoState === 0) return;
        const after = captureModuleState();
        const text  = buildCommentaryText(preStepState, after);
        if (!text) return;
        if (guidedOpen) {
            showStepCommentary(text);
        } else {
            showStepToast(text);
        }
    }, 700);
}

document.getElementById('btn-step').addEventListener('click', onStepPressed);
window.addEventListener('keydown', e => {
    if (guidedOpen && e.isTrusted && e.code === 'Space') onStepPressed();
    if (guidedOpen && e.code === 'Escape') { e.stopImmediatePropagation(); closeGuided(); }
}, true);

// ── DOM locale application ────────────────────────────────────────
function applyLocaleToDOM() {
    const L = getLocale().ui;
    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.dataset.i18n;
        if (L[key] !== undefined) el.textContent = L[key];
    });
    document.querySelectorAll('[data-i18n-html]').forEach(el => {
        const key = el.dataset.i18nHtml;
        if (L[key] !== undefined) el.innerHTML = L[key];
    });
}

// ── Theme toggle ───────────────────────────────────────────────────
const themeBtn = document.getElementById('btn-theme');

function getTheme() {
    return document.documentElement.getAttribute('data-theme') === 'light' ? 'light' : 'dark';
}

function syncThemeToWasm() {
    if (typeof Module._SetUiTheme === 'function') {
        Module._SetUiTheme(getTheme() === 'light' ? 1 : 0);
    }
}

function syncThemeBtn() {
    const light = getTheme() === 'light';
    themeBtn.classList.toggle('active', light);
    themeBtn.setAttribute('data-tip', light
        ? 'Switch to dark theme / ダークモード'
        : 'Switch to light theme / ライトモード');
}

function setTheme(theme) {
    const next = theme === 'light' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', next);
    try { localStorage.setItem('theme', next); } catch (_) {}
    syncThemeBtn();
    syncThemeToWasm();
}

themeBtn.addEventListener('click', () => {
    setTheme(getTheme() === 'light' ? 'dark' : 'light');
});

syncThemeBtn();
syncThemeToWasm();

// ── Language toggle ────────────────────────────────────────────────
const langBtn = document.getElementById('btn-lang');

function syncLangBtn() {
    const lang = getLang();
    langBtn.textContent = lang === 'ja' ? 'JA' : 'EN';
    langBtn.classList.toggle('active', lang === 'ja');
    document.documentElement.lang = lang;
}

langBtn.addEventListener('click', () => {
    setLocale(getLang() === 'ja' ? 'en' : 'ja');
    syncLangBtn();
    applyLocaleToDOM();
    // Re-render any open panels immediately
    if (tutOpen)    renderTutStep();
    if (guidedOpen) renderGuidedStep();
    syncBanner();
});

syncLangBtn();
applyLocaleToDOM();