function setStatus(text, kind) {
  const status = document.getElementById('status');
  status.textContent = text;
  status.dataset.kind = kind || '';
}

function applyEmbedMode() {
  const params = new URLSearchParams(window.location.search);
  const embed = params.get('embed');
  if (embed === '1' || embed === 'true') {
    document.body.classList.add('embedded');
  }
}

const ALLOWED_EXTENSIONS = new Set(['.ui', '.prc', '.adl', '.edl']);

function looksLikeAbsolutePath(input) {
  const p = input.trim();
  if (p === '') return false;

  if (p.startsWith('/')) return true;

  if (p.startsWith('\\') || p.startsWith('//')) return true;
  if (/^[a-zA-Z]:[\\/]/.test(p)) return true;
  if (p.startsWith('\\?\\')) return true;

  if (p.startsWith('\\')) return true;

  return false;
}

function normalizeRelativePath(input) {
  const raw = input.trim();
  if (raw === '') {
    return { ok: false, error: 'File path is required.' };
  }

  if (looksLikeAbsolutePath(raw)) {
    return { ok: false, error: 'File path must be relative (no absolute paths).' };
  }

  let path = raw.replace(/\\/g, '/');

  if (path.includes('../') || path === '..' || path.startsWith('../') || path.endsWith('/..') || path.includes('/../')) {
    return { ok: false, error: 'Path traversal (..) is not allowed.' };
  }

  const segments = path.split('/');
  for (const seg of segments) {
    if (seg === '..') {
      return { ok: false, error: 'Path traversal (..) is not allowed.' };
    }
  }

  while (path.startsWith('./')) path = path.slice(2);
  if (path === '.') {
    return { ok: false, error: 'File path must point to a file.' };
  }

  path = path.replace(/\/+/g, '/');

  const fileName = path.slice(path.lastIndexOf('/') + 1);
  if (fileName.lastIndexOf('.') <= 0) {
    path = `${path}.ui`;
  }

  if (!/^[A-Za-z0-9._\-/ ]+$/.test(path)) {
    return { ok: false, error: 'File path contains unsupported characters.' };
  }

  const lower = path.toLowerCase();
  const dot = lower.lastIndexOf('.');
  if (dot === -1) {
    return { ok: false, error: 'File path must include an extension.' };
  }
  const ext = lower.slice(dot);
  if (!ALLOWED_EXTENSIONS.has(ext)) {
    return { ok: false, error: `Unsupported extension: ${ext}` };
  }

  if (path.endsWith('/')) {
    return { ok: false, error: 'File path must point to a file, not a directory.' };
  }

  return { ok: true, value: path };
}

function normalizeMacros(input) {
  const raw = (input || '').trim();
  if (raw === '') return { ok: true, value: '' };

  const parts = raw
    .split(/[;\n\r]+/)
    .map((p) => p.trim())
    .filter(Boolean);

  const out = [];
  for (const part of parts) {
    const idx = part.indexOf('=');
    if (idx <= 0 || idx === part.length - 1) {
      return { ok: false, error: `Invalid macro: ${part}` };
    }
    const key = part.slice(0, idx).trim();
    const value = part.slice(idx + 1).trim();

    if (!/^[A-Za-z0-9_]+$/.test(key)) {
      return { ok: false, error: `Invalid macro key: ${key}` };
    }
    if (value === '') {
      return { ok: false, error: `Empty macro value for: ${key}` };
    }
    if (/[&,]/.test(value)) {
      return { ok: false, error: `Macro value must not contain ',' or '&': ${key}` };
    }

    out.push(`${key}:${value}`);
  }

  return { ok: true, value: out.join(',') };
}

function buildUrl(pathValue, macrosValue) {
  const base = new URL('.', window.location.href).pathname;
  let url = `${base}?path=${pathValue}`;
  if (macrosValue) url += `&macros=${macrosValue}`;
  return url;
}

function update() {
  const filePathEl = document.getElementById('filePath');
  const macrosEl = document.getElementById('macros');
  const outEl = document.getElementById('generatedUrl');
  const copyBtn = document.getElementById('copyBtn');
  const openBtn = document.getElementById('openBtn');

  const pathRes = normalizeRelativePath(filePathEl.value);
  if (!pathRes.ok) {
    outEl.value = '';
    copyBtn.disabled = true;
    openBtn.disabled = true;
    setStatus(pathRes.error, 'error');
    return;
  }

  const macroRes = normalizeMacros(macrosEl.value);
  if (!macroRes.ok) {
    outEl.value = '';
    copyBtn.disabled = true;
    openBtn.disabled = true;
    setStatus(macroRes.error, 'error');
    return;
  }

  const url = buildUrl(pathRes.value, macroRes.value);
  outEl.value = url;
  copyBtn.disabled = false;
  openBtn.disabled = false;
  setStatus('Ready.', 'ok');
}

async function copyToClipboard(text) {
  if (navigator.clipboard?.writeText) {
    await navigator.clipboard.writeText(text);
    return;
  }

  throw new Error('Clipboard API unavailable');
}

function init() {
  applyEmbedMode();
  const filePathEl = document.getElementById('filePath');
  const macrosEl = document.getElementById('macros');
  const copyBtn = document.getElementById('copyBtn');
  const openBtn = document.getElementById('openBtn');

  filePathEl.addEventListener('input', update);
  macrosEl.addEventListener('input', update);

  copyBtn.addEventListener('click', async () => {
    const outEl = document.getElementById('generatedUrl');
    const url = outEl.value;
    try {
      await copyToClipboard(url);
      setStatus('Copied to clipboard.', 'ok');
    } catch {
      outEl.focus();
      outEl.select();
      setStatus('Clipboard not available — URL selected, press Ctrl/Cmd+C.', 'error');
    }
  });

  openBtn.addEventListener('click', () => {
    const url = document.getElementById('generatedUrl').value;
    window.open(url, '_blank', 'noopener,noreferrer');
  });

  update();
}

document.addEventListener('DOMContentLoaded', init);
