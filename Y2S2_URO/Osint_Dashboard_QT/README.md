# OSINT Intelligence Dashboard

**Project:** OSINT Interactive Desktop Tool  
**Subject:** Y2S2_URO – User Interfaces  
**Technology:** C++, Qt 6  
**Student:** SAI0044 | VŠB-TUO

---

## Goal

Build a cross-platform desktop application using Qt that simulates an advanced Open-Source Intelligence (OSINT) tracking system. The tool provides a highly interactive, custom-styled graphical interface for tracking network nodes, analyzing metadata, and visualizing global data intercepts in real-time.

---

## Features

- [x] **Dynamic Home Screen:** Slowly rotating globe with a custom particle system background.
- [x] **Interactive 2D Map:** Seamless pan and zoom capabilities with accurately mapped geographic nodes.
- [x] **Custom Image Viewer:** Built-in modal lightbox with mouse-wheel zoom and drag-to-pan features for analyzing EXIF photos and routing maps.
- [x] **Rich Metadata Panels:** Dynamic HTML/CSS rendering inside Qt Widgets to display bar charts, network frequencies, and node statuses.
- [x] **Advanced Theming:** Fully custom dark-mode UI using QSS, featuring borderless floating elements, transparent overlays, and custom interactive states.

---

## Setup

### Prerequisites

- Qt 6.x (Qt Creator highly recommended)
- C++17 compatible compiler (GCC / MSVC / Clang)

### Build Instructions

```bash
# Clone the repository
git clone [https://github.com/deavers/University_Projects.git](https://github.com/deavers/University_Projects.git)
cd University_Projects/Y2S2_URO/Osint_Dashboard_QT

# If using QMake (Standard for this project)
qmake Osint_Dashboard_QT.pro
make  # or mingw32-make / nmake depending on your compiler
