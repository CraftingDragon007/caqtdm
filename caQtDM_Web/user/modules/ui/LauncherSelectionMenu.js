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

import { closeAllMenus } from './LauncherMenu.js';

export function setupLauncherSelectionMenu(launcherObject, onSelect) {
  const selectionButton = document.getElementById('launcher-selection');
  if (!selectionButton) return;

  if (!launcherObject || !Object.prototype.hasOwnProperty.call(launcherObject, 'file-choice')) {
    return;
  }

  const fileChoices = Array.isArray(launcherObject['file-choice'])
    ? launcherObject['file-choice']
    : [];

  if (fileChoices.length === 0) {
    return;
  }

  selectionButton.style.display = 'inline-block';
  removeExistingMenu(selectionButton);
  const rootChoice = buildRootChoice(launcherObject);
  const menuElement = buildMenuElement(fileChoices, rootChoice, onSelect);
  selectionButton.appendChild(menuElement);

  selectionButton.onclick = (event) => {
    event.stopPropagation();
    const isShown = menuElement.classList.contains('show');
    closeAllMenus();
    if (!isShown) {
      menuElement.classList.add('show');
      selectionButton.setAttribute('aria-expanded', 'true');
    } else {
      selectionButton.setAttribute('aria-expanded', 'false');
    }
  };
}

function buildMenuElement(fileChoices, rootChoice, onSelect) {
  const menuElement = document.createElement('div');
  menuElement.id = 'launcher-selection-menu';
  menuElement.className = 'popup-menu';
  menuElement.setAttribute('role', 'menu');
  menuElement.setAttribute('aria-hidden', 'true');

  const items = rootChoice ? [rootChoice, ...fileChoices] : fileChoices;
  items.forEach((choice) => {
    const item = createChoiceItem(choice, onSelect);
    if (item) menuElement.appendChild(item);
  });

  return menuElement;
}

function createChoiceItem(choice, onSelect) {
  if (!choice) return null;
  const fileName = String(choice.file || '').trim();
  if (!fileName) return null;
  const label = String(choice.text || choice.file || '').trim();

  const button = document.createElement('button');
  button.className = 'menu-item menu-action';
  button.setAttribute('role', 'menuitem');
  button.textContent = label || fileName;

  button.addEventListener('click', (event) => {
    event.stopPropagation();
    closeAllMenus();
    if (onSelect) onSelect(fileName);
  });

  return button;
}

function buildRootChoice(launcherObject) {
  const title = String(launcherObject?.['menu-title']?.text || '').trim();
  if (!title) return null;
  return { file: 'root', text: title };
}

function removeExistingMenu(selectionButton) {
  const existingMenu = selectionButton.querySelector('#launcher-selection-menu');
  if (existingMenu) existingMenu.remove();
}
