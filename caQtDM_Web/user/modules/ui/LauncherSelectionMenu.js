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
