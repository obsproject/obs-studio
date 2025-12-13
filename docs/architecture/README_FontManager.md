# Font Manager

**Location**: `ui/shared_ui/managers/FontManager/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **Font Manager** manages typography assets in Neural Studio, including TrueType, OpenType, and variable fonts. It provides font discovery, preview rendering, and enables text nodes with proper font loading.

### Key Features
- **Format Support**: TTF, OTF, WOFF, WOFF2, Variable Fonts
- **Font Preview**: Live preview with customizable sample text
- **Family Grouping**: Organize fonts by family (Regular, Bold, Italic)
- **Unicode Support**: Full Unicode range, emoji fonts
- **Preset Management**: Save text styling presets
- **Multi-Node Tracking**: Track all text nodes using fonts

---

## Architecture

### Backend (`FontManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/fonts/` directory
  - Font metadata extraction (family, style, weight)
  - Character set analysis
  - Tracking font usage in text nodes
  - Preset management

### Frontend Options

#### QML View (`FontManager.qml`)
- **Purpose**: Blueprint Frame font library
- **Features**:
  - Font family cards with preview text
  - Style variants (Regular, Bold, Italic, etc.)
  - Character range indicators
  - Drag-to-apply to text nodes
  - "In Use" indicators

#### Qt Widget (`FontManagerWidget.h/cpp`)
- **Purpose**: Active Frame font browser
- **Features**:
  - Searchable font list
  - Preview pane with custom text
  - Font info inspector
  - Import font button

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `fontFamilies` | QVariantList | Font families with variants |
| `managedNodes` | QStringList | Text nodes using fonts |
| `nodePresets` | QVariantList | Saved text style presets |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanFonts()` | - | Rescan fonts directory |
| `loadFont()` | fontPath | Register font with Qt |
| `getFontMetadata()` | fontPath | Extract font info |
| `savePreset()` | nodeId, presetName | Save text style |
| `loadPreset()` | presetName, x, y | Apply font preset |

---

## Supported Formats

### Font Files
- **TrueType** (.ttf) - Most common
- **OpenType** (.otf) - Advanced features
- **WOFF/WOFF2** (.woff, .woff2) - Web fonts
- **Variable Fonts** - Single file, multiple styles

### Features
- **Kerning** - Character spacing
- **Ligatures** - Combined glyphs (fi, fl)
- **OpenType Features** - Small caps, old-style numerals
- **Color Fonts** - Emoji, multi-color glyphs

---

## Usage Example

### C++ (Controller)
```cpp
auto fontManager = new FontManagerController(parent);
fontManager->scanFonts();

// Load a font
fontManager->loadFont("/assets/fonts/Roboto-Regular.ttf");

// Save text style preset
fontManager->savePreset("text_node_789", "Heading Style");
```

### QML (Blueprint UI)
```qml
FontManager {
    controller: fontManagerController
    
    ListView {
        model: controller.fontFamilies
        delegate: FontFamilyCard {
            familyName: modelData.family
            variants: modelData.variants // ["Regular", "Bold", "Italic"]
            previewText: "The quick brown fox"
            inUse: controller.isInUse(modelData.family)
        }
    }
}
```

---

## Font Metadata

```cpp
QVariantMap metadata = fontManager->getFontMetadata(fontPath);

// Available fields:
metadata["family"]      // "Roboto"
metadata["style"]       // "Regular"
metadata["weight"]      // "400"
metadata["isItalic"]    // false
metadata["isMonospace"] // false
metadata["glyphCount"]  // 3452
metadata["unicode"]     // ["Basic Latin", "Latin-1 Supplement", ...]
metadata["license"]     // "OFL" or "proprietary"
```

---

## File Organization

```
/assets/fonts/
├── sans-serif/      # Sans-serif fonts
├── serif/           # Serif fonts
├── monospace/       # Code/terminal fonts
├── display/         # Decorative/display fonts
├── handwriting/     # Script/handwriting styles
└── emoji/           # Color emoji fonts

Presets:
~/.config/neural-studio/text_style_presets.json
```

---

## Typography Features

### Text Rendering
- **Subpixel Antialiasing** - Sharp text on screens
- **Hinting** - Grid-fitted at small sizes
- **Embedded Bitmaps** - Crisp rendering for specific sizes

### Advanced Features
- **Variable Font Axes** - Weight, width, slant interpolation
- **Ligatures** - fi, fl, ff automatic replacement
- **Stylistic Sets** - Alternate character designs
- **Contextual Alternates** - Smart glyph substitution

---

## Integration Points

### With Text Nodes
- Font loading and registration
- Style application
- Real-time preview updates

### With 3D Renderer
- Text mesh generation
- Signed distance field (SDF) rendering
- Outline/shadow effects

---

## Future Enhancements
- [ ] Google Fonts integration
- [ ] Font pairing suggestions
- [ ] Custom glyph editor
- [ ] Font subsetting for web export
- [ ] Variable font axis preview
- [ ] Font similarity search
