// HP Prime PPL — rozszerzenie VS Code.
// Cała wiedza o języku jest w ppl.exe (serwer LSP + serwer MCP); tutaj tylko klient i polecenia.
'use strict';

const vscode = require('vscode');
const path = require('path');
const fs = require('fs');
const { execFile } = require('child_process');
const { LanguageClient, TransportKind } = require('vscode-languageclient/node');

/** @type {LanguageClient | undefined} */
let client;
/** @type {vscode.ExtensionContext} */
let ctx;
let output;

function serverPath() {
  const configured = vscode.workspace.getConfiguration('hpppl').get('serverPath');
  if (configured && fs.existsSync(configured)) return configured;
  const exe = process.platform === 'win32' ? 'ppl.exe' : 'ppl';
  return path.join(ctx.extensionPath, 'bin', exe);
}

// Stała ścieżka ppl.exe dla zewnętrznych agentów (Claude Code, Continue…):
// kopia w globalStorage nie zmienia się przy aktualizacji rozszerzenia.
function stableServerPath() {
  const src = serverPath();
  const dir = ctx.globalStorageUri.fsPath;
  fs.mkdirSync(dir, { recursive: true });
  const dst = path.join(dir, path.basename(src));
  try {
    const a = fs.statSync(src);
    const b = fs.existsSync(dst) ? fs.statSync(dst) : undefined;
    if (!b || a.size !== b.size || a.mtimeMs > b.mtimeMs) fs.copyFileSync(src, dst);
  } catch (e) {
    output.appendLine(`Nie udało się skopiować ppl.exe: ${e}`);
    return src;
  }
  return dst;
}

function runPpl(args, input) {
  return new Promise((resolve, reject) => {
    const child = execFile(serverPath(), args, { encoding: 'utf8', maxBuffer: 16 * 1024 * 1024 }, (err, stdout, stderr) => {
      if (err && err.code !== 1) reject(new Error(stderr || err.message));
      else resolve(stdout);
    });
    if (input !== undefined) child.stdin.end(input);
  });
}

async function startClient() {
  const exe = serverPath();
  if (!fs.existsSync(exe)) {
    vscode.window.showErrorMessage(`HP PPL: nie znaleziono serwera języka (${exe}). Ustaw hpppl.serverPath.`);
    return;
  }
  const serverOptions = {
    run: { command: exe, args: ['lsp'], transport: TransportKind.stdio },
    debug: { command: exe, args: ['lsp'], transport: TransportKind.stdio },
  };
  const clientOptions = {
    documentSelector: [{ language: 'hpppl' }],
    outputChannel: output,
    traceOutputChannel: output,
  };
  client = new LanguageClient('hpppl', 'HP PPL', serverOptions, clientOptions);
  await client.start();
}

function activeDocument() {
  const ed = vscode.window.activeTextEditor;
  if (!ed || ed.document.languageId !== 'hpppl') {
    vscode.window.showWarningMessage('Otwórz plik z programem HP PPL (.hpppl).');
    return undefined;
  }
  return ed;
}

async function applyServerEdits(editor, edits) {
  if (!edits || edits.length === 0) return false;
  const we = new vscode.WorkspaceEdit();
  for (const e of edits) {
    const r = e.range;
    we.replace(editor.document.uri,
      new vscode.Range(r.start.line, r.start.character, r.end.line, r.end.character), e.newText);
  }
  return vscode.workspace.applyEdit(we);
}

async function convert(mode) {
  const ed = activeDocument();
  if (!ed || !client) return;
  const edits = await client.sendRequest('ppl/convert', { textDocument: { uri: ed.document.uri.toString() }, mode });
  const changed = await applyServerEdits(ed, edits);
  vscode.window.setStatusBarMessage(changed ? 'HP PPL: zamieniono operatory' : 'HP PPL: nic do zamiany', 3000);
}

