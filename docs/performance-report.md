# Photo26 v1.0 — Informe de Rendimiento y Memoria (`performance-report.md`)

**Fecha de medición:** 2026-11-23  
**Hardware de prueba:** AMD Ryzen 7 9800X3D (8 núcleos físicos / 16 hilos lógicos), Windows 11 Home x64 (Build 26200)  
**Build medido:** Windows x64 Release (`MinGW-w64 GCC 13.1.0`, C++20, `-O3`/`-O2`, Qt 6.8.3)

---

## 1. Tiempos de Compilación y Tamaño de Artefactos

| Métrica | Valor Real Medido | Notas |
| :--- | :---: | :--- |
| Configuración CMake (`Ninja`, Release) | **3,472 ms** (`3.47 s`) | Primera configuración limpia en NTFS local |
| Compilación completa Release (`37` pasos) | **22,383 ms** (`22.38 s`) | Incluye `photo26-core`, `photo26-desktop-lib`, `photo26.exe` y `photo26-tests.exe` |
| Tamaño de `photo26.exe` (PE32+ GUI x64) | **1,461,938 bytes** (`1.39 MiB`) | Binario Release con 19 herramientas y GUI completa |
| Tamaño del paquete portable (`dist/photo26/`) | **61,960,233 bytes** (`59.09 MiB`) | 37 archivos (ejecutable, 29 DLLs Qt/MinGW/plugins y 7 archivos de licencias/SBOM) |
| Tamaño del archivo distribuible `.zip` | **24,469,786 bytes** (`23.34 MiB`) | `dist/Photo26-v1.0.0-win64-portable.zip` (compresión óptima) |

---

## 2. Arranque y Consumo de Memoria de la Aplicación (`photo26.exe`)

Medido directamente en Windows 11 mediante la API nativa Win32 `GetProcessMemoryInfo` y `QElapsedTimer` desde el paquete portable (`dist/photo26/photo26.exe`) con `PATH=C:\Windows\System32;C:\Windows`:

| Escenario | Tiempo de Arranque | Duración Total | Working Set | Private Commit | Peak Working Set |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Reposo portable (`--idle-ms 1500`)** | **384 ms** | `1,884 ms` | **113.35 MiB** (`118,853,632 B`) | **51.06 MiB** (`53,538,816 B`) | `113.35 MiB` |
| **Autodiagnóstico portable (`--self-test`, 19 herramientas)** | **395 ms** | **576 ms** | **117.93 MiB** (`123,658,240 B`) | **53.31 MiB** (`55,898,112 B`) | `119.5 MiB` |
| **Reposo desde build (`--idle-ms 1500`)** | **386 ms** | `1,886 ms` | **111.35 MiB** (`116,760,576 B`) | **50.46 MiB** (`52,916,224 B`) | `111.35 MiB` |

---

## 3. Benchmarks Reales del Motor `photo26-core` (Imágenes Full HD `1920×1080`)

Medido con un benchmark nativo C++20 (`-O2`) enlazado contra `libphoto26-core.a` sobre imágenes sintéticas Full HD (`1920×1080`, `2.07 MP`) y un lote paralelo de 20 fotografías Full HD:

| Operación | Entrada / Configuración | Tiempo Medido | Resultado / Observaciones |
| :--- | :--- | :---: | :--- |
| **Carga PNG Full HD** | `1920×1080` ARGB32 | **72 ms** | Decodificación + validación `ImageLimits` |
| **Guardado WebP Full HD** | `1920×1080`, calidad `85` | **77 ms** | Escritura atómica vía `SafeFileWriter` |
| **Redimensionado Lanczos-3** | `1920×1080` → `960×540` | **32 ms** | Filtro separable de alta calidad en alfa premultiplicado |
| **Ajuste de color 16 bits (`RGBA64`)** | `1920×1080` (exposición EV + contraste + saturación + sepia) | **781 ms** | Precisión de 16 bits por canal en espacio lineal sRGB |
| **Web Optimizer (5 variantes)** | `1920×1080` → `320, 640, 960, 1280, 1920 px` WebP q82 | **262 ms** | 5 variantes generadas en memoria con limpieza de metadatos |
| **Extracción de paleta (8 colores)** | `1920×1080`, k-means++ determinista | **18 ms** | Submuestreo acotado a `65,536` píxeles |
| **Eliminación local de fondo** | `960×540`, inundación Lab + suavizado gaussiano `1.5 px` | **37 ms** | 100 % local sobre CPU |
| **Lote paralelo de 20 imágenes Full HD** | 20× `1920×1080` JPEG → Lanczos-3 `1280 px` + color + WebP q82 (`4 hilos`) | **713 ms** (`35.6 ms/img`) | `20/20` exitosas; `2,012,775 B → 525,644 B` (**-73.9 %** de peso) |
| **Exportación PKZIP (20 archivos)** | 20 archivos WebP en streaming | **78 ms** | `528,026 bytes`, CRC-32 en cabecera local |
| **Exportación PDF multipágina** | 5 páginas (`960×540`), A4, 300 DPI | **15 ms** | `60,479 bytes`, PDF válido (`%PDF-`) |

---

## 4. Estabilidad y Memoria Acotada Durante Procesamiento por Lotes

Durante el lote paralelo de **20 imágenes Full HD** con 4 hilos trabajadores:
- **Working Set antes del lote:** `54.35 MiB` (`56,987,648 bytes`)
- **Working Set tras finalizar el lote:** `55.09 MiB` (`57,769,984 bytes`) — incremento neto de solo **`+0.75 MiB`**, confirmando liberación inmediata de buffers por imagen y ausencia de fugas de memoria.
- **Pico de Working Set durante la ejecución paralela (4 hilos × pipeline 16 bits):** `364.79 MiB` (`382,509,056 bytes`), estrictamente acotado por la concurrencia limitada del `QThreadPool`.
