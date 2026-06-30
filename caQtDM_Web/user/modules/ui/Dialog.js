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

export class Dialog {
  constructor(opts) {
    this.overlay = opts.overlay;
    this.messageElement = opts.messageElement;
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
