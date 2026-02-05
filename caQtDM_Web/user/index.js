import { PopupWindow } from './modules/ui/PopupWindow.js';
import { Dialog } from './modules/ui/Dialog.js';
import { setupLauncherMenu, closeAllMenus } from './modules/ui/LauncherMenu.js';
import { VNCClient } from './modules/vnc/VNCClient.js';
import { ControlSocket } from './modules/network/ControlSocket.js';
import { parseLogMarkup, formatDateTime } from './modules/utils/Logger.js';

(function() {
  const params = new URLSearchParams(window.location.search);
  let controlPath = params.get('control') || params.get('path') || '30000';
  let noVNCPath = params.get('novnc') || params.get('novncPath') || '30001';
  const macros = params.get('macros') || '';
  const displayMode = params.has('display');

  const vncContainer = document.getElementById('vnc-container');
  const reconnectOverlay = document.getElementById('reconnect-overlay');
  const dialogOverlay = document.getElementById('dialog-overlay');
  const dialogMessage = document.getElementById('dialog-message');
  const errorOverlay = document.getElementById('error-overlay');
  const errorMessage = document.getElementById('error-message');
  const menuLogsButton = document.getElementById('menu-logs');
  const menuUrlBuilderButton = document.getElementById('menu-url-builder');
  const usersCount = document.getElementById('users-count');
  const logWindow = document.getElementById('log-window');
  const logContent = document.getElementById('log-content');
  const urlBuilderWindow = document.getElementById('url-builder-window');

  // App State
  let failedOnce = false;
  let connectedOnce = false;
  let isNonNumericControlPath = !/^[0-9]+$/.test(controlPath.trim());
  let resolvedControlPath = null;
  let resolvedNoVNCPath = null;
  let newInstance = false;
  let originalControlFile = null;
  let pendingOpenPath = null;
  let pendingOpenMacros = null;
  let pendingUrl = null;
  let pendingUrlType = 'url';
  let pendingFilePath = null;

  if (displayMode) {
    const menuBar = document.getElementById('menu-bar');
    if (menuBar) menuBar.style.display = 'none';
    if (vncContainer) {
      vncContainer.style.top = '0';
      vncContainer.style.height = '100%';
    }
    if (reconnectOverlay) {
      reconnectOverlay.style.top = '0';
      reconnectOverlay.style.height = '100%';
    }
  }

  // --- UI Components ---

  const logPopup = new PopupWindow({
    root: logWindow,
    header: document.getElementById('log-header'),
    resizeHandle: document.getElementById('log-resize-handle'),
    menuButton: menuLogsButton,
    closeButton: document.getElementById('log-close'),
    onOpen: () => menuLogsButton.classList.remove('has-new'),
  });

  const urlBuilderPopup = new PopupWindow({
    root: urlBuilderWindow,
    header: urlBuilderWindow.querySelector('[data-popup-header]') || urlBuilderWindow,
    resizeHandle: urlBuilderWindow.querySelector('[data-popup-resize]') || null,
    menuButton: menuUrlBuilderButton,
    closeButton: document.getElementById('url-builder-close'),
    minWidth: 360,
    minHeight: 220,
  });

  const openFileDialog = new Dialog({
    overlay: dialogOverlay,
    messageElement: dialogMessage,
    timeoutDuration: 30000,
    buttons: {
      'dialog-cancel': () => {
        pendingOpenPath = null;
        pendingOpenMacros = null;
      },
      'dialog-accept': () => {
        if (!pendingOpenPath) return;
        const newUrl = new URL(window.location.href);
        newUrl.searchParams.set('path', pendingOpenPath);
        if (pendingOpenMacros && pendingOpenMacros.trim() !== '') {
          const convertedMacros = pendingOpenMacros.replace(/=/g, ':').replace(/;/g, ',');
          newUrl.searchParams.set('macros', convertedMacros);
        }
        window.open(newUrl.toString(), '_blank', 'noopener');
        pendingOpenPath = null;
        pendingOpenMacros = null;
      },
    },
  });

  const urlDialog = new Dialog({
    overlay: dialogOverlay,
    messageElement: dialogMessage,
    timeoutDuration: 30000,
    buttons: {
      'dialog-cancel': () => {
        pendingUrl = null;
        pendingFilePath = null;
        pendingUrlType = 'url';
      },
      'dialog-accept': () => {
        if (pendingUrlType === 'file' && pendingFilePath) {
          navigator.clipboard.writeText(pendingFilePath).catch(err => console.error('Failed to copy path:', err));
          pendingFilePath = null;
          pendingUrlType = 'url';
        } else if (pendingUrl) {
          window.open(pendingUrl, '_blank', 'noopener');
          pendingUrl = null;
        }
      },
    },
  });

  const errorDialog = new Dialog({
    overlay: errorOverlay,
    messageElement: errorMessage,
    buttons: { 'error-ok': () => {} },
  });

  // --- Utils ---

  function addLogMessage(html) {
    if (logContent.dataset.empty === 'true') {
      logContent.innerHTML = '';
      delete logContent.dataset.empty;
    }
    const entry = document.createElement('div');
    entry.className = 'log-entry';
    entry.appendChild(parseLogMarkup(html));
    logContent.appendChild(entry);
    logContent.scrollTop = logContent.scrollHeight;
    if (!logPopup.isShown()) {
      menuLogsButton.classList.add('has-new');
    }
  }

  function getActiveControlPath() {
    if (resolvedControlPath != null) return resolvedControlPath;
    const value = String(controlPath).trim();
    if (value === '' || isNonNumericControlPath) return '30000';
    return value;
  }

  function getActiveNoVNCPath() {
    return resolvedNoVNCPath || String(noVNCPath).trim() || '30001';
  }

  // --- Clients ---

  const vnc = new VNCClient(vncContainer, reconnectOverlay);

  const control = new ControlSocket({
    urlProvider: () => {
      const wsProtocol = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
      return wsProtocol + window.location.host + '/websockify/' + encodeURIComponent(getActiveControlPath());
    },
    onOpen: () => {
      const dateTime = formatDateTime(new Date());
      if (failedOnce && !newInstance) {
        control.reconnectDelay = 1000;
        failedOnce = false;
        setTimeout(() => {
          if (connectedOnce) addLogMessage('<font color="green">' + dateTime + ' Reconnected to server.</font>');
          vnc.connect();
        }, 1000);
      } else if ((resolvedControlPath != null || !isNonNumericControlPath) && !connectedOnce) {
        addLogMessage('<font color="green">' + dateTime + ' Connected to server.</font>');
      }
      newInstance = false;

      if (isNonNumericControlPath && resolvedControlPath == null) {
        control.send('RESOLVE|' + controlPath + '|' + macros);
      } else {
        connectedOnce = true;
        failedOnce = false;
        control.send(getActiveControlPath());
      }
    },
    onMessage: (data) => handleControlMessage(data),
    onClose: () => {
      if (isNonNumericControlPath && !newInstance && originalControlFile != null) {
        resolvedControlPath = null;
        resolvedNoVNCPath = null;
        controlPath = originalControlFile;
      }
      if (connectedOnce && !failedOnce && !newInstance) {
        const dateTime = formatDateTime(new Date());
        addLogMessage('<font color="red">' + dateTime + ' Disconnected from server.</font>');
      }
      failedOnce = true;
    }
  });

  function handleControlMessage(data) {
    if (data.startsWith('USERS|')) {
      const count = parseInt(data.slice(6), 10);
      if (!isNaN(count)) usersCount.textContent = String(count);
      return;
    }
    if (data.startsWith('LOG|')) {
      addLogMessage(data.slice(4));
      return;
    }
    if (data.startsWith('ERROR|')) {
      errorDialog.open(data.slice(6));
      return;
    }
    if (data.startsWith('PROGRESS|')) {
      updateProgress(parseInt(data.slice(9), 10));
      return;
    }
    if (data.startsWith('INIT_PROGRESS|')) {
      const [init, max] = data.slice(14).split('|').map(v => parseInt(v, 10));
      initProgress(init, max);
      return;
    }
    if (data.startsWith('TIMEOUT|')) {
      handleTimeout(data.slice(8));
      return;
    }
    if (data.startsWith('OPEN_URL|')) {
      handleOpenUrl(data.slice(9).trim());
      return;
    }
    if (data.startsWith('INSTANCE|')) {
      handleInstance(data.slice(9));
      return;
    }
    if (data.startsWith('LAUNCHER|')) {
      try {
        setupLauncherMenu(JSON.parse(data.slice(9)));
      } catch (e) {
        console.warn('Failed to parse LAUNCHER payload:', e);
      }
      return;
    }
    const openMatch = data.match(/^OPEN\|([^|]+)\|(.*)$/);
    if (openMatch) {
      handleOpenPath(openMatch[1], openMatch[2]);
    }
  }

  function updateProgress(progress) {
    const progressBar = document.getElementById('progress-bar');
    const progressText = document.getElementById('progress-text');
    if (!isNaN(progress)) {
      const percent = Math.round((progress / progressBar.max) * 100);
      const space = percent < 10 ? '  ' : (percent < 100 ? ' ' : '');
      progressText.textContent = space + percent + '%';
      progressBar.value = progress;
      if (progress >= progressBar.max) {
        setTimeout(() => { document.getElementById('menu-progress').style.display = 'none'; }, 5000);
      }
    }
  }

  function initProgress(init, max) {
    const progressBar = document.getElementById('progress-bar');
    const progressText = document.getElementById('progress-text');
    if (!isNaN(init) && !isNaN(max)) {
      progressBar.max = max;
      progressBar.value = init;
      progressText.textContent = '  ' + Math.round((init / max) * 100) + '%';
      document.getElementById('menu-progress').style.display = 'flex';
    }
  }

  function handleTimeout(reason) {
    errorDialog.open('Connection timed out: ' + reason);
    control.disableReconnect();
    const okBtn = document.getElementById('error-ok');
    if (okBtn) { okBtn.disabled = true; okBtn.style.display = 'none'; }
    control.socket.close();
    vnc.disconnect();
    vncContainer.innerHTML = '';
  }

  function handleOpenUrl(urlToOpen) {
    if (!urlToOpen) return;
    const acceptButton = document.getElementById('dialog-accept');
    const message = document.createElement('p');

    if (urlToOpen.startsWith('file://')) {
      const filePath = urlToOpen.slice(7);
      pendingFilePath = filePath;
      pendingUrlType = 'file';
      message.textContent = 'File path: ';
      const span = document.createElement('span');
      span.textContent = filePath;
      span.style.fontFamily = 'monospace';
      message.appendChild(span);
      if (acceptButton) acceptButton.textContent = 'Copy';
    } else {
      pendingUrl = urlToOpen;
      pendingUrlType = 'url';
      message.textContent = 'Open URL: ';
      const link = document.createElement('a');
      link.href = urlToOpen;
      link.target = '_blank';
      link.rel = 'noopener';
      link.textContent = urlToOpen;
      link.onclick = () => urlDialog.close();
      message.appendChild(link);
      message.appendChild(document.createTextNode('?'));
      if (acceptButton) acceptButton.textContent = 'Open';
    }
    urlDialog.setMessage(message);
    urlDialog.open();
  }

  function handleInstance(payload) {
    const [nextNoVNC, nextControl] = payload.split('|').map(v => (v || '').trim());
    if (isNonNumericControlPath && resolvedControlPath == null) {
      resolvedControlPath = nextControl || '30000';
      resolvedNoVNCPath = nextNoVNC || '30001';
      originalControlFile = controlPath;
      vnc.setPath(resolvedNoVNCPath);
      vnc.connect();
      control.socket.close();
      newInstance = true;
    }
  }

  function handleOpenPath(path, macros) {
    pendingOpenPath = path;
    pendingOpenMacros = macros;
    let msg = 'Open "' + path + '"?';
    if (macros && macros.trim() !== '') {
      const list = macros.split(';').filter(m => m.trim() !== '');
      if (list.length > 0) msg += '\n\nMacros:\n' + list.join('\n');
    }
    openFileDialog.open(msg);
  }

  // --- Global Event Handlers ---

  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') PopupWindow.closeTopMost();
  });

  document.addEventListener('click', (e) => {
    const launcherBtn = document.getElementById('launcher');
    const viewBtn = document.getElementById('menu-view');
    if ((launcherBtn && launcherBtn.contains(e.target)) || (viewBtn && viewBtn.contains(e.target))) return;
    closeAllMenus();
  });

  window.addEventListener('launcher-command', (e) => {
    const cmd = e.detail;
    const message = document.createElement('p');
    message.textContent = 'Command: ';
    const span = document.createElement('span');
    span.textContent = cmd;
    span.style.fontFamily = 'monospace';
    span.style.wordBreak = 'break-all';
    message.appendChild(span);

    pendingFilePath = cmd;
    pendingUrlType = 'file';
    const acceptBtn = document.getElementById('dialog-accept');
    if (acceptBtn) acceptBtn.textContent = 'Copy';
    urlDialog.setMessage(message);
    urlDialog.open();
  });

  const viewBtn = document.getElementById('menu-view');
  if (viewBtn) {
    viewBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      const menu = document.getElementById('view-menu');
      const isShown = menu.classList.contains('show');
      closeAllMenus();
      if (!isShown) {
        menu.classList.add('show');
        viewBtn.setAttribute('aria-expanded', 'true');
      }
    });
  }

  const fullScreenItem = document.getElementById('view-fullscreen');
  if (fullScreenItem) {
    fullScreenItem.addEventListener('click', () => {
      if (!document.fullscreenElement) {
        vncContainer.requestFullscreen().catch(err => console.error('Fullscreen error:', err));
      } else if (document.exitFullscreen) {
        document.exitFullscreen();
      }
      closeAllMenus();
    });
  }

  const urlBuilderOpenTab = document.getElementById('url-builder-open-tab');
  if (urlBuilderOpenTab) {
    urlBuilderOpenTab.addEventListener('click', () => {
      window.open('/url-builder', '_blank', 'noopener');
      urlBuilderPopup.close();
    });
  }

  // --- Initialization ---

  vnc.setPath(getActiveNoVNCPath());
  vnc.connect();
  control.connect();

})();
