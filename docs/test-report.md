# Photo26 v1.0 — Informe de Pruebas y Verificación (`test-report.md`)

**Fecha de ejecución:** 2026-11-23  
**Entorno verificado:** Windows 11 Home x64 (Build 26200), AMD Ryzen 7 9800X3D  
**Compilador y framework:** MinGW-w64 GCC 13.1.0 (`C++20`), Qt 6.8.3 (`win64_mingw`), CMake 3.30.5, Ninja 1.12.1  

---

## 1. Resumen Ejecutivo

| Suite / Verificación | Modo de Ejecución | Resultado | Casos / Herramientas | Duración | Código de Salida |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **`CTest` (`photo26-tests`)** | `-platform offscreen` (Release) | **PASS** | `1/1` suite (`10/10` bloques) | `0.53 s` | `0` |
| **`photo26-tests.exe` (QtTest)** | `-platform offscreen` (Release) | **PASS** | `10 passed, 0 failed, 0 skipped` | `450 ms` | `0` |
| **`photo26-tests.exe` (GUI Nativa)** | `qwindows.dll` nativo Windows 11 | **PASS** | `10 passed, 0 failed, 0 skipped` | `497 ms` | `0` |
| **`photo26.exe --self-test` (Build)** | GUI Nativa Windows 11 (`qwindows.dll`) | **PASS** | `19/19` herramientas verificadas | `619 ms` | `0` |
| **`photo26.exe --self-test` (Portable)** | `dist/photo26/` con `PATH` limpio | **PASS** | `19/19` herramientas verificadas | `576 ms` | `0` |
| **`photo26.exe --idle-ms 1500` (Portable)** | `dist/photo26/` con `PATH` limpio | **PASS** | Arranque + reposo + cierre limpio | `1,884 ms` | `0` |
| **Auditoría de DLLs PE32+ (`objdump -p`)** | `dist/photo26/` (`30` binarios PE32+) | **PASS** | `218` aristas, `0` DLLs faltantes | — | `0` |

---

## 2. Cobertura Detallada de `Photo26TestSuite` (`tests/test_photo26_all.cpp`)

| Bloque de Prueba | Estado | Cobertura Verificada |
| :--- | :---: | :--- |
| `initTestCase` | **PASS** | Inicialización de entorno QtTest. |
| `operationsRegression` | **PASS** | Recorte centrado/bordes/regla de tercios, rotación 90°/libre, redimensionado con prevención de ampliaciones, Lanczos-3/Bicúbico/Bilineal/Nearest sin halos de alfa premultiplicado, 16 bits `RGBA64`, composición/collage/mosaico, compresión en memoria con presupuesto de bytes y rechazo honesto de JPEG lossless, Web Optimizer (sin ampliaciones + bloqueo de *path traversal*), marcas de agua de imagen y texto, bordes y esquinas redondeadas, spritesheets + JSON, paleta determinista k-means++ (JSON/CSS/GPL/PNG), comparación (PSNR/RMSE/MAE y 4 modos), eliminación de fondo (bordes Lab, Chroma Key, luminancia, reemplazo sólido, disponibilidad honesta ONNX), renombrado masivo con detección de conflictos y copia no destructiva, procesamiento por lotes (presets JSON, aislamiento de errores, cancelación cooperativa, protección contra colisiones). |
| `operationVariants` | **PASS** | Todas las relaciones de aspecto (`1:1`, `3:4`, `4:3`, `16:9`, `9:16`, `2:3`, `3:2`, `21:9`, personalizada), rotaciones `90°/180°/270°`, volteos H/V, modos de redimensionado (porcentaje y dimensiones máximas), 4 disposiciones de combinación con fondo transparente, todos los filtros de color (sepia, brillo, contraste, saturación, exposición EV, B/N) y las 5 anchuras responsivas del Web Optimizer (`320..1920`). |
| `formatsAndIo` | **PASS** | Detección de cabeceras binarias (`png`, `jpg`, `webp`, `gif`, `bmp`, `tiff`, `ico`, `svg`, `heic`, `avif`), round-trip completo de escritura y lectura en `png`, `jpg`, `webp`, `bmp`, `tiff`, `gif`, `ico`, `ppm`, aplanado de transparencia sobre fondo en formatos sin alfa, rechazo honesto de escritura bitmap→SVG (`UnsupportedFormat`), rasterización real de SVG a `60×40`, bloqueo por límites (`maxWidth`), archivos inexistentes y archivos corruptos (`broken.png`). |
| `safeZipAndDocuments` | **PASS** | Escritura atómica `SafeFileWriter`, protección contra sobrescritura (`ConflictDetected`) manteniendo intacto el original, sugerencia de nombres (`foto_recortada.png`), `writeWithCallback`, contenedor PKZIP con verificación exacta del vector canónico CRC-32 (`2639f4cb` para `"123456789"` en la cabecera local), exportación PDF de una página y multipágina con cabecera `%PDF-`. |
| `iconAndAnimation` | **PASS** | Codificación y lectura `.ico` multi-resolución (`16, 32, 48 px`), paquete Favicon completo (`.ico` + 6 PNGs + `site.webmanifest` JSON), codificación animada `GIF89a`, extracción de fotogramas y exportación a carpeta. |
| `metadata` | **PASS** | Serialización e importación JSON `photo26.metadata.v1` de los 15 campos (incluyendo GPS e ICC), rechazo de JSON inválido/esquema erróneo (`SchemaValidationError`), advertencias de formato incompatible (`gif`), escritura e inspección en `jpg`, `png`, `webp` y `tiff`, copia de metadatos entre archivos, actualización por lotes, limpieza total y limpieza selectiva por categorías. |
| `colorProfiles` | **PASS** | Inspección y conversión real entre perfiles `sRGB`, `Adobe RGB`, `Display P3` y `ProPhoto RGB`. |
| `widgetsAndWindow` | **PASS** | `ToolCardGridWidget` (19 herramientas, búsqueda, iconos por tema), `ThemeManager` (Dark/Light), `InteractiveCanvasWidget` (vista dividida Antes/Después y recorte interactivo), `ExportBarWidget` (sufijos y nombres descriptivos), `MainWindow` (activación de las 19 herramientas, carga múltiple, exportación individual, exportación a carpeta, exportación ZIP y ejecución completa de `runSelfTestRoutine`). |
| `cleanupTestCase` | **PASS** | Cierre limpio sin fugas ni errores. |

