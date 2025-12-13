# Graphics Manager

**Location**: `ui/shared_ui/managers/GraphicsManager/`
**Status**: ✅ Implemented
**Version**: 1.0.0
**Date**: 2025-12-14

---

## Overview

The **Graphics Manager** manages 2D graphic assets including vector graphics (SVG), icons, logos, overlays, and UI elements for use in Neural Studio scenes and interfaces.

### Key Features
- **Vector Graphics (SVG)**: Scalable resolution-independent graphics
- **Raster Graphics**: PNG, JPG icons and images
- **Icon Libraries**: Organize icons by category
- **Overlay Support**: Lower-thirds, watermarks, borders
- **Multi-Node Tracking**: Track graphic usage
- **Preset Management**: Save graphic configurations

---

## Architecture

### Backend (`GraphicsManagerController.h/cpp`)
- **Inheritance**: `BaseManagerController` → `QObject`
- **Responsibilities**:
  - Scanning `/assets/graphics/` directory
  - SVG parsing and validation
  - Rasterization for thumbnails
  - Tracking graphic usage in overlay nodes
  - Preset management

### Frontend Options

#### QML View (`GraphicsManager.qml`)
- **Purpose**: Blueprint Frame graphics library
- **Features**:
  - Categorized graphic list (icons, logos, overlays)
  - SVG/raster badges
  - Live previews with transparency
  - Drag-to-add to scene
  - "In Use" indicators

#### Qt Widget (`GraphicsManagerWidget.h/cpp`)
- **Purpose**: Active Frame graphics browser
- **Features**:
  - Searchable graphics library
  - Category filters
  - Preview pane
  - Import graphic button

---

## Interface

### Controller Properties
| Property | Type | Description |
|----------|------|-------------|
| `graphicFiles` | QVariantList | Discovered graphics with metadata |
| `managedNodes` | QStringList | Overlay nodes using graphics |
| `nodePresets` | QVariantList | Saved overlay configs |

### Methods
| Method | Parameters | Description |
|--------|------------|-------------|
| `scanGraphics()` | - | Rescan graphics directory |
| `getGraphicMetadata()` | graphicPath | Extract SVG/image info |
| `rasterizeSVG()` | svgPath, width, height | Render SVG to bitmap |
| `createGraphicNode()` | graphicPath, x, y | Create overlay node |
| `savePreset()` | nodeId, presetName | Save overlay config |

---

## Supported Formats

### Vector Graphics
- **SVG** (.svg) - Scalable Vector Graphics
  - Full SVG 1.1 support
  - CSS styling
  - Animations (planned)
  - Text rendering

### Raster Graphics
- **PNG** (.png) - Transparency support
- **JPG** (.jpg) - Photo overlays
- **WebP** (.webp) - Modern format
- **GIF** (.gif) - Animated graphics

---

## Graphic Categories

### Icons
- **UI Icons**: Menu, toolbar, control icons
- **Social Icons**: Platform logos (YouTube, Twitch, etc.)
- **Custom Icons**: Project-specific icons

### Logos
- **Channel Logos**: Streamer/brand logos
- **Sponsor Logos**: Partner/sponsor branding
- **Product Logos**: Game/app logos

### Overlays
- **Lower Thirds**: Name/title cards
- **Borders**: Frame decorations
- **Watermarks**: Branding overlays
- **Alerts**: Subscriber/donation notifications

### UI Elements
- **Buttons**: Interactive UI components
- **Panels**: Background panels
- **Dividers**: Section separators

---

## Usage Example

### C++ (Controller)
```cpp
auto graphicsManager = new GraphicsManagerController(parent);
graphicsManager->scanGraphics();

// Get graphic metadata
QVariantMap metadata = graphicsManager->getGraphicMetadata(
    "/assets/graphics/logos/channel_logo.svg"
);

// Rasterize SVG for preview
QImage preview = graphicsManager->rasterizeSVG(
    "/assets/graphics/logos/channel_logo.svg", 
    256, 256
);

// Create graphic overlay node
graphicsManager->createGraphicNode(
    "/assets/graphics/overlays/lower_third.svg", 
    100, 500
);
```

### QML (Blueprint UI)
```qml
GraphicsManager {
    controller: graphicsManagerController
    
    TabBar {
        Tab { text: "Icons" }
        Tab { text: "Logos" }
        Tab { text: "Overlays" }
    }
    
    GridView {
        model: controller.graphicFiles
        delegate: GraphicCard {
            thumbnail: modelData.thumbnailPath
            graphicName: modelData.name
            isVector: modelData.format === "svg"
            dimensions: modelData.resolution
            inUse: controller.managedNodes.includes(modelData.id)
        }
    }
}
```

---

## SVG Features

### Rendering
- Anti-aliased rendering
- Transparency support
- CSS color overrides
- Text as paths or glyphs

### Optimization
- Path simplification
- Unused element removal
- Embedded image extraction
- Viewbox normalization

---

## File Organization

```
/assets/graphics/
├── icons/           # UI and decorative icons
│   ├── social/
│   ├── ui/
│   └── custom/
├── logos/           # Channel and brand logos
├── overlays/        # Stream overlays
│   ├── lower_thirds/
│   ├── borders/
│   ├── watermarks/
│   └── alerts/
└── ui_elements/     # Interface components

Presets:
~/.config/neural-studio/GraphicNode_presets.json

Thumbnail cache:
~/.cache/neural-studio/graphic_thumbs/
```

---

## Integration Points

### With Scene Graph
- Creates GraphicNode instances
- 2D rendering in 3D space
- Z-order compositing

### With UI System
- Icon theme provision
- SVG UI components
- Dynamic color themeing

---

## Future Enhancements
- [ ] SVG animation support
- [ ] Icon font integration
- [ ] Real-time SVG editing
- [ ] Graphic templates
- [ ] AI-powered icon generation
- [ ] Figma/Sketch import
