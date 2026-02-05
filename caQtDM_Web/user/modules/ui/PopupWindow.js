export const DEFAULT_MIN_PADDING = 12;
export const DEFAULT_MIN_RESIZE_WIDTH = 240;
export const DEFAULT_MIN_RESIZE_HEIGHT = 160;

export class PopupWindow {
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
      try { this.header.style.touchAction = 'none'; } catch (_e) {}
      try { this.header.style.cursor = 'move'; } catch (_e) {}
    }
    if (this.resizeHandle) {
      this.resizeHandle.addEventListener('pointerdown', this.onResizeStart.bind(this));
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
    try { this.root.focus(); } catch (_e) {}
    this.ensureInView();
    PopupWindow.openStack.push(this);
  }

  close() {
    if (!this.isShown()) return;
    this.root.classList.remove('show');
    if (this.menuButton) this.menuButton.setAttribute('aria-expanded', 'false');
    if (typeof this.onClose === 'function') this.onClose();
    const idx = PopupWindow.openStack.lastIndexOf(this);
    if (idx !== -1) PopupWindow.openStack.splice(idx, 1);
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
    try { this.root.releasePointerCapture(this.dragCaptureId); } catch (_e) {}
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
    try { this.root.releasePointerCapture(this.resizeCaptureId); } catch (_e) {}
    this.root.removeEventListener('pointermove', this.onResizeMoveBound);
    this.root.removeEventListener('pointerup', this.onResizeEndBound);
  }

  static closeTopMost() {
    const top = PopupWindow.openStack[PopupWindow.openStack.length - 1];
    if (top) top.close();
  }
}