function checkCommand() {
  const ed = activeDocument();
  if (!ed) return;
  const diags = vscode.languages.getDiagnostics(ed.document.uri);
  const errors = diags.filter((d) => d.severity === vscode.DiagnosticSeverity.Error).length;
  const warnings = diags.filter((d) => d.severity === vscode.DiagnosticSeverity.Warning).length;
  if (errors === 0 && warnings === 0) {
    vscode.window.showInformationMessage('HP PPL: brak błędów i ostrzeżeń ✔');
  } else {
    vscode.commands.executeCommand('workbench.actions.view.problems');
    vscode.window.showWarningMessage(`HP PPL: ${errors} błąd(ów), ${warnings} ostrzeżeń — szczegóły w panelu Problemy.`);
  }
}

async function copyForCalculator() {
  const ed = activeDocument();
  if (!ed) return;
  let text = ed.document.getText();
  if (vscode.workspace.getConfiguration('hpppl').get('copy.useCalculatorSymbols') && client) {
    const edits = await client.sendRequest('ppl/convert', { textDocument: { uri: ed.document.uri.toString() }, mode: 'symbols' });
    if (edits && edits.length) text = edits[0].newText;
  }
  text = text.replace(/\r\n/g, '\n');
  await vscode.env.clipboard.writeText(text);
  const errors = vscode.languages.getDiagnostics(ed.document.uri)
    .filter((d) => d.severity === vscode.DiagnosticSeverity.Error).length;
  const msg = 'Skopiowano program. W Connectivity Kit: Programs › New, wklej (Ctrl+V) i zapisz.';
  if (errors) vscode.window.showWarningMessage(`${msg} Uwaga: program ma ${errors} błąd(ów).`);
  else vscode.window.showInformationMessage(msg);
}

let commandDb;
function loadCommandDb() {
  if (!commandDb) {
    const file = path.join(ctx.extensionPath, 'data', 'commands.json');
    commandDb = JSON.parse(fs.readFileSync(file, 'utf8'));
  }
  return commandDb;
}

function commandMarkdown(c) {
  let md = `# ${c.name}\n\n\`\`\`ppl\n${c.syntax}\n\`\`\`\n\n${c.desc}\n`;
  if (c.example) md += `\n**Przykład:**\n\n\`\`\`ppl\n${c.example}\n\`\`\`\n`;
  md += `\n*${c.cat}*`;
  if (c.ch) md += ` · [rozdział kursu](https://github.com/dominikmaslak11/hp-prime-tutorial/blob/main/rozdzialy/${c.ch})`;
  return md;
}

async function showMarkdown(content) {
  const doc = await vscode.workspace.openTextDocument({ language: 'markdown', content });
  await vscode.commands.executeCommand('markdown.showPreview', doc.uri);
}

async function commandReference() {
  const db = loadCommandDb();
  const ed = vscode.window.activeTextEditor;
  let word = '';
  if (ed) {
    const range = ed.document.getWordRangeAtPosition(ed.selection.active);
    if (range) word = ed.document.getText(range).replace(/^CAS\./, '');
  }
  const items = db.commands.map((c) => ({ label: c.name, description: c.syntax, detail: c.desc, cmd: c }));
  const pick = vscode.window.createQuickPick();
  pick.items = items;
  pick.matchOnDescription = true;
  pick.matchOnDetail = true;
  pick.placeholder = 'Wpisz nazwę komendy PPL lub słowo z opisu (np. klawisz, lista, tekst)';
  pick.value = word;
  pick.onDidAccept(async () => {
    const sel = pick.selectedItems[0];
    pick.hide();
    if (sel) await showMarkdown(commandMarkdown(sel.cmd));
  });
  pick.show();
}

async function newProgram() {
  const name = await vscode.window.showInputBox({
    prompt: 'Nazwa programu (litery, cyfry, _; pierwsza litera)',
    validateInput: (v) => (/^[A-Za-z][A-Za-z0-9_]*$/.test(v) ? undefined : 'Niepoprawna nazwa programu'),
  });
  if (!name) return;
  const content = `// ${name} — opis programu\n\nEXPORT ${name}()\nBEGIN\n  \nEND;\n`;
  const doc = await vscode.workspace.openTextDocument({ language: 'hpppl', content });
  const ed = await vscode.window.showTextDocument(doc);
  ed.selection = new vscode.Selection(4, 2, 4, 2);
}

