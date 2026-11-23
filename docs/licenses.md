# Photo26 v1.0 — Licencias de Terceros e Inventario de Distribución (`licenses.md`)

**Fecha:** 2026-11-04  
**Versión del Producto:** Photo26 v1.0 (`1.0.0`)  
**Plataforma de Distribución:** Windows 10 / Windows 11 x64 (`x86_64-w64-mingw32`, Paquete Portable)  
**Directorio de Avisos Legales en el Paquete Portable:** `licenses/` ([`dist/bootstrap/licenses/`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses))  

---

## 1. Resumen Ejecutivo de Licencias del Paquete Portable

El paquete portable de **Photo26 v1.0** para Windows x64 se distribuye junto con las bibliotecas dinámicas (`.dll`) necesarias para su ejecución autónoma sin instalador. Todos los componentes de terceros incluidos cumplen rigurosamente sus respectivas licencias de software libre y de código abierto:

1. **Qt 6.8.3 (`qtbase`, `qtsvg`, `qtimageformats`):** Enlazado **dinámicamente** bajo los términos de la licencia **GNU Lesser General Public License v3.0 (`LGPL-3.0-only` / `LGPL-3.0-or-later`)** (con disponibilidad dual **GPLv3** según el módulo de Qt).
2. **Códecs de Imagen integrados en Qt 6.8.3 (`Qt6Gui` y plugins `imageformats/`):**
   - **JPEG (`qjpeg.dll` / `libjpeg-turbo`):** Licencias permisivas **IJG (Independent JPEG Group)**, **BSD-3-Clause** y **Zlib**.
   - **PNG y DEFLATE (`Qt6Gui.dll` / `libpng` + `zlib`):** Licencias permisivas **libpng License (`Libpng`)** y **zlib License (`Zlib`)**.
   - **WebP (`qwebp.dll` / `libwebp`):** Licencia permisiva **BSD-3-Clause** (Google Inc.).
   - **TIFF (`qtiff.dll` / `libtiff`):** Licencia permisiva **libtiff License** (Sam Leffler / Silicon Graphics, compatible con BSD).
   - **GIF e ICO (`qgif.dll`, `qico.dll`, `qicns.dll`, `qtga.dll`, `qwbmp.dll`):** Módulos de **Qt ImageFormats 6.8.3** bajo **LGPL-3.0** / **GPL-3.0** (complementados por los codificadores nativos propios `GifCodec` e `IcoCodec` de `photo26-core`).
3. **Runtime C/C++ de MinGW-w64 GCC 13.1.0 (`libgcc_s_seh-1.dll`, `libstdc++-6.dll`):** Distribuido bajo **GPL-3.0-or-later con la Excepción de Biblioteca de Ejecución de GCC v3.1 (`GCC-exception-3.1`)**, más las licencias permisivas del runtime **MinGW-w64** (**Zope Public License 2.1 / BSD / MIT / Dominio Público**).
4. **Biblioteca POSIX Threads para Windows (`libwinpthread-1.dll`):** Distribuida bajo licencia permisiva **MIT y BSD-3-Clause** (`winpthreads` del proyecto MinGW-w64 y Lockless Inc.).
5. **Exclusión de herramientas de desarrollo:** El paquete portable de usuario final **no incluye** compiladores (`g++.exe`, `gcc.exe`), enlazadores (`ld.exe`), cabeceras `.h`/`.hpp`, bibliotecas de importación `.a` ni herramientas de construcción (`cmake.exe`, `ninja.exe`, `windeployqt.exe`).

---

## 2. Inventario Detallado de Componentes de Terceros y Licencias

### 2.1. Bibliotecas Principales de Qt 6.8.3 (`win64_mingw`)

