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