// ------------------------------------------------------------------ AI

async function languageGuide() {
  return runPpl(['guide']);
}

function writeIfAbsentOrAppend(file, marker, content) {
  fs.mkdirSync(path.dirname(file), { recursive: true });
  if (!fs.existsSync(file)) {
    fs.writeFileSync(file, content, 'utf8');
    return 'utworzono';
  }
  const existing = fs.readFileSync(file, 'utf8');
  if (existing.includes(marker)) return 'bez zmian (sekcja już istnieje)';
  fs.writeFileSync(file, existing.replace(/\s*$/, '\n\n') + content, 'utf8');
  return 'dopisano sekcję';
}

function mergeJson(file, update) {
  fs.mkdirSync(path.dirname(file), { recursive: true });
  let obj = {};
  if (fs.existsSync(file)) {
    try { obj = JSON.parse(fs.readFileSync(file, 'utf8')); } catch { return 'pominięto (plik nie jest poprawnym JSON)'; }
  }
  update(obj);
  fs.writeFileSync(file, JSON.stringify(obj, null, 2) + '\n', 'utf8');
  return 'zapisano';
}

async function setupAi() {
  const folder = vscode.workspace.workspaceFolders && vscode.workspace.workspaceFolders[0];
  if (!folder) {
    vscode.window.showWarningMessage('Otwórz folder projektu (Plik › Otwórz folder), aby zapisać konfigurację AI.');
    return;
  }
  const root = folder.uri.fsPath;
  const choices = await vscode.window.showQuickPick([
    { label: 'GitHub Copilot', description: '.github/copilot-instructions.md', id: 'copilot', picked: true },
    { label: 'Claude Code', description: 'CLAUDE.md + .mcp.json', id: 'claude', picked: true },
    { label: 'Continue (Claude, GPT, Gemini, DeepSeek…)', description: '.continue/rules + .continue/mcpServers', id: 'continue', picked: true },
    { label: 'Gemini CLI / inne agenty', description: 'GEMINI.md + AGENTS.md', id: 'agents', picked: false },
    { label: 'Serwer MCP w VS Code (plik)', description: '.vscode/mcp.json — gdy nie używasz wbudowanej rejestracji', id: 'vscodemcp', picked: false },
  ], { canPickMany: true, title: 'Dla których asystentów AI przygotować projekt HP PPL?' });
  if (!choices || choices.length === 0) return;

  const exe = stableServerPath();
  const guide = await languageGuide();
  const marker = '<!-- hp-prime-ppl -->';
  const instructions = `${marker}\n# Programy HP PPL (HP Prime) w tym projekcie\n\n` +
    'Pliki `*.hpppl` to programy w języku HP PPL dla kalkulatora HP Prime.\n' +
    '- Po każdej zmianie programu sprawdź go narzędziem MCP `ppl_check_file` (albo `ppl_validate`) i popraw wszystkie błędy.\n' +
    `- Bez MCP możesz uruchomić: \`"${exe}" check PLIK.hpppl\`.\n` +
    '- Nie wymyślaj komend — sprawdzaj je narzędziem `ppl_command_help` / `ppl_search_commands`.\n' +
    '- Odpowiadaj po polsku.\n\n' + guide;
  const report = [];
  const mcpEntry = { command: exe, args: ['mcp'] };

  for (const c of choices) {
    if (c.id === 'copilot') {
      report.push('Copilot: ' + writeIfAbsentOrAppend(path.join(root, '.github', 'copilot-instructions.md'), marker, instructions));
    } else if (c.id === 'claude') {
      report.push('CLAUDE.md: ' + writeIfAbsentOrAppend(path.join(root, 'CLAUDE.md'), marker, instructions));
      report.push('.mcp.json: ' + mergeJson(path.join(root, '.mcp.json'), (o) => {
        o.mcpServers = o.mcpServers || {};
        o.mcpServers['hp-prime-ppl'] = { type: 'stdio', ...mcpEntry };
      }));
    } else if (c.id === 'continue') {
      report.push('Continue rules: ' + writeIfAbsentOrAppend(path.join(root, '.continue', 'rules', 'hp-ppl.md'), marker,
        `---\nname: HP PPL\nglobs: "**/*.hpppl"\n---\n\n${instructions}`));
      const yaml = `name: HP Prime PPL\nversion: 1.0.0\nschema: v1\nmcpServers:\n  - name: hp-prime-ppl\n    command: ${JSON.stringify(exe)}\n    args:\n      - mcp\n`;
      const f = path.join(root, '.continue', 'mcpServers', 'hp-prime-ppl.yaml');
      fs.mkdirSync(path.dirname(f), { recursive: true });
      fs.writeFileSync(f, yaml, 'utf8');
      report.push('Continue MCP: zapisano');
    } else if (c.id === 'agents') {
      report.push('GEMINI.md: ' + writeIfAbsentOrAppend(path.join(root, 'GEMINI.md'), marker, instructions));
      report.push('AGENTS.md: ' + writeIfAbsentOrAppend(path.join(root, 'AGENTS.md'), marker, instructions));
    } else if (c.id === 'vscodemcp') {
      report.push('.vscode/mcp.json: ' + mergeJson(path.join(root, '.vscode', 'mcp.json'), (o) => {
        o.servers = o.servers || {};
        o.servers['hp-prime-ppl'] = { type: 'stdio', ...mcpEntry };
      }));
    }
  }
  output.appendLine('Konfiguracja AI:\n  ' + report.join('\n  '));
  vscode.window.showInformationMessage('HP PPL: konfiguracja AI zapisana. ' + report.join(' · '));
}

