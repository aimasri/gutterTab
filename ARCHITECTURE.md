# gutterTab Systems Architecture

This document outlines the architectural specifications, component boundaries, and systems programming patterns implemented in `gutterTab`.

---

## 1. Architectural Pattern: Domain-Driven Design (DDD)

`gutterTab` is organized into three bounded contexts to ensure strict separation of concerns, testability, and maintainability:

```
┌─────────────────────────────────────────────────────────────┐
│                     Presentation Layer                      │
│  (OverlayWindow, GutterStrip, GutterTab, NoteEditorOverlay) │
└──────────────┬───────────────────────────────▲──────────────┘
               │ Invokes methods               │ Emits Qt Signals
               ▼                               │
┌──────────────────────────────────────────────┴──────────────┐
│                        Domain Layer                         │
│             (TabController, Note Entity, State)             │
└──────────────┬──────────────────────────────────────────────┘
               │ Queries / Persists
               ▼
┌─────────────────────────────────────────────────────────────┐
│                    Infrastructure Layer                     │
│               (DatabaseManager, ConfigManager)              │
└─────────────────────────────────────────────────────────────┘
```

### 1.1 Domain Layer (`src/domain/`)
- **Responsibility**: Houses business entities and orchestrates application state transitions.
- **Components**:
  - `Note`: An entity model representing a note/tab with properties for identity, title, Markdown body, tab color, sync ID, and vertical display order (`sort_order`).
  - `TabController`: Central state machine (`enum class State { IDLE, PEEKING, OPEN }`). Maintains an in-memory cache of notes, coordinates note CRUD operations, and notifies subscribers via Qt signals (`stateChanged`, `notesLoaded`, `activeNoteChanged`).
- **Invariants**: Contains zero direct dependencies on UI widgets. Communicates with the presentation layer strictly through Qt signals and slots.

### 1.2 Infrastructure Layer (`src/infrastructure/`)
- **Responsibility**: Manages concrete persistence, external filesystem operations, and operating system interaction.
- **Components**:
  - `ConfigManager`: Manages the global profile registry (`~/.config/gutterTab/profiles.json`) and per-profile JSON configuration (`config.json`), resolving filesystem paths and settings (edge orientation, tab widths).
  - `DatabaseManager`: Implements SQLite storage (`notes.db`), managing schema creation, dynamic column migrations (`sort_order`), note CRUD queries, and atomic index reordering.
- **Invariants**: Thread-safe initialization, defensive schema verification, and atomic disk writes.

### 1.3 Presentation Layer (`src/presentation/`)
- **Responsibility**: UI rendering, event filtering, drag-and-drop processing, and X11 system integration.
- **Components**:
  - `OverlayWindow`: A full-screen transparent widget acting as the root canvas. It integrates with X11 via the XShape extension to ensure mouse click pass-through to underlying applications.
  - `GutterStrip`: A vertical layout container anchored to the screen edge that positions and stacks tabs, calculates binder overlaps, manages collapse timers, and handles note reordering.
  - `GutterTab`: Individual tab widget with rotated text rendering (-90 degrees), width animations (`QPropertyAnimation`), and contextual action menus.
  - `NoteEditorOverlay`: Slide-in editor overlay providing Markdown formatting actions and auto-saving text inputs.
  - `MarkdownEditor`: Custom `QTextEdit` subclass intercepting clipboard MIME data to save pasted images directly to the active profile's attachment directory.
  - `ProfilePickerWindow` & `SleekDialogs`: Frameless, dark-mode modal dialogs for switching profiles, editing metadata, and selecting screen edges.

---

## 2. Systems Programming & X11 Integration

### 2.1 X11 XShape Click-Through Overlay
In desktop environments like Openbox, a standard full-screen transparent window would intercept all mouse events across the entire desktop, rendering other applications unusable.

To solve this, `OverlayWindow` utilizes the **X11 Non-Rectangular Window Shape Extension (XShape)**:
1. **Window Flags**: The window is initialized with `Qt::FramelessWindowHint`, `Qt::WindowStaysOnTopHint`, and `Qt::WA_TranslucentBackground`.
2. **Input Region Calculation**: `OverlayWindow::updateInputMask()` computes a `QRegion` composed strictly of:
   - The bounding rectangle of the `GutterStrip` (or its tabs).
   - The bounding rectangle of the `NoteEditorOverlay` (when visible in `OPEN` state).
3. **Mask Application**:
   Using `XShapeCombineRectangles` (or XCB shape calls) targeted at `ShapeInput`:
   ```cpp
   XShapeCombineRectangles(
       QX11Info::display(),
       winId(),
       ShapeInput,
       0, 0,
       rectangles.data(),
       rectangles.size(),
       ShapeSet,
       YXBanded
   );
   ```
4. **Result**: Clicks falling within the active tab or editor boundaries are routed to `gutterTab`. All other mouse clicks pass directly through to desktop windows underneath.

### 2.2 Rotated Text Rendering & Geometry
`GutterTab` displays titles vertically along the screen edge:
- **Coordinate Space Transformation**: During `paintEvent`, the painter translates to `(0, height)` and rotates `-90` degrees.
- **Clipping & Metrics**: Font metrics are calculated against the rotated height, clipping overflow text smoothly.
- **Binder Overlap Cascade**: In `GutterStrip`, negative spacing and reverse Z-ordering (`raise()` / `lower()`) create a physical notebook binder tab appearance.

---

## 3. Concurrency & Event Loop Model

- **GUI Thread Affinity**: All Qt widgets, XShape mask updates, and QTimer handlers run strictly on the main thread to satisfy X11 and Qt GUI reentrancy constraints.
- **Non-Blocking Persistence**: Database queries and JSON serialization are optimized for minimal latency. SQLite operations utilize indexed primary keys and small payloads, preventing main-thread stutter.
- **State Synchronization**: Modifying data in the domain layer immediately updates in-memory structures and triggers asynchronous or atomic persistence, followed by Qt signal dispatches to refresh UI views in-place.
