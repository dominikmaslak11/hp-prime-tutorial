// Integration test run inside VS Code (see test/run.js).
const vscode = require('vscode');
const assert = require('assert');

const SOURCE = [
  'EXPORT DEMO(x)',
  'BEGIN',
  '  LOCAL wynik;',
  '  IF x=1 THEN wynik := 2; END;',
  '  L1 := 5;',
  '  RETURN TEXTOUT_P("a", 1, 2);',
  'END;',
  '',
].join('\n');

async function waitFor(fn, timeoutMs = 20000) {
  const start = Date.now();
  for (;;) {
    const v = await fn();
    if (v) return v;
    if (Date.now() - start > timeoutMs) throw new Error('timeout');
    await new Promise((r) => setTimeout(r, 200));
  }
}

async function run() {
  const results = [];
  const test = async (name, fn) => {
    try { await fn(); results.push(`OK   ${name}`); }
    catch (e) { results.push(`FAIL ${name}: ${e.message}`); }
  };

  const doc = await vscode.workspace.openTextDocument({ language: 'hpppl', content: SOURCE });
  await vscode.window.showTextDocument(doc);
  const ext = vscode.extensions.getExtension('dominikmaslak11.hp-prime-ppl');

  await test('rozszerzenie aktywne', async () => {
    await waitFor(() => ext && ext.isActive);
  });

  await test('diagnostyka na żywo', async () => {
    const diags = await waitFor(() => {
      const d = vscode.languages.getDiagnostics(doc.uri);
      return d.length >= 2 ? d : undefined;
    });
    const msgs = diags.map((d) => d.message).join(' | ');
    assert.ok(msgs.includes("'=='"), msgs);
    assert.ok(msgs.includes('L1 przechowuje'), msgs);
  });

  await test('podpowiedzi', async () => {
    const list = await vscode.commands.executeCommand('vscode.executeCompletionItemProvider', doc.uri, new vscode.Position(4, 2));
    const labels = list.items.map((i) => (typeof i.label === 'string' ? i.label : i.label.label));
    assert.ok(labels.includes('wynik'), 'brak zmiennej lokalnej');
    assert.ok(labels.includes('MSGBOX'), 'brak komendy');
  });

  await test('opis po najechaniu', async () => {
    const hovers = await vscode.commands.executeCommand('vscode.executeHoverProvider', doc.uri, new vscode.Position(5, 12));
    const text = hovers.map((h) => h.contents.map((c) => c.value || c).join('')).join('');
    assert.ok(text.includes('TEXTOUT_P('), text);
  });

  await test('podpowiedź parametrów', async () => {
    const sig = await vscode.commands.executeCommand('vscode.executeSignatureHelpProvider', doc.uri, new vscode.Position(5, 27));
    assert.ok(sig && sig.signatures[0].label.startsWith('TEXTOUT_P('));
    assert.strictEqual(sig.activeParameter, 2);
  });

  await test('konspekt', async () => {
    const symbols = await vscode.commands.executeCommand('vscode.executeDocumentSymbolProvider', doc.uri);
    assert.ok(symbols.some((s) => s.name === 'DEMO(x)'));
  });

  await test('formatowanie', async () => {
    const messy = await vscode.workspace.openTextDocument({ language: 'hpppl', content: 'export f()\nbegin\nreturn 1;\nend;' });
    await new Promise((r) => setTimeout(r, 1000));
    const edits = await vscode.commands.executeCommand('vscode.executeFormatDocumentProvider', messy.uri, { tabSize: 2, insertSpaces: true });
    assert.ok(edits && edits.length > 0, 'brak zmian');
    const we = new vscode.WorkspaceEdit();
    we.set(messy.uri, edits);
    await vscode.workspace.applyEdit(we);
    assert.strictEqual(messy.getText(), 'EXPORT f()\nBEGIN\n  RETURN 1;\nEND;');
  });

  await test('kolorowanie (gramatyka zarejestrowana)', async () => {
    assert.strictEqual(doc.languageId, 'hpppl');
  });

  await test('polecenia zarejestrowane', async () => {
    const cmds = await vscode.commands.getCommands(true);
    for (const c of ['hpppl.run', 'hpppl.check', 'hpppl.copyForCalculator', 'hpppl.setupAi', 'hpppl.commandReference']) assert.ok(cmds.includes(c), c);
  });

  await test('kopiowanie do schowka', async () => {
    await vscode.commands.executeCommand('hpppl.copyForCalculator');
    const clip = await vscode.env.clipboard.readText();
    assert.ok(clip.startsWith('EXPORT DEMO(x)'), clip.slice(0, 40));
  });

  const report = results.join('\n');
  require('fs').writeFileSync(process.env.PPL_TEST_REPORT, report, 'utf8');
  if (results.some((r) => r.startsWith('FAIL'))) throw new Error(report);
}

module.exports = { run };