function registerMcp(context) {
  const lm = vscode.lm;
  if (!lm || typeof lm.registerMcpServerDefinitionProvider !== 'function' || !vscode.McpStdioServerDefinition) {
    output.appendLine('Ta wersja VS Code nie obsługuje rejestracji serwerów MCP przez rozszerzenia (wymagane 1.101+).');
    return;
  }
  const changed = new vscode.EventEmitter();
  context.subscriptions.push(changed);
  context.subscriptions.push(lm.registerMcpServerDefinitionProvider('hpppl.mcp', {
    onDidChangeMcpServerDefinitions: changed.event,
    provideMcpServerDefinitions: async () => {
      if (!vscode.workspace.getConfiguration('hpppl').get('mcp.enabled')) return [];
      return [new vscode.McpStdioServerDefinition('HP Prime PPL', serverPath(), ['mcp'], {}, '1.0.0')];
    },
  }));
  context.subscriptions.push(vscode.workspace.onDidChangeConfiguration((e) => {
    if (e.affectsConfiguration('hpppl.mcp.enabled')) changed.fire();
  }));
}

// ------------------------------------------------------------------ activation

async function activate(context) {
  ctx = context;
  output = vscode.window.createOutputChannel('HP PPL');
  context.subscriptions.push(output);

  const reg = (id, fn) => context.subscriptions.push(vscode.commands.registerCommand(id, fn));
  reg('hpppl.check', checkCommand);
  reg('hpppl.format', () => vscode.commands.executeCommand('editor.action.formatDocument'));
  reg('hpppl.toAscii', () => convert('ascii'));
  reg('hpppl.toSymbols', () => convert('symbols'));
  reg('hpppl.copyForCalculator', copyForCalculator);
  reg('hpppl.commandReference', commandReference);
  reg('hpppl.newProgram', newProgram);
  reg('hpppl.setupAi', setupAi);
  reg('hpppl.showLanguageGuide', async () => showMarkdown(await languageGuide()));
  reg('hpppl.restartServer', async () => {
    if (client) await client.stop();
    await startClient();
    vscode.window.setStatusBarMessage('HP PPL: serwer uruchomiony ponownie', 3000);
  });

  registerMcp(context);
  await startClient();
}

async function deactivate() {
  if (client) await client.stop();
}

module.exports = { activate, deactivate };
