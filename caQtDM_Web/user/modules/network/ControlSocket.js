/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2026
 *
 *  Author:
 *    Julian Houba
 *  Contact details:
 *    julian.houba@psi.ch
 */

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
