export class ControlSocket {
  constructor(options) {
    this.urlProvider = options.urlProvider;
    this.onMessage = options.onMessage;
    this.onOpen = options.onOpen;
    this.onClose = options.onClose;
    this.socket = null;
    this.heartbeat = null;
    this.reconnectTimer = null;
    this.reconnectDelay = 1000;
  }

  connect() {
    this.clearReconnect();
    const wsUrl = this.urlProvider();

    try {
      this.socket = new WebSocket(wsUrl);
    } catch (err) {
      console.error('WebSocket connection failed:', err);
      this.scheduleReconnect();
      return;
    }

    this.socket.onopen = () => {
      this.reconnectDelay = 1000;
      this.startHeartbeat();
      if (this.onOpen) this.onOpen();
    };

    this.socket.onmessage = (event) => {
      if (this.onMessage) this.onMessage(event.data);
    };

    this.socket.onerror = (error) => {
      console.warn('WebSocket error:', error);
      this.socket.close();
    };

    this.socket.onclose = () => {
      this.stopHeartbeat();
      if (this.onClose) this.onClose();
      this.scheduleReconnect();
    };
  }

  send(data) {
    if (this.socket && this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(data);
    }
  }

  startHeartbeat() {
    this.stopHeartbeat();
    this.heartbeat = setInterval(() => {
      this.send('PING');
    }, 50000);
  }

  stopHeartbeat() {
    if (this.heartbeat) {
      clearInterval(this.heartbeat);
      this.heartbeat = null;
    }
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

  disableReconnect() {
    this.reconnectDelay = -1;
    this.clearReconnect();
  }

  close() {
    this.disableReconnect();
    if (this.socket) {
      this.socket.close();
    }
  }
}
