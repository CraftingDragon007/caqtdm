(function() {
  const params = new URLSearchParams(window.location.search);
  let path = params.get('path') || '0';
  const macros = params.get('macros') || '';

  const iframe = document.getElementById('vnc-container');
  const dialogOverlay = document.getElementById('dialog-overlay');
  const dialogMessage = document.getElementById('dialog-message');
  const dialogAccept = document.getElementById('dialog-accept');
  const dialogCancel = document.getElementById('dialog-cancel');
  let pendingOpenPath = null;
  let pendingOpenMacros = null;
  const errorOverlay = document.getElementById('error-overlay');
  const errorMessage = document.getElementById('error-message');
  const errorOk = document.getElementById('error-ok');
  const menuLogsButton = document.getElementById('menu-logs');
  const menuUrlBuilderButton = document.getElementById('menu-url-builder');
  const logWindow = document.getElementById('log-window');
  const logClose = document.getElementById('log-close');
  const logContent = document.getElementById('log-content');
  const logHeader = document.getElementById('log-header');
  const logResizeHandle = document.getElementById('log-resize-handle');
  const urlBuilderWindow = document.getElementById('url-builder-window');
  const urlBuilderClose = document.getElementById('url-builder-close');
  const urlBuilderOpenTab = document.getElementById('url-builder-open-tab');
  let dialogTimeout = null;
  const DEFAULT_MIN_PADDING = 12;
  const DEFAULT_MIN_RESIZE_WIDTH = 240;
  const DEFAULT_MIN_RESIZE_HEIGHT = 160;

  class PopupWindow {
    static zCounter = 1000;
    static openStack = [];

    constructor(opts) {
      this.root = opts.root;
      this.header = opts.header;
      this.resizeHandle = opts.resizeHandle || null;
      this.menuButton = opts.menuButton || null;
      this.closeButton = opts.closeButton || null;
      this.minPadding = opts.minPadding != null ? opts.minPadding : DEFAULT_MIN_PADDING;
      this.minWidth = opts.minWidth != null ? opts.minWidth : DEFAULT_MIN_RESIZE_WIDTH;
      this.minHeight = opts.minHeight != null ? opts.minHeight : DEFAULT_MIN_RESIZE_HEIGHT;
      this.onOpen = opts.onOpen || null;
      this.onClose = opts.onClose || null;

      this.isDragging = false;
      this.dragOffsetX = 0;
      this.dragOffsetY = 0;
      this.isResizing = false;
      this.resizeStartWidth = 0;
      this.resizeStartHeight = 0;
      this.resizeStartX = 0;
      this.resizeStartY = 0;
      this.manuallyPositioned = false;

      if (this.header) {
        this.header.addEventListener('pointerdown', this.onDragStart.bind(this));
        // Prevent touch scroll/gestures from canceling pointer events during drag
        try { this.header.style.touchAction = 'none'; } catch (_e) {}
        try { this.header.style.cursor = 'move'; } catch (_e) {}
      }
      if (this.resizeHandle) {
        this.resizeHandle.addEventListener('pointerdown', this.onResizeStart.bind(this));
        // Prevent touch scroll/gestures from canceling pointer events during resize
        try { this.resizeHandle.style.touchAction = 'none'; } catch (_e) {}
        try { this.resizeHandle.style.cursor = 'nwse-resize'; } catch (_e) {}
      }
      if (this.closeButton) {
        this.closeButton.addEventListener('click', this.close.bind(this));
      }
      if (this.menuButton) {
        this.menuButton.addEventListener('click', this.toggle.bind(this));
      }
      this.root.addEventListener('pointerdown', () => this.bringToFront());

      window.addEventListener('resize', () => this.ensureInView());
      document.addEventListener('pointercancel', () => this.onDragEnd());
      document.addEventListener('pointercancel', () => this.onResizeEnd());
    }

    bringToFront() {
      PopupWindow.zCounter += 1;
      this.root.style.zIndex = String(PopupWindow.zCounter);
    }

    isShown() {
      return this.root.classList.contains('show');
    }

    open() {
      if (this.isShown()) return;
      this.root.classList.add('show');
      this.bringToFront();
      if (this.menuButton) this.menuButton.setAttribute('aria-expanded', 'true');
      if (typeof this.onOpen === 'function') this.onOpen();
      // Focus the first focusable element, fallback to root
      try { this.root.focus(); } catch (_e) {}
      this.ensureInView();
      PopupWindow.openStack.push(this);
    }

    close() {
      if (!this.isShown()) return;
      this.root.classList.remove('show');
      if (this.menuButton) this.menuButton.setAttribute('aria-expanded', 'false');
      if (typeof this.onClose === 'function') this.onClose();
      // Remove from stack if present
      const idx = PopupWindow.openStack.lastIndexOf(this);
      if (idx !== -1) PopupWindow.openStack.splice(idx, 1);
      // Return focus to menu button to keep keyboard flow
      if (this.menuButton) this.menuButton.focus();
    }

    toggle() {
      if (this.isShown()) this.close(); else this.open();
    }

    prepareManualPosition(rect) {
      if (this.manuallyPositioned) return;
      this.root.style.left = rect.left + 'px';
      this.root.style.top = rect.top + 'px';
      this.root.style.right = 'auto';
      this.root.style.maxHeight = 'none';
      this.manuallyPositioned = true;
      this.ensureInView();
    }

    ensureInView() {
      if (!this.manuallyPositioned || !this.isShown()) return;
      const MIN_PADDING = this.minPadding;
      const maxWidth = Math.max(0, window.innerWidth - (2 * MIN_PADDING));
      const maxHeight = Math.max(0, window.innerHeight - (2 * MIN_PADDING));
      let rect = this.root.getBoundingClientRect();
      let width = rect.width;
      let height = rect.height;
      if (maxWidth > 0 && width > maxWidth) {
        width = maxWidth;
        this.root.style.width = width + 'px';
      }
      if (maxHeight > 0 && height > maxHeight) {
        height = maxHeight;
        this.root.style.height = height + 'px';
      }
      rect = this.root.getBoundingClientRect();
      width = rect.width;
      height = rect.height;
      const maxLeft = Math.max(0, window.innerWidth - width - MIN_PADDING);
      const maxTop = Math.max(0, window.innerHeight - height - MIN_PADDING);
      const nextLeft = Math.min(Math.max(MIN_PADDING, rect.left), maxLeft);
      const nextTop = Math.min(Math.max(MIN_PADDING, rect.top), maxTop);
      this.root.style.left = nextLeft + 'px';
      this.root.style.top = nextTop + 'px';
      this.root.style.right = 'auto';
    }

    onDragStart(event) {
      if (event.button !== 0 && event.pointerType !== 'touch') return;
      if (event.target && event.target.closest('button')) return;
      let rect = this.root.getBoundingClientRect();
      this.prepareManualPosition(rect);
      rect = this.root.getBoundingClientRect();
      this.isDragging = true;
      this.dragOffsetX = event.clientX - rect.left;
      this.dragOffsetY = event.clientY - rect.top;
      // Capture the pointer so we continue to receive events even
      // if the cursor leaves the window or moves over an iframe
      try { this.root.setPointerCapture(event.pointerId); } catch (_e) {}
      this.root.addEventListener('pointermove', this.onDragMoveBound = this.onDragMove.bind(this));
      this.root.addEventListener('pointerup', this.onDragEndBound = this.onDragEnd.bind(this));
      event.preventDefault();
    }

    onDragMove(event) {
      if (!this.isDragging) return;
      const MIN_PADDING = this.minPadding;
      const width = this.root.offsetWidth;
      const height = this.root.offsetHeight;
      let newLeft = event.clientX - this.dragOffsetX;
      let newTop = event.clientY - this.dragOffsetY;
      const maxLeft = Math.max(0, window.innerWidth - width - MIN_PADDING);
      const maxTop = Math.max(0, window.innerHeight - height - MIN_PADDING);
      newLeft = Math.min(Math.max(MIN_PADDING, newLeft), maxLeft);
      newTop = Math.min(Math.max(MIN_PADDING, newTop), maxTop);
      this.root.style.left = newLeft + 'px';
      this.root.style.top = newTop + 'px';
      this.root.style.right = 'auto';
    }

    onDragEnd() {
      if (!this.isDragging) return;
      this.isDragging = false;
      try { this.root.releasePointerCapture && this.root.releasePointerCapture(this.dragCaptureId); } catch (_e) {}
      this.root.removeEventListener('pointermove', this.onDragMoveBound);
      this.root.removeEventListener('pointerup', this.onDragEndBound);
    }

    onResizeStart(event) {
      if (event.button !== 0 && event.pointerType !== 'touch') return;
      let rect = this.root.getBoundingClientRect();
      this.prepareManualPosition(rect);
      rect = this.root.getBoundingClientRect();
      this.isResizing = true;
      this.resizeStartWidth = rect.width;
      this.resizeStartHeight = rect.height;
      this.resizeStartX = event.clientX;
      this.resizeStartY = event.clientY;
      // Capture the pointer so resize continues even if cursor leaves
      try { this.root.setPointerCapture(event.pointerId); } catch (_e) {}
      this.root.addEventListener('pointermove', this.onResizeMoveBound = this.onResizeMove.bind(this));
      this.root.addEventListener('pointerup', this.onResizeEndBound = this.onResizeEnd.bind(this));
      event.preventDefault();
    }

    onResizeMove(event) {
      if (!this.isResizing) return;
      const MIN_PADDING = this.minPadding;
      const deltaX = event.clientX - this.resizeStartX;
      const deltaY = event.clientY - this.resizeStartY;
      const maxWidth = Math.max(0, window.innerWidth - (2 * MIN_PADDING));
      const maxHeight = Math.max(0, window.innerHeight - (2 * MIN_PADDING));
      const minWidth = maxWidth > 0 ? Math.min(this.minWidth, maxWidth) : 0;
      const minHeight = maxHeight > 0 ? Math.min(this.minHeight, maxHeight) : 0;
      const targetWidth = this.resizeStartWidth + deltaX;
      const targetHeight = this.resizeStartHeight + deltaY;
      const newWidth = Math.max(minWidth, Math.min(targetWidth, maxWidth || targetWidth));
      const newHeight = Math.max(minHeight, Math.min(targetHeight, maxHeight || targetHeight));
      this.root.style.width = newWidth + 'px';
      this.root.style.height = newHeight + 'px';
      this.ensureInView();
    }

    onResizeEnd() {
      if (!this.isResizing) return;
      this.isResizing = false;
      try { this.root.releasePointerCapture && this.root.releasePointerCapture(this.resizeCaptureId); } catch (_e) {}
      this.root.removeEventListener('pointermove', this.onResizeMoveBound);
      this.root.removeEventListener('pointerup', this.onResizeEndBound);
    }

    static closeTopMost() {
      const top = PopupWindow.openStack[PopupWindow.openStack.length - 1];
      if (top) top.close();
    }
  }

  let failedOnce = false;
  let connectedOnce = false;
  let isNonNumericPath = false;
  let resolvedInstance = null;
  let newInstance = false;
  let file = null;

  let heartbeat = null;

  const logPopup = new PopupWindow({
    root: logWindow,
    header: logHeader,
    resizeHandle: logResizeHandle,
    menuButton: menuLogsButton,
    closeButton: logClose,
    minPadding: DEFAULT_MIN_PADDING,
    minWidth: DEFAULT_MIN_RESIZE_WIDTH,
    minHeight: DEFAULT_MIN_RESIZE_HEIGHT,
    onOpen: function() { menuLogsButton.classList.remove('has-new'); },
  });

  const urlBuilderPopup = new PopupWindow({
    root: urlBuilderWindow,
    header: urlBuilderWindow.querySelector('[data-popup-header]') || urlBuilderWindow,
    resizeHandle: urlBuilderWindow.querySelector('[data-popup-resize]') || null,
    menuButton: menuUrlBuilderButton,
    closeButton: urlBuilderClose,
    minPadding: DEFAULT_MIN_PADDING,
    minWidth: 360,
    minHeight: 220,
  });

  function parseLogMarkup(raw) {
    const container = document.createElement('div');
    container.innerHTML = raw;
    const fontElements = container.getElementsByTagName('font');
    while (fontElements.length) {
      const fontNode = fontElements[0];
      const span = document.createElement('span');
      const colorAttr = fontNode.getAttribute('color');
      if (colorAttr) span.style.color = colorAttr;
      while (fontNode.firstChild) {
        span.appendChild(fontNode.firstChild);
      }
      if (fontNode.parentNode) {
        fontNode.parentNode.replaceChild(span, fontNode);
      } else {
        container.appendChild(span);
      }
    }
    const fragment = document.createDocumentFragment();
    while (container.firstChild) {
      fragment.appendChild(container.firstChild);
    }
    return fragment;
  }

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

  if (urlBuilderOpenTab) {
    urlBuilderOpenTab.addEventListener('click', function() {
      window.open('/url-builder', '_blank', 'noopener');
      urlBuilderPopup.close();
    });
  }

  document.addEventListener('keydown', function(event) {
    if (event.key !== 'Escape') return;
    PopupWindow.closeTopMost();
  });

  function isNumericPath(value) {
    const normalized = String(value).trim();
    return /^[0-9]+$/.test(normalized);
  }

  function getInstancePath() {
    if (resolvedInstance != null) {
      return resolvedInstance;
    }
    if (path == null) return '00';
    const value = String(path).trim();
    if (value === '') return '00';
    if (!isNumericPath(value)) {
      isNonNumericPath = true;
      return '00';
    }
    return value.length === 1 ? '0' + value : value;
  }

  function setIframeToPath(p) {
    let value = p;
    if (value == null || value === '') value = '0';
    value = String(value).trim();
    if (/^[0-9]$/.test(value)) value = '0' + value;
    if (value.startsWith('/noVNC') || value.startsWith('http')) {
      iframe.src = value;
      path = value;
      return;
    }
    path = value;
    iframe.src = '/noVNC/vnc.html?path=/websockify/' + encodeURIComponent(getInstancePath()) + '&autoconnect=true&reconnect=true&reconnect_delay=5000-novnc_readonly &resize=scale';
  }

  setIframeToPath(path);

  let reconnectDelay = 1000;
  let reconnectTimer = null;

  function scheduleReconnect() {
    if (reconnectDelay < 0) return; //disabled
    failedOnce = true;
    if (reconnectTimer) return;
    reconnectTimer = setTimeout(function() {
      reconnectTimer = null;
      reconnectDelay = Math.min(reconnectDelay * 2, 10000);
      connectWebSocket();
    }, reconnectDelay);
  }

  function hideDialog() {
    if (dialogTimeout) {
      clearTimeout(dialogTimeout);
      dialogTimeout = null;
    }
    dialogOverlay.classList.remove('show');
    pendingOpenPath = null;
    pendingOpenMacros = null;
  }

  function showOpenFileDialog(requestedPath, macros) {
    pendingOpenPath = requestedPath;
    pendingOpenMacros = macros;
    let message = 'Open "' + requestedPath + '"?';

    if (macros && macros.trim() !== '') {
      const macroList = macros.split(';').filter(function(m) { return m.trim() !== ''; });
      if (macroList.length > 0) {
        message += '\n\nMacros:\n' + macroList.join('\n');
      }
    }

    dialogMessage.textContent = message;
    dialogOverlay.classList.add('show');
    if (dialogTimeout) clearTimeout(dialogTimeout);
    dialogTimeout = setTimeout(function() {
      hideDialog();
    }, 30000);
  }

  dialogCancel.addEventListener('click', hideDialog);

  dialogAccept.addEventListener('click', function() {
    if (!pendingOpenPath) {
      hideDialog();
      return;
    }
    const newUrl = new URL(window.location.href);
    newUrl.searchParams.set('path', pendingOpenPath);
    if (pendingOpenMacros && pendingOpenMacros.trim() !== '') {
      // Convert macros from KEY=VALUE;KEY1=VALUE1 to KEY:VALUE,KEY1:VALUE1
      const convertedMacros = pendingOpenMacros.replace(/=/g, ':').replace(/;/g, ',');
      newUrl.searchParams.set('macros', convertedMacros);
    }
    window.open(newUrl.toString(), '_blank', 'noopener');
    hideDialog();
  });

  function hideErrorDialog() {
    errorOverlay.classList.remove('show');
  }

  function showErrorDialog(message) {
    errorMessage.textContent = message;
    errorOverlay.classList.add('show');
  }

  errorOk.addEventListener('click', hideErrorDialog);

  function formatDateTime(date) {
    const day = String(date.getDate()).padStart(2, '0');
    const month = String(date.getMonth() + 1).padStart(2, '0'); // Months are 0-indexed
    const year = date.getFullYear();
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');
    const seconds = String(date.getSeconds()).padStart(2, '0');

    return `${day}-${month}-${year} ${hours}:${minutes}:${seconds}`;
  }

  function connectWebSocket() {
    const wsProtocol = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
    const wsUrl = wsProtocol + window.location.host + '/instance/' + encodeURIComponent(getInstancePath());
    let socket;

    try {
      socket = new WebSocket(wsUrl);
    } catch (err) {
      console.error('WebSocket connection failed:', err);
      scheduleReconnect();
      return;
    }

    socket.onopen = function() {
      const dateTime = formatDateTime(new Date());
      if (failedOnce && !newInstance) {
        reconnectDelay = 1000;
        failedOnce = false;
        setTimeout(function() {
          if (connectedOnce)
            addLogMessage('<font color="green">' + dateTime + ' Reconnected to server.</font>');
          iframe.contentWindow.location.reload();
        }, 1000);
      } else if ((resolvedInstance != null || !isNonNumericPath) && !connectedOnce) {
        addLogMessage('<font color="green">' + dateTime + ' Connected to server.</font>');
      }
      newInstance = false;
      try {
        if (isNonNumericPath && resolvedInstance == null) {
          // Request instance number for non-numeric path
          const message = 'RESOLVE|' + path + '|' + macros;
          socket.send(message);
        } else {
          connectedOnce = true;
          failedOnce = false;
          socket.send(getInstancePath());
          if (heartbeat != null) {
            clearInterval(heartbeat);
            heartbeat = null;
          }
          heartbeat = setInterval(function() {
            if (socket.readyState === WebSocket.OPEN) {
              socket.send('PING');
            } else {
              clearInterval(heartbeat);
              heartbeat = null;
            }
          }, 50000); //send a ping every 50 seconds
        }
      } catch (sendErr) {
        console.warn('WebSocket send failed:', sendErr);
      }
    };

    socket.onmessage = function(event) {
      const data = String(event.data || '');
      if (data.startsWith('LOG|')) {
        addLogMessage(data.slice(4));
        return;
      }
      if (data.startsWith('ERROR|')) {
        showErrorDialog(data.slice(6));
        return;
      }
      if (data.startsWith('PROGRESS|')) {
        const progressText = document.getElementById('progress-text');
        const progressBar = document.getElementById('progress-bar');
        const menuProgress = document.getElementById('menu-progress');
        const progress = parseInt(data.slice(9), 10);
        if (!isNaN(progress)) {
          const percent = Math.round((progress / progressBar.max) * 100);
          const digitSpace = percent < 10 ? '  ' : (percent < 100 ? ' ' : '');
          progressText.textContent = digitSpace + percent + '%';
          progressBar.value = progress;
          if (progress >= progressBar.max) {
            setTimeout(function() {
              menuProgress.style.display = 'none';
            }, 5000);
          }
        } else {
          console.warn('Invalid PROGRESS value:', data.slice(9));
        }
        return;
      }
      if (data.startsWith('INIT_PROGRESS|')) {
        const progressText = document.getElementById('progress-text');
        const progressBar = document.getElementById('progress-bar');
        const menuProgress = document.getElementById('menu-progress');
        const [init, max] = data.slice(14).split('|').map(function(v) { return parseInt(v, 10); });
        if (!isNaN(init) && !isNaN(max)) {
          progressBar.max = max;
          progressBar.value = init;
          progressText.textContent = '  ' + Math.round((init / max) * 100) + '%';
          menuProgress.style.display = 'flex';
        } else {
          console.warn('Invalid INIT_PROGRESS values:', data.slice(14));
        }
        return;
      }
      if (data.startsWith('TIMEOUT|')) {
        showErrorDialog('Connection timed out: ' + data.slice(8));
        reconnectDelay = -1; //disable further reconnects
        errorOk.disabled = true;
        errorOk.style.display = 'none';
        socket.close();
        iframe.src = 'about:blank';
        return;
      }
      if (data.startsWith('INSTANCE|')) {
        let instance = data.slice(9).trim();
        if (isNonNumericPath && resolvedInstance == null) {
          instance = instance.length === 1 ? '0' + instance : instance
          resolvedInstance = instance;
          file = path;
          setIframeToPath(instance);
          socket.close();
          newInstance = true;
          // onclose will call scheduleReconnect, which will use the resolved instance
        }
        return;
      }
      const openMatchPipe = data.match(/^OPEN\|([^|]+)\|(.*)$/);
      if (openMatchPipe) {
        showOpenFileDialog(openMatchPipe[1], openMatchPipe[2]);
        return;
      }
    };

    socket.onerror = function(errorEvent) {
      console.warn('WebSocket error:', errorEvent);
      socket.close();
    };

    socket.onclose = function() {
      if (isNonNumericPath && !newInstance && file != null) {
        // ask for instance again, as the path could've changed
        resolvedInstance = null;
        path = file;
      }
      if (connectedOnce && !failedOnce && !newInstance) {
        const dateTime = formatDateTime(new Date());
        addLogMessage('<font color="red">' + dateTime + ' Disconnected from server.</font>');
      }
      scheduleReconnect();
    };
  }

  connectWebSocket();
})();
