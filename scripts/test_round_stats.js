/**
 * Verify distinguishable-class stats track the viewed round (currentRound),
 * not always the final history-tree level.
 *
 * Usage: node scripts/test_round_stats.js [port]
 */
const http = require("http");
const path = require("path");
const { execSync } = require("child_process");

const PORT = parseInt(process.argv[2], 10) || 8020;
const DIR = path.resolve(__dirname, "../website");

function waitForPort(port, ms = 30000) {
  const start = Date.now();
  return new Promise((resolve, reject) => {
    (function poll() {
      const req = http.get(`http://127.0.0.1:${port}/`, (res) => {
        res.resume();
        resolve();
      });
      req.on("error", () => {
        if (Date.now() - start > ms) reject(new Error(`port ${port} timeout`));
        else setTimeout(poll, 200);
      });
    })();
  });
}

async function loadPuppeteer() {
  const root = path.resolve(__dirname, "..");
  try {
    return require("puppeteer");
  } catch {
    execSync("npm install puppeteer --no-save", { cwd: root, stdio: "inherit" });
    return require("puppeteer");
  }
}

function assert(cond, msg) {
  if (!cond) throw new Error(msg);
}

async function runTests(page) {
  return page.evaluate(() => {
    const failures = [];

    const check = (cond, msg) => {
      if (!cond) failures.push(msg);
    };

    const waitModule = () =>
      new Promise((resolve, reject) => {
        const t0 = Date.now();
        (function poll() {
          if (
            typeof Module !== "undefined" &&
            Module.calledRun &&
            typeof Module._TutorialLoadNetwork === "function" &&
            typeof Module._TestGotoRound === "function"
          ) {
            resolve(Module);
            return;
          }
          if (Date.now() - t0 > 60000) reject(new Error("Module timeout"));
          else setTimeout(poll, 100);
        })();
      });

    return waitModule().then((M) => {
      M._TutorialLoadNetwork();

      const n = M._GetNumAgents();
      const rounds = M._GetNumRounds();
      const finalLevel = M._GetAuxLevelCount() - 1;

      check(n === 6, `tutorial agents: expected 6, got ${n}`);
      check(rounds === 5, `tutorial rounds: expected 5, got ${rounds}`);
      check(finalLevel >= 2, `aux levels: expected >= 3, got ${M._GetAuxLevelCount()}`);

      // currentRound = -1 → no stats
      M._TestGotoRound(-1);
      check(M._GetCurrentRound() === -1, "goto -1: current round");
      check(M._GetNumAnonymityClasses() === 0, "goto -1: classes should be 0");
      check(M._GetNumUniqueAgents() === 0, "goto -1: unique should be 0");

      const perRound = [];
      for (let r = 0; r < rounds; r++) {
        M._TestGotoRound(r);
        const level = r + 2;
        const classes = M._GetNumAnonymityClasses();
        const unique = M._GetNumUniqueAgents();
        const width = M._GetAuxLevelWidth(level);
        const levelUnique = M._GetAuxLevelUniqueCount(level);
        const anonSum = M._GetAuxLevelAnonymitySum(level);

        check(M._GetCurrentRound() === r, `round ${r}: currentRound`);
        check(classes === width, `round ${r}: classes ${classes} vs level width ${width}`);
        check(unique === levelUnique, `round ${r}: unique ${unique} vs level unique ${levelUnique}`);
        check(unique <= classes, `round ${r}: unique <= classes`);
        check(anonSum === n, `round ${r}: anonymity sum ${anonSum} vs n=${n}`);

        perRound.push({ r, classes, unique, level, width });
      }

      // Final viewed round must differ from always-using-final-level behavior when early rounds differ.
      M._TestGotoRound(0);
      const round0Classes = M._GetNumAnonymityClasses();
      const finalClasses = M._GetAuxLevelWidth(finalLevel);
      check(
        round0Classes !== finalClasses,
        `round 0 classes (${round0Classes}) should differ from final (${finalClasses})`
      );

      // Tutorial round 0: L + {1,5} + {2,3,4} (ring symmetry).
      check(round0Classes === 3, `round 0: expected 3 classes, got ${round0Classes}`);
      M._TestGotoRound(0);
      check(M._GetNumUniqueAgents() === 1, `round 0: expected 1 unique (leader), got ${M._GetNumUniqueAgents()}`);

      // Class count is non-decreasing over rounds (refinement only).
      for (let i = 1; i < perRound.length; i++) {
        check(
          perRound[i].classes >= perRound[i - 1].classes,
          `classes monotone: round ${perRound[i - 1].r} (${perRound[i - 1].classes}) -> round ${perRound[i].r} (${perRound[i].classes})`
        );
      }

      // Last round: fully distinguished in tutorial.
      const last = perRound[perRound.length - 1];
      check(last.classes === 6, `final round: expected 6 classes, got ${last.classes}`);
      check(last.unique === 6, `final round: expected 6 unique, got ${last.unique}`);

      return { failures, perRound, n, rounds, round0Classes, finalClasses };
    });
  });
}

async function main() {
  const server = http.createServer((req, res) => {
    let url = decodeURIComponent(req.url.split("?")[0]);
    if (url === "/") url = "/index.html";
    const filePath = path.join(DIR, url);
    if (!filePath.startsWith(DIR)) {
      res.writeHead(403);
      res.end("Forbidden");
      return;
    }
    const fs = require("fs");
    fs.readFile(filePath, (err, data) => {
      if (err) {
        res.writeHead(404);
        res.end("Not Found");
        return;
      }
      const ext = path.extname(filePath).toLowerCase();
      const mime = {
        ".html": "text/html",
        ".js": "application/javascript",
        ".wasm": "application/wasm",
        ".data": "application/octet-stream",
        ".css": "text/css",
      }[ext] || "application/octet-stream";
      res.writeHead(200, {
        "Content-Type": mime,
        "Cross-Origin-Opener-Policy": "same-origin",
        "Cross-Origin-Embedder-Policy": "require-corp",
      });
      res.end(data);
    });
  });

  await new Promise((resolve) => server.listen(PORT, resolve));
  console.log(`Test server http://127.0.0.1:${PORT}/`);

  try {
    await waitForPort(PORT);
    const puppeteer = await loadPuppeteer();
    const browser = await puppeteer.launch({ headless: true });
    const page = await browser.newPage();
    await page.goto(`http://127.0.0.1:${PORT}/`, { waitUntil: "networkidle0" });
    const result = await runTests(page);
    await browser.close();

    if (result.failures.length) {
      console.error("FAILURES:");
      for (const f of result.failures) console.error("  -", f);
      console.error("perRound:", result.perRound);
      process.exit(1);
    }

    console.log("All round-stats tests passed.");
    console.log("perRound:", result.perRound);
    console.log(`round0=${result.round0Classes}, final=${result.finalClasses}, n=${result.n}`);
  } finally {
    server.close();
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
