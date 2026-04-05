# Digital Forensics QT

**Project:** Digital Forensics Desktop Tool  
**Subject:** Y2S2_URO – User Interfaces  
**Technology:** C++, Qt 6  
**Student:** SAI0044 | VŠB-TUO

---

## Goal

Build a cross-platform desktop application using Qt that assists with digital forensics tasks. The tool provides a clean graphical interface for analyzing files, extracting metadata, and visualizing forensic data.

---

## Features

- [ ] File browser with metadata viewer (size, timestamps, permissions)
- [ ] Hash computation (MD5, SHA-1, SHA-256) for file integrity verification
- [ ] Hex viewer for binary file inspection
- [ ] Search and filter functionality across forensic artifacts
- [ ] Export results to CSV / TXT report

---

## Setup

### Prerequisites

- Qt 6.x (Qt Creator recommended)
- C++17 compatible compiler (GCC / MSVC / Clang)
- CMake 3.16+

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/deavers/University_Projects.git
cd University_Projects/Y2S2_URO/Digital_Forensics_QT

# Configure and build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or open the `.pro` / `CMakeLists.txt` file directly in **Qt Creator** and click **Build**.

---

## Screenshots

> *(Screenshots will be added once the UI is implemented.)*

---

## Notes

- Project developed as part of the **User Interfaces** course at VŠB-TUO.
- All Qt-specific build artefacts are excluded via root `.gitignore`.
