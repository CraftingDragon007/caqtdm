import RFB from '../../noVNC/core/rfb.js';

export class VNCClient {
  constructor(container, reconnectOverlay) {
    this.container = container;
    this.reconnectOverlay = reconnectOverlay;
    this.rfb = null;
    this.reconnectTimer = null;
    this.reconnectDelay = 1000;
    this.isConnected = false;
    this.noVNCPath = '30001';
  }

  setPath(path) {
    this.noVNCPath = path;
  }

  connect() {
    this.clearReconnect();

    if (this.rfb) {
      this.rfb.removeEventListener('disconnect', this.handleDisconnect.bind(this));
      try { this.rfb.disconnect(); } catch (e) {}
      this.rfb = null;
    }

    this.isConnected = false;
    this.reconnectOverlay.classList.add('show');

    let wsUrl;
    const path = this.noVNCPath;

    try {
      const tempUrl = new URL(path);
      if (tempUrl.protocol === 'ws:' || tempUrl.protocol === 'wss:') {
        wsUrl = path;
      } else {
        throw new Error("Not a WS URL");
      }
    } catch (e) {
      const wsProtocol = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
      wsUrl = wsProtocol + window.location.host + '/websockify/' + encodeURIComponent(path);
    }

    try {
      this.rfb = new RFB(this.container, wsUrl);
      this.rfb.scaleViewport = true;
      this.rfb.addEventListener('connect', this.handleConnect.bind(this));
      this.rfb.addEventListener('disconnect', this.handleDisconnect.bind(this));
    } catch (e) {
      console.error('RFB Init Failed:', e);
      this.scheduleReconnect();
    }
  }

  handleConnect() {
    this.isConnected = true;
    this.reconnectDelay = 1000;
    this.reconnectOverlay.classList.remove('show');
  }

  handleDisconnect() {
    this.isConnected = false;
    this.rfb = null;
    this.reconnectOverlay.classList.add('show');
    this.scheduleReconnect();
  }

  scheduleReconnect() {
    if (this.reconnectDelay < 0) return;
    if (this.reconnectTimer) return;
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      this.reconnectDelay = Math.min(this.reconnectDelay * 2, 10000);
      this.connect();
    }, this.reconnectDelay);
  }

  clearReconnect() {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
  }

  disconnect() {
    this.clearReconnect();
    if (this.rfb) {
      this.rfb.removeEventListener('disconnect', this.handleDisconnect.bind(this));
      try { this.rfb.disconnect(); } catch (e) {}
      this.rfb = null;
    }
  }
}
