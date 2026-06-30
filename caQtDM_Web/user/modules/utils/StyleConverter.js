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

export const QSS_PROPERTIES = {
  FONT: 'font'
};

export const FONT_PATTERNS = {
  WEIGHT: /^\d{3}$/,
  SIZE: /^\d+(\.\d+)?(pt|px|em|rem|%)$/i,
  STYLE_KEYWORDS: /^(bold|normal|italic|oblique)$/i
};

export const FONT_WEIGHTS = ['bold', 'normal'];
export const FONT_STYLES = ['italic', 'oblique'];

export function applyStyleToElement(element, styleString) {
  if (!styleString) return;

  const styleDeclarations = parseStyleString(styleString);
  styleDeclarations.forEach(function(declaration) {
    applyStyleDeclaration(element, declaration);
  });
}

function parseStyleString(styleString) {
  return styleString
    .split(';')
    .map(function(style) { return style.trim(); })
    .filter(function(style) { return style.length > 0; })
    .map(function(style) {
      const colonIndex = style.indexOf(':');
      if (colonIndex === -1) return null;

      const property = style.substring(0, colonIndex).trim();
      const value = style.substring(colonIndex + 1).trim();

      return property && value ? { property: property, value: value } : null;
    })
    .filter(function(declaration) { return declaration !== null; });
}

function applyStyleDeclaration(element, declaration) {
  const property = declaration.property;
  const value = declaration.value;

  if (property === QSS_PROPERTIES.FONT) {
    applyQSSFontProperty(element, value);
    return;
  }

  const cssProperty = convertQSSPropertyToCSS(property);
  const cssValue = convertQSSValueToCSS(value);
  element.style[cssProperty] = cssValue;
}

function convertQSSPropertyToCSS(property) {
  return property.replace(/([A-Z])/g, '-$1').toLowerCase();
}

function convertQSSValueToCSS(value) {
  if (typeof value === 'string' && value.startsWith('#')) {
    return convertToHTML5Hex(value);
  }
  return value;
}

function applyQSSFontProperty(element, fontValue) {
  if (!fontValue) return;

  const fontParts = fontValue.trim().split(/\s+/);
  const fontProperties = parseFontParts(fontParts);
  applyFontProperties(element, fontProperties);
}

function parseFontParts(parts) {
  const properties = {
    weight: null,
    style: null,
    size: null,
    families: []
  };

  parts.forEach(function(part) {
    const normalizedPart = part.toLowerCase();

    if (isFontWeight(normalizedPart, part)) {
      properties.weight = part;
    } else if (isFontStyle(normalizedPart)) {
      properties.style = part;
    } else if (isFontSize(part)) {
      properties.size = part;
    } else if (!isStyleKeyword(normalizedPart)) {
      properties.families.push(part);
    }
  });

  return properties;
}

function isFontWeight(normalizedValue, originalValue) {
  return FONT_WEIGHTS.indexOf(normalizedValue) !== -1 ||
         FONT_PATTERNS.WEIGHT.test(originalValue);
}

function isFontStyle(normalizedValue) {
  return FONT_STYLES.indexOf(normalizedValue) !== -1;
}

function isFontSize(value) {
  return FONT_PATTERNS.SIZE.test(value);
}

function isStyleKeyword(normalizedValue) {
  return FONT_PATTERNS.STYLE_KEYWORDS.test(normalizedValue);
}

function applyFontProperties(element, properties) {
  if (properties.weight) {
    element.style.fontWeight = properties.weight;
  }
  if (properties.style) {
    element.style.fontStyle = properties.style;
  }
  if (properties.size) {
    element.style.fontSize = properties.size;
  }
  if (properties.families.length > 0) {
    element.style.fontFamily = properties.families.join(', ');
  }
}

function convertToHTML5Hex(hex) {
  const hex48bitRegex = /^#([0-9a-fA-F]{4})([0-9a-fA-F]{4})([0-9a-fA-F]{4})$/;
  const match = hex.match(hex48bitRegex);

  if (match) {
    const r = match[1].substring(0, 2);
    const g = match[2].substring(0, 2);
    const b = match[3].substring(0, 2);
    return `#${r}${g}${b}`.toLowerCase();
  }

  return hex;
}
