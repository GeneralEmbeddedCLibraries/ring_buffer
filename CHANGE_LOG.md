# Changelog
All notable changes to this project/module will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project/module adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## V2.2.0 - 06.06.2025

### Added
 - New API functions for get full and empty state of buffers

### Changed
 - Fixing compiler warnigns on implicit type conversions

### Fixed
 - Possible compielr out of order execution (added compiler barriers)
 - Race conditions (added atomic counter)

---
## V2.1.0 - 22.08.2023

### Added
 - New API functions to add/get many items in single call

### Changed
 - Time execution optimization
 - Cleaning code, spliting larger function blocks into smaller

### Fixed 
 - Various compiler warnings

---
## V2.0.3 - 04.11.2022

### Changed
 - Replace version notes with changelog
 - Updated readme

---
## V2.0.2 - 23.06.2022

### Changed
 - Added protection for multiple init calls
 - Added detailed description about module limitations

---
## V2.0.1 - 25.05.2022

### Changed
 - Removed unused function
 - Change return status enumeration. Now supports masking error codes
 - Removed not needed include files

---
## V2.0.0 - 31.07.2021

### Note
Complete rework of *Ring Buffer* module. Inspiration of new API was taken from CMSIS RTOS Queues.

### Added
 - Optional buffer item size 
 - FIFO support
 - Override mode 
 - Dynamic or static allocation of memory
 - Buffer name for debugging purposes
 - Two types of buffer access: NORMAL (for fifo) & INVERS (for filters)
 - Get functions to acquire name, taken items, free items, size of buffer, size of item

---
## V1.0.1 - 25.07.2021

### Added
 - Added module version
 - Added copyright notice
 - Change is_init func in to return status

### Changed
 -  *iss_init* returns status

### Todo
 - Change "get" functions to return status and return value of buffer via arguments
---

## V1.0.0 - 13.02.2022

### Added
 - Initial implementation of ring buffer
 - Supported three data types: uint32_t, int32_t and float32_t
 - Two types of buffer access: NORMAL & INVERS
---