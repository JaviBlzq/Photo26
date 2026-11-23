# Photo26 — Catálogo y Estrategia de Dependencias (`dependencies.md`)

**Fecha:** 2026-11-04  
**Manifiestos reproducibles en la raíz del repositorio:**
- [`/home/jblaz/dev/photo26/vcpkg.json`](file:///home/jblaz/dev/photo26/vcpkg.json) (Baseline Git fijado: `eef0ef7bf4b2c0ddbf7be1a776f1bede94dfc861`)
- [`/home/jblaz/dev/photo26/conanfile.txt`](file:///home/jblaz/dev/photo26/conanfile.txt) (Conan 2.34.0 con `CMakeDeps` y `CMakeToolchain`)

---

## 1. Estado de las Bibliotecas de Procesamiento e Interfaz para Photo26

| Biblioteca / Capacidad | Propósito en Photo26 | Estado Actual | Versión en `conanfile.txt` / SDK | Paquete en `vcpkg.json` | Estrategia de Integración para Windows x64 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Qt 6 (`qtbase`)** | UI nativa (`Qt6::Widgets`, `Gui`, `Core`, `Test`, `Concurrent`) | **Instalado y verificado** en Windows SDK | `6.8.3` (SDK) / `qt/6.8.2` (Conan) | `qtbase` | Instalado en `C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64` y desplegado en `dist/bootstrap/` |
| **Soporte SVG (`qtsvg`)** | Renderizado de iconos y gráficos vectoriales SVG | **Instalado y verificado** en Windows SDK | `6.8.3` (`Qt6Svg.dll`, `qsvg.dll`, `qsvgicon.dll`) | `qtsvg` | Desplegado en `dist/bootstrap/Qt6Svg.dll`, `imageformats/qsvg.dll` e `iconengines/qsvgicon.dll` |
| **GIF e ICO (`qtimageformats`)** | Lectura/escritura de GIF, ICO, ICNS, TGA, WBMP | **Instalado y verificado** en Windows SDK | `6.8.3` (`qgif.dll`, `qico.dll`, `qicns.dll`, `qtga.dll`) | `qtimageformats` | Desplegado en `dist/bootstrap/imageformats/` |
| **libjpeg-turbo** | Decodificación/codificación JPEG acelerada por SIMD | Plugin Qt (`qjpeg.dll`) **instalado**; biblioteca dev fijada en manifiesto | `libjpeg-turbo/3.1.0` | `libjpeg-turbo` | Lectura/escritura básica ya disponible vía `qjpeg.dll`; API directa C/C++ vía `vcpkg`/`Conan` para el motor de Photo26 |
| **libpng** | Decodificación/codificación PNG de 8/16 bits | Integrado en `Qt6Gui.dll`; biblioteca dev fijada en manifiesto | `libpng/1.6.47` | `libpng` | Soporte nativo en `Qt6Gui` + librería directa en manifiesto reproducible |
| **libwebp** | Soporte WebP con y sin pérdida / animación | Plugin Qt (`qwebp.dll`) **instalado**; biblioteca dev fijada en manifiesto | `libwebp/1.5.0` | `libwebp` | Disponible ya en `dist/bootstrap/imageformats/qwebp.dll` + librería directa en manifiesto |
| **libtiff** | Soporte TIFF multicapa y 16/32 bits | Plugin Qt (`qtiff.dll`) **instalado**; biblioteca dev fijada en manifiesto | `libtiff/4.7.0` | `tiff` | Disponible ya en `dist/bootstrap/imageformats/qtiff.dll` + librería directa en manifiesto |
| **libvips** | Pipeline de procesamiento de imagen por tiles y bajo consumo RAM | Fijado en manifiesto reproducible para fase del motor | `libvips/8.16.1` | `libvips` | Integración en Fase de Motor mediante `vcpkg` (`x64-mingw-dynamic`) o binarios oficiales `libvips-dev-w64` |
| **libheif** | Soporte HEIC / HEIF / AVIF | Fijado en manifiesto reproducible | `libheif/1.19.7` | `libheif` | Integración mediante `vcpkg` / `Conan` junto al pipeline de `libvips` |
| **Exiv2** | Lectura/escritura de metadatos EXIF, IPTC y XMP | Fijado en manifiesto reproducible | `exiv2/0.28.5` | `exiv2` | Integración directa con CMake (`find_package(exiv2 CONFIG REQUIRED)`) |
| **LittleCMS (`lcms2`)** | Gestión de color e interpretación de perfiles ICC | Fijado en manifiesto reproducible | `lcms/2.16` | `lcms` | Integración directa con CMake (`find_package(lcms2 CONFIG REQUIRED)`) |
| **Soporte PDF** | Importación y rasterización de páginas PDF | Fijado en manifiesto reproducible (`pdfium` / `poppler`) | `poppler/24.12.0` | `pdfium` (y extensión `qtpdf` disponible en `aqt` 6.8.3) | `qtpdf` (`aqt install-qt ... -m qtpdf`) o `pdfium` / `poppler` vía gestor de paquetes |
| **ONNX Runtime** | Eliminación de fondos local e inferencia IA en CPU/DirectML | Fijado en manifiesto reproducible | `onnxruntime/1.20.1` | `onnxruntime` | Integración mediante paquete binario oficial de Microsoft (`Microsoft.ML.OnnxRuntime.DirectML` / `onnxruntime-win-x64`) o `vcpkg` |

---

## 2. Dependencias de Runtime Verificadas en `photo26-bootstrap.exe`

Mediante inspección con `objdump -p`, el ejecutable `photo26-bootstrap.exe` (Release Windows x64) importa directamente:

### 2.1. Bibliotecas Empaquetadas en `/home/jblaz/dev/photo26/dist/bootstrap/`
1. **Bibliotecas principales Qt 6.8.3 (`win64_mingw`)**:
   - `Qt6Core.dll` (`6,986,928` bytes)
   - `Qt6Gui.dll` (`11,041,960` bytes)
   - `Qt6Widgets.dll` (`7,025,320` bytes)
   - `Qt6Svg.dll` (`510,120` bytes — requerida por el plugin `iconengines/qsvgicon.dll`)
   - `Qt6Network.dll` (`2,012,840` bytes — requerida por el plugin `generic/qtuiotouchplugin.dll`)
2. **Runtime C/C++ y POSIX Threads de MinGW-w64 GCC 13.1.0**:
   - `libgcc_s_seh-1.dll` (`109,056` bytes)
   - `libstdc++-6.dll` (`2,243,072` bytes)
   - `libwinpthread-1.dll` (`53,248` bytes)
3. **Plugins de Qt 6.8.3 (16 DLLs en subdirectorios)**:
   - `platforms/qwindows.dll` (`1,102,008` bytes)
   - `styles/qmodernwindowsstyle.dll` (`240,312` bytes)
   - `iconengines/qsvgicon.dll` (`73,400` bytes)
   - `imageformats/qgif.dll`, `qicns.dll`, `qico.dll`, `qjpeg.dll`, `qsvg.dll`, `qtga.dll`, `qtiff.dll`, `qwbmp.dll`, `qwebp.dll`
   - `generic/qtuiotouchplugin.dll`
   - `networkinformation/qnetworklistmanager.dll`
   - `tls/qcertonlybackend.dll`, `tls/qschannelbackend.dll`

### 2.2. DLLs del Sistema Operativo Windows (Provistas por Windows 10/11)
Las únicas dependencias externas al directorio `dist/bootstrap/` son DLLs estándar de `C:\Windows\System32`:
`KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `SHELL32.dll`, `OLE32.dll`, `OLEAUT32.dll`, `ADVAPI32.dll`, `COMDLG32.dll`, `IMM32.dll`, `SETUPAPI.dll`, `SHCORE.dll`, `SHLWAPI.dll`, `UXTHEME.dll`, `VERSION.dll`, `WINMM.dll`, `WS2_32.dll`, `WTSAPI32.dll`, `DWMAPI.dll`, `D3D11.dll`, `D3D9.dll`, `DXGI.dll`, `DWRITE.dll`, `BCRYPT.dll`, `CRYPT32.dll`, `NCRYPT.dll`, `SECUR32.dll`, `AUTHZ.dll`, `DNSAPI.dll`, `IPHLPAPI.dll`, `MPR.dll`, `NETAPI32.dll`, `USERENV.dll`, `WINHTTP.dll`, `msvcrt.dll` y `api-ms-win-core-*`.

**Resultado de la auditoría de grafo de dependencias (`objdump -p` sobre los 25 binarios PE32+):**
- Binarios auditados: **25** (1 `.exe` + 24 `.dll`)
- Aristas de importación verificadas: **185**
- DLLs faltantes no pertenecientes a Windows: **0**

---

## 3. Licencias y Cumplimiento en el Paquete Portable

En `/home/jblaz/dev/photo26/dist/bootstrap/licenses/` se incluyen:
- `THIRD-PARTY-NOTICES.txt`: Aviso consolidado de componentes de terceros.
- `LGPL-3.0.txt`: Texto íntegro de la licencia GNU Lesser General Public License v3.0 aplicable a las bibliotecas dinámicas de Qt 6.8.3.
- `GCC-RUNTIME-LIBRARY-EXCEPTION.txt`: Excepción de biblioteca de ejecución de GCC v3.1 para `libstdc++-6.dll` y `libgcc_s_seh-1.dll`.
- `MINGW-W64-RUNTIME.txt`: Licencias y avisos de copyright del runtime de MinGW-w64.
- `WINPTHREADS-COPYING.txt`: Licencia MIT/BSD de `libwinpthread-1.dll`.
- `qt-sbom/qtbase-6.8.3.spdx` y `qt-sbom/qtsvg-6.8.3.spdx`: Documentos SBOM SPDX oficiales de Qt 6.8.3.
