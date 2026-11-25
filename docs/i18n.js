// English and Swedish for the install page and the emulator.
//
// The English text is in the page as usual. An element with Swedish has it right next to it, in a data-sv
// attribute (its HTML) or data-sv-aria (its aria-label), so a translation can't end up in the wrong place.
// Texts made by a script are written as T("English", "Svenska").
//
// The language is chosen like on the lamp's own page: ?lang=sv or ?lang=en in the address, otherwise the
// language last chosen with the button on the page, otherwise the browser's language: Swedish if it asks
// for it, English for everything else.
"use strict";
const I18N = (() => {
  const KEY = "elpris-led-lang";
  let lang = "en";
  const listeners = [];

  function pick() {
    const q = new URLSearchParams(location.search).get("lang");
    if (q === "sv" || q === "en") return q;
    try {
      const saved = localStorage.getItem(KEY);
      if (saved === "sv" || saved === "en") return saved;
    } catch (e) { /* storage can be off; then the browser's language decides */ }
    for (const l of navigator.languages || [navigator.language || ""]) {
      const primary = String(l).toLowerCase().split("-")[0];
      if (primary === "sv") return "sv";
      if (primary === "en") return "en";
    }
    return "en";
  }

  function apply(l) {
    lang = l === "sv" ? "sv" : "en";
    document.documentElement.lang = lang;
    document.querySelectorAll("[data-sv]").forEach(el => {
      if (el.dataset.en === undefined) el.dataset.en = el.innerHTML;
      el.innerHTML = lang === "sv" ? el.dataset.sv : el.dataset.en;
    });
    document.querySelectorAll("[data-sv-aria]").forEach(el => {
      if (el.dataset.enAria === undefined) el.dataset.enAria = el.getAttribute("aria-label") || "";
      el.setAttribute("aria-label", lang === "sv" ? el.dataset.svAria : el.dataset.enAria);
    });
    const btn = document.getElementById("langBtn");
    if (btn) {
      btn.textContent = lang === "sv" ? "English" : "Svenska";
      btn.lang = lang === "sv" ? "en" : "sv";
    }
    listeners.forEach(fn => fn(lang));
  }

  // The button shows the other language; choosing it is remembered in this browser
  function toggle() {
    const next = lang === "sv" ? "en" : "sv";
    try { localStorage.setItem(KEY, next); } catch (e) { /* not remembered, but still switched */ }
    apply(next);
  }

  return {
    get lang() { return lang; },
    pick, apply, toggle,
    onChange: fn => listeners.push(fn),
    T: (en, sv) => (lang === "sv" ? sv : en),
  };
})();
const T = I18N.T;
