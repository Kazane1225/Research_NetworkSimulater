// ── i18n.js — Locale registry (thin core) ────────────────────────────────
// Locale data lives in js/locales/en.js and js/locales/ja.js.
// This file just wires them together and exposes three helpers.

const LOCALES = { en: LOCALE_EN, ja: LOCALE_JA };

let _lang = (function () {
    try { return localStorage.getItem('locale') || 'en'; } catch (_) { return 'en'; }
}());

function setLocale(lang) {
    if (LOCALES[lang]) {
        _lang = lang;
        try { localStorage.setItem('locale', _lang); } catch (_) {}
    }
}

function getLang()   { return _lang; }
function getLocale() { return LOCALES[_lang] || LOCALES['en']; }