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

export function parseLogMarkup(raw) {
  const fragment = document.createDocumentFragment();
  const root = document.createElement('span');
  fragment.appendChild(root);

  const stack = [root];
  const safeText = String(raw == null ? '' : raw);
  const tagRegex = /<\s*\/?\s*[a-zA-Z][^>]*>/g;
  let lastIndex = 0;

  function appendText(text) {
    if (!text) return;
    stack[stack.length - 1].appendChild(document.createTextNode(text));
  }

  function normalizeColor(value) {
    const trimmed = String(value || '').trim();
    if (!trimmed) return null;
    const hexMatch = trimmed.match(/^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6})$/);
    if (hexMatch) return trimmed;
    if (/^[a-zA-Z]+$/.test(trimmed)) return trimmed;
    return null;
  }

  function parseTag(tag) {
    const isClosing = /^<\s*\//.test(tag);
    const nameMatch = tag.match(/^<\s*\/?\s*([a-zA-Z]+)/);
    const tagName = nameMatch ? nameMatch[1].toLowerCase() : null;
    return { isClosing, tagName, raw: tag };
  }

  function parseFontColor(tag) {
    const attrMatch = tag.match(/color\s*=\s*("([^"]*)"|'([^']*)'|([^\s>]+))/i);
    if (!attrMatch) return null;
    return normalizeColor(attrMatch[2] || attrMatch[3] || attrMatch[4]);
  }

  let match;
  while ((match = tagRegex.exec(safeText)) !== null) {
    const tag = match[0];
    appendText(safeText.slice(lastIndex, match.index));
    lastIndex = match.index + tag.length;

    const parsed = parseTag(tag);
    if (!parsed.tagName) {
      appendText(tag);
      continue;
    }

    if (parsed.isClosing) {
      if (parsed.tagName === 'b' || parsed.tagName === 'font') {
        if (stack.length > 1) {
          const last = stack[stack.length - 1];
          if (last.dataset && last.dataset.tagName === parsed.tagName) {
            stack.pop();
          }
        }
      } else {
        appendText(tag);
      }
      continue;
    }

    if (parsed.tagName !== 'b' && parsed.tagName !== 'font') {
      appendText(tag);
      continue;
    }

    if (parsed.tagName === 'b') {
      const strong = document.createElement('strong');
      strong.dataset.tagName = 'b';
      stack[stack.length - 1].appendChild(strong);
      stack.push(strong);
      continue;
    }

    if (parsed.tagName === 'font') {
      const span = document.createElement('span');
      const color = parseFontColor(tag);
      if (color) span.style.color = color;
      span.dataset.tagName = 'font';
      stack[stack.length - 1].appendChild(span);
      stack.push(span);
      continue;
    }
  }

  appendText(safeText.slice(lastIndex));

  const output = document.createDocumentFragment();
  while (root.firstChild) {
    output.appendChild(root.firstChild);
  }
  return output;
}

export function formatDateTime(date) {
  const day = String(date.getDate()).padStart(2, '0');
  const month = String(date.getMonth() + 1).padStart(2, '0');
  const year = date.getFullYear();
  const hours = String(date.getHours()).padStart(2, '0');
  const minutes = String(date.getMinutes()).padStart(2, '0');
  const seconds = String(date.getSeconds()).padStart(2, '0');

  return `${day}-${month}-${year} ${hours}:${minutes}:${seconds}`;
}
