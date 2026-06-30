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

import { applyStyleToElement } from '../utils/StyleConverter.js';

export function setupLauncherMenu(launcherObject) {
  if (!launcherObject) return;
  const launcherButton = document.getElementById('launcher');
  if (!launcherButton) return;
  const titleColor = launcherObject['menu-title']?.style?.split(': ')[1] || '#000000';
  const titleText = launcherObject['menu-title']?.text || 'Launcher';

  try { launcherButton.style.color = titleColor; } catch (_e) {}

  if (launcherButton.style.color === '#000000' || launcherButton.style.color === 'rgb(0, 0, 0)' || launcherButton.style.color.toLowerCase() === 'black') {
    launcherButton.style.color = '#1e293b'; // dark slate instead of black
  }

  launcherButton.textContent = titleText;

  if (launcherObject.menu && Array.isArray(launcherObject.menu) && launcherObject.menu.length > 0) {
    launcherButton.style.display = 'inline-block';
    createPopupPullDownMenu(launcherObject, launcherButton);
  } else {
    launcherButton.style.display = 'none';
  }
}

function createPopupPullDownMenu(object, parentElement) {
  const existingMenu = parentElement.querySelector('#launcher-menu');
  if (existingMenu) existingMenu.remove();

  const menuElement = document.createElement('div');
  menuElement.id = 'launcher-menu';
  menuElement.className = 'popup-menu';
  menuElement.setAttribute('role', 'menu');
  menuElement.setAttribute('aria-hidden', 'true');

  if (object.menu && Array.isArray(object.menu)) {
    buildMenuItems(object.menu, menuElement);
  }

  parentElement.appendChild(menuElement);

  parentElement.onclick = function(e) {
    e.stopPropagation();
    const isShown = menuElement.classList.contains('show');
    closeAllMenus();
    if (!isShown) {
      menuElement.classList.add('show');
    }
  };
}

function buildMenuItems(menuArray, containerElement) {
  menuArray.forEach(function(item) {
    const element = createMenuElement(item);
    if (element) {
      containerElement.appendChild(element);
    }
  });
}

function createMenuElement(item) {
  if (!item || !item.type) return null;
  if (item.type === 'separator') return createSeparator();
  if (item.type === 'title') return createTitleItem(item);
  if (item.type === 'menu') return createSubmenuItem(item);
  if (item.type === 'cmd') return createCommandItem(item);
  if (item.type === 'caqtdm' || item.type === 'medm' || item.type === 'pep') {
    return createActionItem(item);
  }
  return null;
}

function createSeparator() {
  const separator = document.createElement('hr');
  separator.className = 'menu-separator';
  return separator;
}

function createTitleItem(item) {
  const titleItem = document.createElement('div');
  titleItem.className = 'menu-title';
  titleItem.textContent = item.text || '';
  applyMenuMeta(titleItem, item);
  return titleItem;
}

function createSubmenuItem(item) {
  const menuButton = document.createElement('button');
  menuButton.className = 'menu-item menu-submenu';
  menuButton.setAttribute('role', 'menuitem');
  menuButton.setAttribute('aria-haspopup', 'true');
  menuButton.textContent = item.text || '';
  applyMenuMeta(menuButton, item);

  const submenu = document.createElement('div');
  submenu.className = 'popup-submenu';
  submenu.setAttribute('role', 'menu');

  if (item.menu && Array.isArray(item.menu)) {
    buildMenuItems(item.menu, submenu);
  }

  menuButton.appendChild(submenu);
  bindSubmenuEvents(menuButton, submenu);
  return menuButton;
}

function bindSubmenuEvents(menuButton, submenu) {
  menuButton.addEventListener('mouseenter', () => submenu.classList.add('show'));
  menuButton.addEventListener('mouseleave', () => submenu.classList.remove('show'));
  submenu.addEventListener('mouseenter', () => submenu.classList.add('show'));
  submenu.addEventListener('mouseleave', () => submenu.classList.remove('show'));

  menuButton.addEventListener('click', function(e) {
    e.stopPropagation();
    submenu.classList.toggle('show');
  });
}

function createActionItem(item) {
  const actionButton = document.createElement('button');
  actionButton.className = 'menu-item menu-action';
  actionButton.setAttribute('role', 'menuitem');
  actionButton.textContent = item.text || '';
  applyMenuMeta(actionButton, item);

  actionButton.addEventListener('click', function(e) {
    e.stopPropagation();
    const filePath = item.path || item.panel;
    if (filePath) {
      const newUrl = new URL(window.location.href);
      newUrl.searchParams.set('path', filePath);
      if (item.macros && item.macros.trim() !== '') {
        const convertedMacros = item.macros.replace(/=/g, ':').replace(/;/g, ',');
        newUrl.searchParams.set('macros', convertedMacros);
      }
      window.open(newUrl.toString(), '_blank', 'noopener');
    }
    closeAllMenus();
  });

  return actionButton;
}

function createCommandItem(item) {
  const cmdButton = document.createElement('button');
  cmdButton.className = 'menu-item menu-action menu-cmd';
  cmdButton.setAttribute('role', 'menuitem');
  cmdButton.textContent = item.text || '';
  applyMenuMeta(cmdButton, item);

  cmdButton.addEventListener('click', function(e) {
    e.stopPropagation();
    if (item.command) {
      window.dispatchEvent(new CustomEvent('launcher-command', { detail: item.command }));
    }
    closeAllMenus();
  });

  return cmdButton;
}

function applyMenuMeta(element, item) {
  if (item.tip) element.setAttribute('title', item.tip);
  if (item.style) {
    applyStyleToElement(element, item.style);
  }
}

export function closeAllMenus() {
  const allMenus = document.querySelectorAll('.popup-menu, .popup-submenu');
  allMenus.forEach(function(menu) {
    menu.classList.remove('show');
  });
  const viewButton = document.getElementById('menu-view');
  if (viewButton) viewButton.setAttribute('aria-expanded', 'false');
  const launcherButton = document.getElementById('launcher');
  if (launcherButton) launcherButton.setAttribute('aria-expanded', 'false');
  const launcherSelectionButton = document.getElementById('launcher-selection');
  if (launcherSelectionButton) launcherSelectionButton.setAttribute('aria-expanded', 'false');
}
