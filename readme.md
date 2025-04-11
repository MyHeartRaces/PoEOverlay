# PoEOverlay

![Version](https://img.shields.io/badge/version-0.1.0-blue.svg)
![License](https://img.shields.io/badge/license-GPL%20v3-green.svg)
![C++](https://img.shields.io/badge/C%2B%2B-17%2F20-orange.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)

A lightweight, non-injecting browser overlay application for Path of Exile and Path of Exile 2.

## ✨ Features

- **Lightweight and Non-Intrusive**: Minimal resource usage, no game file modifications
- **Embedded Web Browser**: Access build guides, trade sites without alt-tabbing
- **Bookmark Management**: Organize your PoE resources efficiently
- **Performance Focused**: Minimal CPU and GPU impact, especially when inactive
- **Hotkey Controls**: Toggle visibility and interactivity with customizable hotkeys
- **ToS Compliant**: Fully compliant with Path of Exile's Terms of Service

---

## 📋 Table of Contents

- [Project Overview](#project-overview)
- [Technical Specifications](#technical-specifications)
- [Feature Specifications](#feature-specifications)
- [Implementation Architecture](#implementation-architecture)
- [Development Setup](#development-setup)
- [Usage Instructions](#usage-instructions)
- [Performance Considerations](#performance-considerations)
- [Security and Compliance](#security-and-compliance)
- [License and Attribution](#license-and-attribution)
- [Contribution Guidelines](#contribution-guidelines)
- [Project Status](#project-status)

---

## Project Overview

PoEOverlay is a lightweight, non-injecting overlay application for Path of Exile (PoE) and Path of Exile 2 that provides players with an embedded web browser interface without modifying game files. This tool allows players to quickly access build guides, trading sites, and other web resources without alt-tabbing out of the game.

## Technical Specifications

### Core Architecture

- **Language:** C++17/20
- **Browser Engine:** Chromium Embedded Framework (CEF)
- **Rendering:** Direct composition overlay using DWM (Desktop Window Manager)
- **Window Management:** Native Win32 API with custom window procedures
- **Configuration Storage:** JSON-based settings with local file storage
- **Build System:** CMake with vcpkg for dependency management

### Performance Requirements

- **Memory Footprint:**
  - Active: ≤200MB RAM
  - Inactive/Minimized: ≤20MB RAM
- **CPU Usage:**
  - Active: ≤2% on quad-core CPU (reference: i5-8600K)
  - Inactive/Minimized: ≤0.1% on same reference
- **GPU Impact:**
  - Active: Minimal GPU acceleration only when rendering content
  - Inactive: Zero GPU utilization

### System Requirements

- Windows 10/11 64-bit
- DirectX 11 compatible GPU
- 4GB RAM minimum (8GB recommended)
- 100MB disk space for application
- Internet connection for web browsing functionality

## Feature Specifications

### Browser Engine

- **CEF Integration:**
  - Chromium version: Latest stable CEF release
  - JavaScript support: Full ES2021 implementation
  - Renderer process isolation for stability
  - Custom resource handling for optimization

- **Browser Features:**
  - Bookmark management system with folder organization
  - History tracking (session-only by default)
  - Basic navigation controls (back, forward, refresh)
  - Custom URL bar with search integration
  - Context menu with custom game-relevant options

### Overlay Management

- **Window Behaviors:**
  - Click-through capability when inactive
  - Configurable opacity levels (25%, 50%, 75%, 100%)
  - Smart borders that highlight only when cursor approaches
  - Multi-monitor support with proper DPI awareness
  
- **Hotkey System:**
  - Global hotkeys for visibility toggle (default: Alt+B)
  - Secondary hotkey for interactive/non-interactive modes (default: Alt+I)
  - Custom hotkey binding through configuration interface
  - Hotkey conflict detection with Path of Exile

### Resource Optimization

- **Inactive State Management:**
  - Browser process suspension when minimized/hidden
  - Dynamic resource allocation based on visibility state
  - Memory trimming on minimize/hide operations
  - Render throttling when game is in active foreground

- **Startup Optimizations:**
  - Cold start under 2 seconds
  - Browser initialization deferred until needed
  - Minimal blocking operations during game activity

### User Interface

- **Main Components:**
  - Browser viewport with resizable handles
  - Minimal navigation bar (hideable)
  - Bookmark bar with folder support
  - Status indicator for overlay mode
  
- **Design Principles:**
  - Dark theme matching PoE aesthetic
  - Minimal UI footprint when not needed
  - Touch targets optimized for quick interactions
  - Consistent with Windows design language

## Implementation Architecture

### Component Structure

1. **Core Application Layer:**
   - Main application loop
   - Window management
   - Input handling
   - Settings management
   - Game process detection

2. **Overlay Management Layer:**
   - Window composition
   - Click-through handling
   - Z-order management
   - Multi-monitor support

3. **Browser Integration Layer:**
   - CEF lifecycle management
   - Renderer process isolation
   - Resource handling
   - JavaScript bridge for native functionality

4. **UI Framework Layer:**
   - Custom lightweight UI components
   - Theming system
   - Layout management
   - Input routing

5. **Storage and State Layer:**
   - Bookmark persistence
   - Configuration storage
   - Session management
   - Error logging

### Non-Injecting Methodology

The overlay utilizes completely non-invasive methods to provide overlay functionality:

- Window positioning via DWM composition
- WS_EX_LAYERED and WS_EX_TRANSPARENT window styles
- DirectComposition for hardware-accelerated transparency
- Windows event hooks for detecting game state changes
- No modification of game memory or executable files
- No interception of DirectX/OpenGL calls

## Development Setup

### Prerequisites

- Visual Studio 2019 or 2022 (Community Edition or higher)
- CMake 3.20+
- vcpkg package manager
- Git

### Environment Setup

1. Clone repository with submodules
   ```bash
   git clone --recurse-submodules https://github.com/MyHeartRaces/PoEOverlay.git
   cd PoEOverlay
   ```

2. Initialize vcpkg and install dependencies:
   ```bash
   vcpkg install cef:x64-windows
   vcpkg install nlohmann-json:x64-windows
   vcpkg install spdlog:x64-windows
   vcpkg install fmt:x64-windows
   ```

3. Configure CMake project:
   ```bash
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake
   ```

4. Build project:
   ```bash
   cmake --build build --config Release
   ```

## Usage Instructions

### Installation

1. Download latest release from GitHub
2. Extract to desired location (no installer required)
3. Run PoEOverlay.exe

### First-Time Setup

1. On first launch, configure hotkeys and default browser settings
2. Import bookmarks (optional) or use default PoE community links
3. Set default opacity and behavior preferences

### Using the Overlay

1. Launch Path of Exile
2. Press Alt+B (default) to show/hide overlay
3. Press Alt+I (default) to toggle interactive mode
4. Use browser like any standard web browser
5. Add bookmarks for frequently visited PoE resources

## Performance Considerations

### Memory Management

- CEF browser instances are expensive resources
- Browser process hibernation when not in use
- Memory trimming on minimize/hide operations
- Careful resource cleanup for long gaming sessions

### Rendering Pipeline

- Direct composition minimizes redraw impact
- Hardware acceleration only when overlay visible
- Throttled rendering based on overlay state
- Isolated rendering thread to prevent game stutters

### Process Communication

- Minimal IPC overhead between components
- Efficient message passing between browser and UI
- Reduced update frequency when inactive

## Security and Compliance

### Game Terms of Service

This overlay complies with Path of Exile's Terms of Service by:
- Not injecting code into the game process
- Not reading or writing game memory
- Not automating gameplay
- Not modifying game files or resources
- Using only approved methods for window overlay

### User Privacy

- No telemetry collection
- All browsing data stored locally
- No external authentication required
- Optional: private browsing mode for no history

## License and Attribution

- PoEOverlay is licensed under GNU General Public License v3.0
- CEF is licensed under BSD License
- Additional third-party libraries used with appropriate attribution
- No affiliation with Grinding Gear Games or Path of Exile

## Contribution Guidelines

### Code Style

- C++ Core Guidelines conformance
- Consistent naming conventions
- Comprehensive error handling
- Performance-first mindset

### Pull Request Process

1. Fork the repository
2. Create feature branch
3. Submit pull request with detailed description
4. Address code review feedback
5. Maintain test coverage

## Project Status

PoEOverlay is currently in specification phase with active development planned.

---

## 📝 License

GNU General Public License v3.0

Copyright (c) 2025 PoEOverlay Contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