| Binario Distribuido | Módulo Qt 6.8.3 | Licencia Principal Aplicable | Códecs / Terceros Internos (según SBOM SPDX) | Función en Photo26 v1.0 |
| :--- | :--- | :--- | :--- | :--- |
| `Qt6Core.dll` | `qtbase` | **LGPL-3.0-only** *(o GPL-3.0-only)* | PCRE2 (`BSD-3-Clause`), Zlib (`Zlib`), Double-Conversion (`BSD-3-Clause`), Blake2 (`CC0-1.0`) | Tipos base, sistema de archivos, hilos, JSON, XML y bucle de eventos. |
| `Qt6Gui.dll` | `qtbase` | **LGPL-3.0-only** *(o GPL-3.0-only)* | `libpng` (`Libpng`), `zlib` (`Zlib`), FreeType (`FTL` / `GPL-2.0`), HarfBuzz (`MIT`), md4c (`MIT`) | Procesamiento de imágenes `QImage`, espacios de color `QColorSpace`, tipografía, `QPdfWriter` y formatos nativos (`PNG`, `BMP`, `PPM/PGM/PBM`, `XBM`, `XPM`). |
| `Qt6Widgets.dll` | `qtbase` | **LGPL-3.0-only** *(o GPL-3.0-only)* | — | Interfaz gráfica de escritorio (`photo26-desktop`: ventanas, tarjetas, lienzo interactivo, diálogos nativos). |
| `Qt6Svg.dll` | `qtsvg` | **LGPL-3.0-only** *(o GPL-3.0-only)* | — | Rasterización de gráficos vectoriales `SVG` / `SVGZ` (`QSvgRenderer`) y renderizado de iconos vectoriales. |
| `Qt6Network.dll` | `qtbase` | **LGPL-3.0-only** *(o GPL-3.0-only)* | — | Dependencia de carga de plugins de plataforma/táctiles de Qt (Photo26 v1.0 opera 100% offline y no realiza conexiones de red). |
| `Qt6PrintSupport.dll` *(si se despliega)* | `qtbase` | **LGPL-3.0-only** *(o GPL-3.0-only)* | — | Soporte de paginación e impresión documental asociado a exportación PDF. |

---

### 2.2. Plugins de Formatos de Imagen y Plataforma de Qt 6.8.3

| Ruta Relativa en el Paquete | Módulo de Origen | Licencia del Plugin | Biblioteca Subyacente y Licencia | Capacidad Aportada |
| :--- | :--- | :--- | :--- | :--- |
| `imageformats/qjpeg.dll` | `qtbase` | **LGPL-3.0-only** / **GPL-3.0** | `libjpeg-turbo` (**IJG** / **BSD-3-Clause** / **Zlib**) | Lectura y escritura acelerada por SIMD de imágenes `JPEG` (`.jpg`, `.jpeg`). |
| `imageformats/qwebp.dll` | `qtimageformats` | **LGPL-3.0-only** / **GPL-3.0** | `libwebp` (**BSD-3-Clause**, Google Inc.) | Lectura y escritura con/sin pérdida de imágenes `WebP` (`.webp`). |
| `imageformats/qtiff.dll` | `qtimageformats` | **LGPL-3.0-only** / **GPL-3.0** | `libtiff` (**libtiff License** / BSD-like) | Lectura y escritura de contenedores `TIFF` (`.tif`, `.tiff`). |
| `imageformats/qgif.dll` | `qtbase` | **LGPL-3.0-only** / **GPL-3.0** | Decodificador GIF de Qt | Lectura de archivos `GIF` (`GIF87a` / `GIF89a`). *(La escritura `GIF89a` la realiza el motor propio `GifCodec` de Photo26).* |
| `imageformats/qico.dll` | `qtbase` | **LGPL-3.0-only** / **GPL-3.0** | Codec ICO de Qt | Soporte complementario de lectura/escritura de iconos `.ico` junto a `IcoCodec`. |
| `imageformats/qsvg.dll` | `qtsvg` | **LGPL-3.0-only** / **GPL-3.0** | `Qt6Svg` (`QSvgRenderer`) | Plugin `QImageIOHandler` para lectura y rasterización de archivos `.svg` y `.svgz`. |
| `imageformats/qicns.dll` | `qtimageformats` | **LGPL-3.0-only** / **GPL-3.0** | Codec ICNS de Qt | Soporte de contenedores de iconos `.icns`. |
| `imageformats/qtga.dll` | `qtimageformats` | **LGPL-3.0-only** / **GPL-3.0** | Codec TGA de Qt | Lectura de imágenes Truevision Targa (`.tga`). |
| `imageformats/qwbmp.dll` | `qtimageformats` | **LGPL-3.0-only** / **GPL-3.0** | Codec WBMP de Qt | Lectura y escritura de mapas de bits inalámbricos monocromos (`.wbmp`). |
| `iconengines/qsvgicon.dll` | `qtsvg` | **LGPL-3.0-only** / **GPL-3.0** | `Qt6Svg` | Motor de iconos vectoriales escalables para la interfaz de usuario HiDPI. |
| `platforms/qwindows.dll` | `qtbase` | **LGPL-3.0-only** / **GPL-3.0** | Integración Win32 / Windows 10 & 11 API | Integración nativa con el gestor de ventanas de Windows, portapapeles, Drag & Drop y diálogos `Save As`. |
| `styles/qmodernwindowsstyle.dll` | `qtbase` | **LGPL-3.0-only** / **GPL-3.0** | Estilo nativo Windows 10/11 | Renderizado de controles visuales modernos de Windows. |
| `generic/qtuiotouchplugin.dll` | `qtbase` | **LGPL-3.0-only** / **GPL-3.0** | Plugin TUIO de Qt | Desplegado automáticamente por `windeployqt.exe`. |
| `networkinformation/qnetworklistmanager.dll` | `qtbase` | **LGPL-3.0-only** / **GPL-3.0** | API Windows NLM | Desplegado automáticamente por `windeployqt.exe` (inactivo en Photo26). |
| `tls/qcertonlybackend.dll`, `tls/qschannelbackend.dll` | `qtbase` | **LGPL-3.0-only** / **GPL-3.0** | API Windows Schannel | Desplegado automáticamente por `windeployqt.exe` (inactivo en Photo26). |

