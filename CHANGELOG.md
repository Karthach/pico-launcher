# Changelog

## [v1.0.2] - 25 Apr 2026

### Added
- Multi-language support via JSON files (`/_pico/lang/`)
- Language selection integrated into the Display Settings menu
- English and Spanish translations included by default

### Changed
- UI labels now dynamically load based on selected language
- Display Settings menu layout optimized for localized text

### Fixed
- Memory management issues with large JSON files on NDS hardware
- Navigation and interaction bugs in settings bottom sheets

## [v1.0.1] - 25 Apr 2026

### Changed
- Baseline for public release and nightly builds

### Added
- Ability to set the position of the top screen cover image in custom themes
- Support for fast scrolling with the L and R buttons in coverflow display mode
- Support for touch input

### Fixed
- Use after free bug with the texture load request in Label3DView. This occurred for example when spamming B in banner list mode.

## [v1.2.0] - 29 Mar 2026

### Added
- Support for cheats with Pico Loader API v3
- Hide files/dirs with hidden attribute, or with a name starting with a period
- New customization options for custom themes
    - Position of elements on the top screen
    - Text colors
    - Blend colors

### Changed
- File name on the top screen now uses marquee when too long

### Fixed
- Improve error handling for banners to better detect if a rom has a valid banner

## [v1.1.0] - 11 Jan 2026

### Added
- Support for Pico Loader API v2. This makes it possible to return to Pico Launcher from supported homebrew applications.

## [v1.0.0] - 25 Nov 2025
- Initial release