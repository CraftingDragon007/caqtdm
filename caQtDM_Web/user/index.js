(function() {
  const params = new URLSearchParams(window.location.search);
  let controlPath = params.get('control') || params.get('path') || '30000';
  let noVNCPath = params.get('novnc') || params.get('novncPath') || '30001';
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

  class Dialog {
    constructor(opts) {
      this.overlay = opts.overlay;
      this.messageElement = opts.messageElement;
      this.timeout = opts.timeout || null;
      this.timeoutDuration = opts.timeoutDuration || 0;
      this.buttons = opts.buttons || {}; // { buttonId: callback }
      this.onBeforeOpen = opts.onBeforeOpen || null;
      this.onAfterClose = opts.onAfterClose || null;

      this.timeoutHandle = null;
      this.setupButtonListeners();
    }

    setupButtonListeners() {
      Object.entries(this.buttons).forEach(([buttonId, callback]) => {
        const button = document.getElementById(buttonId);
        if (button) {
          button.addEventListener('click', () => {
            if (typeof callback === 'function') {
              callback();
            }
            this.close();
          });
        }
      });
    }

    setMessage(message) {
      if (this.messageElement) {
        if (typeof message === 'string') {
          this.messageElement.textContent = message;
        } else {
          this.messageElement.innerHTML = '';
          this.messageElement.appendChild(message);
        }
      }
    }

    open(message) {
      if (message) {
        this.setMessage(message);
      }
      if (typeof this.onBeforeOpen === 'function') {
        this.onBeforeOpen();
      }
      this.overlay.classList.add('show');
      if (this.timeoutDuration > 0) {
        this.startTimeout();
      }
    }

    close() {
      this.overlay.classList.remove('show');
      this.clearTimeout();
      if (typeof this.onAfterClose === 'function') {
        this.onAfterClose();
      }
    }

    isShown() {
      return this.overlay.classList.contains('show');
    }

    startTimeout() {
      this.clearTimeout();
      this.timeoutHandle = setTimeout(() => {
        this.close();
      }, this.timeoutDuration);
    }

    clearTimeout() {
      if (this.timeoutHandle) {
        window.clearTimeout(this.timeoutHandle);
        this.timeoutHandle = null;
      }
    }
  }

  let failedOnce = false;
  let connectedOnce = false;
  let isNonNumericControlPath = false;
  let resolvedControlPath = null;
  let resolvedNoVNCPath = null;
  let newInstance = false;
  let originalControlFile = null;

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

  const openFileDialog = new Dialog({
    overlay: dialogOverlay,
    messageElement: dialogMessage,
    timeoutDuration: 30000,
    buttons: {
      'dialog-cancel': function() {
        pendingOpenPath = null;
        pendingOpenMacros = null;
      },
      'dialog-accept': function() {
        if (!pendingOpenPath) {
          pendingOpenPath = null;
          pendingOpenMacros = null;
          return;
        }
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

  const errorDialog = new Dialog({
    overlay: errorOverlay,
    messageElement: errorMessage,
    timeoutDuration: 0,
    buttons: {
      'error-ok': function() {},
    },
  });

  let pendingUrl = null;
  let pendingUrlType = 'url'; // 'url' or 'file'
  let pendingFilePath = null;

  const urlDialog = new Dialog({
    overlay: dialogOverlay,
    messageElement: dialogMessage,
    timeoutDuration: 30000,
    buttons: {
      'dialog-cancel': function() {
        pendingUrl = null;
        pendingFilePath = null;
        pendingUrlType = 'url';
      },
      'dialog-accept': function() {
        if (pendingUrlType === 'file' && pendingFilePath) {
          navigator.clipboard.writeText(pendingFilePath).then(function() {
            console.log('Path copied to clipboard');
          }).catch(function(err) {
            console.error('Failed to copy path:', err);
          });
          pendingFilePath = null;
          pendingUrlType = 'url';
        } else if (pendingUrl) {
          window.open(pendingUrl, '_blank', 'noopener');
          pendingUrl = null;
        }
      },
    },
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

  function getControlPath() {
    if (resolvedControlPath != null) {
      return resolvedControlPath;
    }
    if (controlPath == null) return '30000';
    const value = String(controlPath).trim();
    if (value === '') return '30000';
    if (!isNumericPath(value)) {
      isNonNumericControlPath = true;
      return '30000';
    }
    return value;
  }

  function getNoVNCPath() {
    if (resolvedNoVNCPath != null) {
      return resolvedNoVNCPath;
    }
    if (noVNCPath == null) return '30001';
    const value = String(noVNCPath).trim();
    if (value === '') return '30001';
    return value;
  }

  function sanitizeNoVNCUrl(raw) {
    if (raw == null) return null;
    const value = String(raw).trim();
    if (value === '') return null;

    // Allow only same-origin URLs under the /noVNC path.
    try {
      const url = new URL(value, window.location.origin);
      if (url.origin !== window.location.origin) {
        return null;
      }
      if (!url.pathname.startsWith('/noVNC')) {
        return null;
      }
      if (url.protocol !== window.location.protocol) {
        return null;
      }
      return url.toString();
    } catch (e) {
      return null;
    }
  }

  function setIframeToNoVNCPath(p) {
    let value = p;
    if (value == null || value === '') value = '30001';
    value = String(value).trim();

    const sanitizedUrl = sanitizeNoVNCUrl(value);
    if (sanitizedUrl !== null) {
      iframe.src = sanitizedUrl;
      noVNCPath = value;
      return;
    }

    noVNCPath = value;
    iframe.src = '/noVNC/vnc.html?path=/websockify/' + encodeURIComponent(getNoVNCPath()) + '&autoconnect=true&reconnect=true&reconnect_delay=5000-novnc_readonly &resize=scale';
  }

  setIframeToNoVNCPath(noVNCPath);

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
    const wsUrl = wsProtocol + window.location.host + '/websockify/' + encodeURIComponent(getControlPath());
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
      } else if ((resolvedControlPath != null || !isNonNumericControlPath) && !connectedOnce) {
        addLogMessage('<font color="green">' + dateTime + ' Connected to server.</font>');
      }
      newInstance = false;
      try {
        if (isNonNumericControlPath && resolvedControlPath == null) {
          // Request instance number for non-numeric path
          const message = 'RESOLVE|' + controlPath + '|' + macros;
          socket.send(message);
        } else {
          connectedOnce = true;
          failedOnce = false;
          socket.send(getControlPath());
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
        errorDialog.open(data.slice(6));
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
        errorDialog.open('Connection timed out: ' + data.slice(8));
        reconnectDelay = -1; //disable further reconnects
        const errorOkButton = document.getElementById('error-ok');
        if (errorOkButton) {
          errorOkButton.disabled = true;
          errorOkButton.style.display = 'none';
        }
        socket.close();
        iframe.src = 'about:blank';
        return;
      }
      if (data.startsWith('OPEN_URL|')) {
        const urlToOpen = data.slice(9).trim();
        if (urlToOpen) {
          if (urlToOpen.startsWith('file://')) {
            const filePath = urlToOpen.slice(7);
            pendingFilePath = filePath;
            pendingUrlType = 'file';
            const message = document.createElement('p');
            message.textContent = 'File path: ';
            const pathSpan = document.createElement('span');
            pathSpan.textContent = filePath;
            pathSpan.style.fontFamily = 'monospace';
            message.appendChild(pathSpan);
            urlDialog.setMessage(message);
            // Update button text to "Copy"
            const acceptButton = document.getElementById('dialog-accept');
            if (acceptButton) {
              acceptButton.textContent = 'Copy';
            }
            urlDialog.open();
          } else {
            pendingUrl = urlToOpen;
            pendingUrlType = 'url';
            const message = document.createElement('p');
            message.textContent = 'Open URL: ';
            const link = document.createElement('a');
            link.href = urlToOpen;
            link.target = '_blank';
            link.rel = 'noopener';
            link.textContent = urlToOpen;
            link.addEventListener('click', function() {
              urlDialog.close();
            });
            message.appendChild(link);
            message.appendChild(document.createTextNode('?'));
            urlDialog.setMessage(message);
            // Update button text back to "Open"
            const acceptButton = document.getElementById('dialog-accept');
            if (acceptButton) {
              acceptButton.textContent = 'Open';
            }
            urlDialog.open();
          }
        }
        return;
      }
      if (data.startsWith('INSTANCE|')) {
        const payload = data.slice(9).split('|');
        const nextNoVNCPath = (payload[0] || '').trim();
        const nextControlPath = (payload[1] || '').trim();
        if (isNonNumericControlPath && resolvedControlPath == null) {
          resolvedControlPath = nextControlPath || '30000';
          resolvedNoVNCPath = nextNoVNCPath || '30001';
          originalControlFile = controlPath;
          setIframeToNoVNCPath(resolvedNoVNCPath);
          socket.close();
          newInstance = true;
          // onclose will call scheduleReconnect, which will use the resolved control path
        }
        return;
      }
      const openMatchPipe = data.match(/^OPEN\|([^|]+)\|(.*)$/);
      if (openMatchPipe) {
        pendingOpenPath = openMatchPipe[1];
        pendingOpenMacros = openMatchPipe[2];
        let message = 'Open "' + pendingOpenPath + '"?';

        if (pendingOpenMacros && pendingOpenMacros.trim() !== '') {
          const macroList = pendingOpenMacros.split(';').filter(function(m) { return m.trim() !== ''; });
          if (macroList.length > 0) {
            message += '\n\nMacros:\n' + macroList.join('\n');
          }
        }

        openFileDialog.open(message);
        return;
      }
    };

    socket.onerror = function(errorEvent) {
      console.warn('WebSocket error:', errorEvent);
      socket.close();
    };

    socket.onclose = function() {
      if (isNonNumericControlPath && !newInstance && originalControlFile != null) {
        // ask for instance again, as the path could've changed
        resolvedControlPath = null;
        resolvedNoVNCPath = null;
        controlPath = originalControlFile;
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