---

### 2.3. Bibliotecas de Ejecución (Runtime) de MinGW-w64 GCC 13.1.0

| Binario Distribuido | Proyecto de Origen | Licencia Aplicable | Texto de Licencia en `licenses/` | Descripción Técnica |
| :--- | :--- | :--- | :--- | :--- |
| `libstdc++-6.dll` | GNU Compiler Collection (GCC 13.1.0) | **GPL-3.0-or-later WITH GCC-exception-3.1** | [`GCC-RUNTIME-LIBRARY-EXCEPTION.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/GCC-RUNTIME-LIBRARY-EXCEPTION.txt) | Biblioteca estándar de C++20 (`std::vector`, `std::string`, `std::atomic`, `<algorithm>`, `<cmath>`, etc.). La Excepción de Runtime v3.1 permite su distribución junto al ejecutable compilado con GCC sin imponer términos copyleft al código propio. |
| `libgcc_s_seh-1.dll` | GNU Compiler Collection (GCC 13.1.0) | **GPL-3.0-or-later WITH GCC-exception-3.1** | [`GCC-RUNTIME-LIBRARY-EXCEPTION.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/GCC-RUNTIME-LIBRARY-EXCEPTION.txt) | Rutinas de soporte de bajo nivel del compilador GCC y manejo estructurado de excepciones de 64 bits (`SEH` en Windows x64). |
| `libwinpthread-1.dll` | Proyecto MinGW-w64 (`winpthreads`) | **MIT License / BSD-3-Clause** | [`WINPTHREADS-COPYING.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/WINPTHREADS-COPYING.txt) | Implementación de hilos POSIX (`pthread`) sobre la API nativa de Windows utilizada por `std::thread`, `std::mutex` y `std::atomic` en MinGW-w64. |
| *(Código CRT enlazado estáticamente)* | Proyecto MinGW-w64 (`mingw-w64-crt`) | **ZPL-2.1 / BSD / MIT / Dominio Público** | [`MINGW-W64-RUNTIME.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/MINGW-W64-RUNTIME.txt) | Código de arranque (`crt2.o`), `getopt`, conversión de punto flotante `gdtoa` (David M. Gay / Lucent) y rutinas matemáticas de MinGW-w64. |

