// Runs test/suite.js inside VS Code with an isolated profile.
// Usage: node test/run.js [path-to-Code.exe]
const path = require('path');
const os = require('os');
const fs = require('fs');
const { runTests } = require('@vscode/test-electron');

(async () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'hpppl-test-'));
  const report = path.join(tmp, 'report.txt');
  process.env.PPL_TEST_REPORT = report;
  const exe = process.argv[2] || path.join(process.env.LOCALAPPDATA || '', 'Programs', 'Microsoft VS Code', 'Code.exe');
  try {
    await runTests({
      vscodeExecutablePath: fs.existsSync(exe) ? exe : undefined,
      extensionDevelopmentPath: path.resolve(__dirname, '..'),
      extensionTestsPath: path.resolve(__dirname, 'suite.js'),
      launchArgs: ['--disable-extensions', '--user-data-dir', path.join(tmp, 'user'), '--extensions-dir', path.join(tmp, 'ext'), '--skip-welcome', '--skip-release-notes'],
      extensionTestsEnv: { PPL_TEST_REPORT: report },
    });
  } catch (e) {
    console.error('Testy nie przeszły.');
  }
  if (fs.existsSync(report)) console.log(fs.readFileSync(report, 'utf8'));
  else { console.error('Brak raportu z testów.'); process.exit(1); }
})();