---

## 3. Defectos Detectados y Corregidos Durante la Validación Windows

1. **Bloqueo de manejadores de archivo en Windows (`SafeFileWriter`)**: `QTemporaryFile` mantiene abierto el manejador nativo en Windows incluso tras llamar a `.close()`, lo que provocaba fallos en `QSaveFile::commit()` y `MoveFileExW` al escribir sin sobrescritura o en callbacks anidados (`gif`, `metadata`, `self-test`). **Corregido** mediante rutas temporales únicas con `QUuid`, limpieza RAII (`QScopeGuard`) y publicación atómica con `MoveFileExW(..., MOVEFILE_WRITE_THROUGH)` sin handles abiertos.
2. **Manejador `QImageReader` abierto al actualizar TIFF (`MetadataEngine::writeMetadataToFile`)**: `QImageReader` mantenía bloqueado el archivo TIFF mientras `SafeFileWriter` intentaba confirmarlo en Windows. **Corregido** cerrando el ámbito de `QImageReader` antes de invocar la escritura atómica.
3. **Cabecera local PKZIP (`ZipWriter`)**: se actualizó para parchear directamente `CRC-32` y tamaños en la cabecera local (`flag 0x0800`) tras el streaming, maximizando compatibilidad con Windows Explorer y cumpliendo el vector de prueba canónico.
4. **Coherencia de diálogos de guardado (`MainWindow` y `ExportBarWidget`)**: se unificó el uso de `lastExportDirectory()` en todos los diálogos de exportación/importación y se ajustó el aviso de conflictos del diálogo de carpeta para reflejar fielmente la protección no destructiva.

---

## 4. Verificación del Paquete Portable sin SDK

Con `PATH=C:\Windows\System32;C:\Windows` (sin Qt SDK, sin MinGW y sin Python en `PATH`):
- `dist/photo26/photo26.exe --self-test`: **EXIT 0**, `ok: true`, **19/19 herramientas** verificadas (activación, panel de parámetros, carga de imagen, vista previa, exportación, guardado repetido sin reprocesar y relectura PNG).
- `dist/photo26/photo26.exe --idle-ms 1500`: **EXIT 0**, `ok: true`.
- `objdump -p` sobre los 30 binarios PE32+ (`photo26.exe` + 29 DLLs): **0 dependencias faltantes** fuera de `C:\Windows\System32`.
