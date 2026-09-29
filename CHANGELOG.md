# Changelog

## [v1.3.1] - 29 Sep 2026

### English

#### Added
- Added English and Spanish interface translations with a language selector in Display Settings.
- Added sorting by internal game title, alphabetical quick navigation, a scrollable layout selector, and inverted Coverflow mode.
- Added a theme selector, custom BMP icons for games and folders, custom NDS banners, and custom folder covers using `cover.bmp`.

#### Fixed
- Kept cover artwork within the visible screen area and used the DS icon when a DSi banner has no DSi section.
- Fixed stale game-code data affecting cover lookup when a ROM has no game code.
- Improved translated settings navigation, focus retention after alphabetical jumps, and Coverflow layout labels.

### Español

#### Añadido
- Interfaz traducida al inglés y al español, con selector de idioma en los ajustes de pantalla.
- Ordenación por título interno, navegación alfabética rápida, selector de diseño desplazable y modo Coverflow invertido.
- Selector de temas, iconos BMP personalizados para juegos y carpetas, banners NDS personalizados y portadas de carpeta mediante `cover.bmp`.

#### Corregido
- Las portadas se mantienen dentro de la pantalla y se usa el icono de DS cuando un banner DSi no incluye su sección DSi.
- Corregida la búsqueda de portadas cuando una ROM no tiene código de juego, evitando reutilizar datos anteriores.
- Mejoradas la navegación de los ajustes traducidos, la conservación del foco al saltar por letras y las etiquetas del diseño Coverflow.

## [v1.3.0] - 18 Apr 2026

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