---

## 3. Archivos de Licencia y Documentos SBOM Incluidos en el Paquete

Todo paquete portable de Photo26 incorpora la carpeta `licenses/` (verificable en [`/home/jblaz/dev/photo26/dist/bootstrap/licenses/`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/)) con los siguientes textos legales íntegros y documentos de lista de materiales de software (**SBOM SPDX**):

1. **[`THIRD-PARTY-NOTICES.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/THIRD-PARTY-NOTICES.txt):** Resumen consolidado de atribuciones y avisos de componentes de terceros incluidos en el paquete portable.
2. **[`LGPL-3.0.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/LGPL-3.0.txt):** Texto oficial íntegro de la *GNU Lesser General Public License Version 3* (29 June 2007), junto con la referencia a los términos de la *GNU General Public License Version 3*.
3. **[`GCC-RUNTIME-LIBRARY-EXCEPTION.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/GCC-RUNTIME-LIBRARY-EXCEPTION.txt):** Texto oficial íntegro de la *GCC Runtime Library Exception, Version 3.1* (31 March 2009, Free Software Foundation, Inc.).
4. **[`MINGW-W64-RUNTIME.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/MINGW-W64-RUNTIME.txt):** Avisos de copyright y licencias del proyecto MinGW-w64, incluyendo `getopt` (Todd C. Miller / The NetBSD Foundation), `gdtoa` (David M. Gay / Lucent Technologies), rutinas matemáticas (Sun Microsystems / Cephes) y cabeceras importadas de Wine (`LGPL-2.1+`).
5. **[`WINPTHREADS-COPYING.txt`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/WINPTHREADS-COPYING.txt):** Avisos de copyright y licencia MIT/BSD de `winpthreads` (Copyright © 2011 mingw-w64 project y Copyright © 2010 Lockless Inc.).
6. **Documentos SBOM SPDX oficiales de Qt 6.8.3 (`licenses/qt-sbom/`):**
   - **[`qtbase-6.8.3.spdx`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/qt-sbom/qtbase-6.8.3.spdx):** Inventario oficial en formato estándar `SPDX-2.3` de todos los módulos, plugins y dependencias internas compiladas dentro de `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `qwindows.dll`, `qjpeg.dll`, `qgif.dll` y `qico.dll`.
   - **[`qtsvg-6.8.3.spdx`](file:///home/jblaz/dev/photo26/dist/bootstrap/licenses/qt-sbom/qtsvg-6.8.3.spdx):** Inventario oficial en formato estándar `SPDX-2.3` de `Qt6Svg.dll`, `qsvg.dll` y `qsvgicon.dll`.

---

## 4. Cumplimiento de las Condiciones de Enlace Dinámico (LGPLv3)

Para garantizar el pleno cumplimiento de la sección 4 de la licencia **GNU LGPL v3.0** respecto a las bibliotecas de **Qt 6.8.3**:
- **Enlace dinámico exclusivo (`.dll`):** El ejecutable de Photo26 enlaza con Qt 6.8.3 exclusivamente mediante bibliotecas compartidas dinámicas (`Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Svg.dll`, etc.) situadas en el mismo directorio del paquete portable.
- **Posibilidad de sustitución por el usuario:** Cualquier usuario puede reemplazar libremente las DLLs de Qt 6.8.3 o sus plugins en los subdirectorios `platforms/`, `styles/`, `iconengines/` e `imageformats/` por versiones modificadas y compatibles a nivel de interfaz binaria de aplicación (ABI `win64_mingw` GCC 13.1.0 x86_64).
- **Preservación de avisos legales:** Se distribuyen de forma visible e inalterada todos los textos de licencia (`LGPL-3.0.txt`, `THIRD-PARTY-NOTICES.txt` y los archivos SPDX en `qt-sbom/`).
