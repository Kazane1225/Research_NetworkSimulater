// ── Canvas sizing ─────────────────────────────────────────────
const canvas     = document.getElementById('canvas');
const canvasWrap = document.getElementById('canvas-wrap');

function resizeCanvas() {
    canvas.width  = canvasWrap.clientWidth;
    canvas.height = canvasWrap.clientHeight;
}
window.addEventListener('resize', resizeCanvas);
resizeCanvas();

// Emscripten Module entry point (must be set before index.js loads)
var Module = { canvas };

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

function syncAlgoUI() {
    algoButtons.forEach((b, i) => b.classList.toggle('active', i === algoState));
    stepBtn.disabled = (algoState === 0);
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

function openHelp()  { helpModal.classList.add('open');    }
function closeHelp() { helpModal.classList.remove('open'); }

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
