# Photo26 — Informe de Auditoría del Entorno (`environment-report.md`)

**Fecha de auditoría:** 2026-11-04  
**Director Técnico:** `photo26-director`  
**Workspace:** `/home/jblaz/dev/photo26`

---

## 1. Sistema Operativo, WSL y Hardware

| Componente | Valor Verificado | Método de Verificación |
| :--- | :--- | :--- |
| **Distribución Linux (WSL)** | Ubuntu 24.04.4 LTS (Noble Numbat) | `/etc/os-release` |
| **Kernel WSL** | `Linux 6.18.33.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC` | `uname -a` |
| **Versión de WSL / WSLg** | WSL `2.7.11.0` / WSLg `1.0.73.2` (WSL 2) | `wsl.exe --version` |
| **Sistema Operativo Host (Windows)** | Microsoft Windows 11 Home (64 bits), versión `10.0.26200` (Build `26200` / `10.0.26200.9711`) | `Get-CimInstance Win32_OperatingSystem`, `cmd.exe /c ver` |
| **Arquitectura de CPU** | `x86_64` (`AMD64`, Little Endian, 64 bits) | `lscpu` |
| **Procesador (CPU)** | AMD Ryzen 7 9800X3D 8-Core Processor (8 núcleos físicos / 16 hilos lógicos) | `lscpu` |
| **Cachés de CPU** | L1d: 384 KiB (8 inst.), L1i: 256 KiB (8 inst.), L2: 8 MiB (8 inst.), L3: 96 MiB (1 inst.) | `lscpu` |
| **Instrucciones SIMD soportadas** | `SSE4.2`, `AVX`, `AVX2`, `FMA`, `AVX-512` (`avx512f`, `avx512bw`, `avx512vl`, `avx512_vnni`, `avx512_bf16`) | `lscpu` |
| **Memoria RAM asignada a WSL2** | 15 GiB totales (~11 GiB libres en reposo), Swap: 4.0 GiB | `free -h` |
| **Almacenamiento WSL (`/dev/sdd`)** | 1007 GiB totales, 949 GiB disponibles (1% usado) | `df -h` |
| **Almacenamiento Windows (`C:\`)** | Montado en `/mnt/c` (DrvFS), 1.9 TiB totales, 804 GiB disponibles | `df -h`, `ls -la /mnt/c` |

---

## 2. Estado Inicial vs. Estado Preparado de Compiladores y Herramientas

### 2.1. Entorno Linux (WSL2 Ubuntu 24.04)

| Herramienta | Estado Inicial | Estado Tras Preparación | Ruta y Versión Verificada |
| :--- | :--- | :--- | :--- |
| **GCC (`gcc`)** | Instalado | Operativo | `/usr/bin/gcc` — `13.3.0` (Ubuntu 13.3.0-6ubuntu2~24.04.1) |
| **G++ (`g++`)** | Instalado | Operativo | `/usr/bin/g++` — `13.3.0` (soporte completo C++20) |
| **Clang / Clang++** | No instalado | Pendiente de `sudo apt` (opcional) | Paquete Ubuntu candidato: `1:18.0-59~exp2` |
| **CMake (Linux)** | No instalado | **Instalado y operativo** | `/home/jblaz/.local/bin/cmake` — `4.4.4` (incluye `ctest`, `cpack`) |
| **Ninja (Linux)** | No instalado | **Instalado y operativo** | `/home/jblaz/.local/bin/ninja` — `1.13.2` |
| **Git (Linux)** | Instalado | Operativo | `/usr/bin/git` — `2.43.0` |
| **Python 3 (Linux)** | Instalado | Operativo | `/usr/bin/python3` — `3.12.3` |
| **uv (Linux)** | Instalado | Operativo | `/home/jblaz/.local/bin/uv` — `0.12.9` |
| **Conan (Linux)** | No instalado | **Instalado y operativo** | `/home/jblaz/.local/bin/conan` — `2.34.0` |
| **aqtinstall (`aqt`)** | No instalado | **Instalado y operativo** | `/home/jblaz/.local/bin/aqt` — `3.3.0` |
| **pkg-config (Linux)** | No instalado | Pendiente de `sudo apt` | Paquete Ubuntu candidato: `1.8.1-2build1` |
| **MinGW-w64 cruzado Linux** | No instalado | Suplido por MinGW-w64 Windows x64 oficial de Qt | Paquete Ubuntu candidato: `mingw-w64` (`11.0.1-3build1`) |

### 2.2. Entorno Windows x64 (Accesible desde WSL2 e Interop)

| Herramienta | Estado Inicial | Estado Tras Preparación | Ruta y Versión Verificada |
| :--- | :--- | :--- | :--- |
| **MinGW-w64 GCC/G++ x64** | No instalado | **Instalado y operativo** | `C:\Users\jblaz\.photo26-sdk\Qt\Tools\mingw1310_64\bin\g++.exe` (`x86_64-w64-mingw32-g++.exe`) — **MinGW-w64 GCC 13.1.0** (POSIX threads, SEH, MSVCRT rt_v11) |
| **MSVC (`cl.exe`)** | No instalado | No instalado (opcional) | No existe instalación en `C:\Program Files\Microsoft Visual Studio`; requiere instalador con elevación UAC si se desea en el futuro |
| **CMake (Windows x64)** | No instalado | **Instalado y operativo** | `C:\Users\jblaz\.photo26-sdk\Qt\Tools\CMake_64\bin\cmake.exe` — `3.30.5` |
| **Ninja (Windows x64)** | No instalado | **Instalado y operativo** | `C:\Users\jblaz\.photo26-sdk\Qt\Tools\Ninja\ninja.exe` — `1.12.1` |
| **Qt 6 para Windows x64** | No instalado | **Instalado y operativo** | `C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64` — **Qt 6.8.3 (`win64_mingw`)** |
| **`windeployqt.exe`** | No instalado | **Instalado y operativo** | `C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64\bin\windeployqt.exe` — `6.8.3` |
| **`qmake.exe` / `qtpaths.exe`** | No instalado | **Instalado y operativo** | `C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64\bin\qmake.exe` / `qtpaths.exe` — `6.8.3` |
| **Git (Windows)** | Instalado | Operativo | `C:\Program Files\Git\cmd\git.exe` — `2.55.0.windows.3` |
| **Python 3 / pip (Windows)** | Instalado | Operativo | `C:\Users\jblaz\AppData\Local\Programs\Python\Python312\python.exe` — `3.12.10` / `pip 26.2.1` |
| **uv (Windows)** | Instalado | Operativo | `uv.exe` — `0.12.9` |
| **WinGet (Windows)** | Instalado | Operativo | `C:\Users\jblaz\AppData\Local\Microsoft\WindowsApps\winget.exe` — `v1.29.380` |

---

## 3. Estado de Qt 6 y Módulos Disponibles (Target Windows x64)

Se ha instalado la distribución oficial de **Qt 6.8.3** para **Windows Desktop x64 (`win64_mingw`)** en:
- Ruta Windows: `C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64`
- Ruta desde WSL2: `/mnt/c/Users/jblaz/.photo26-sdk/Qt/6.8.3/mingw_64`

### Módulos y Plugins Instalados
- **Bibliotecas principales (`bin/` y `lib/cmake/`)**:
  - `Qt6Core.dll` (`Qt6::Core`)
  - `Qt6Gui.dll` (`Qt6::Gui`)
  - `Qt6Widgets.dll` (`Qt6::Widgets`)
  - `Qt6Test.dll` (`Qt6::Test`)
  - `Qt6Concurrent.dll` (`Qt6::Concurrent`)
  - `Qt6Network.dll` (`Qt6::Network`)
  - `Qt6OpenGL.dll` / `Qt6OpenGLWidgets.dll` (`Qt6::OpenGL`, `Qt6::OpenGLWidgets`)
  - `Qt6PrintSupport.dll` (`Qt6::PrintSupport`)
  - `Qt6Sql.dll` (`Qt6::Sql`)
  - `Qt6Svg.dll` / `Qt6SvgWidgets.dll` (`Qt6::Svg`, `Qt6::SvgWidgets`)
  - `Qt6Xml.dll` (`Qt6::Xml`)
- **Plugins verificados (`plugins/`)**:
  - `platforms/qwindows.dll`, `platforms/qminimal.dll`, `platforms/qoffscreen.dll`, `platforms/qdirect2d.dll`
  - `styles/qmodernwindowsstyle.dll`
  - `imageformats/qgif.dll`, `qicns.dll`, `qico.dll`, `qjpeg.dll`, `qsvg.dll`, `qtga.dll`, `qtiff.dll`, `qwbmp.dll`, `qwebp.dll`
  - `iconengines/qsvgicon.dll`

---

## 4. Capacidad de Compilación y Ejecución Windows desde WSL2

1. **Interoperabilidad WSL2 ↔ Windows**:
   - **Verificada y funcional**. Los subagentes sobre el harness `codex-native` (`photo26_implementer`, `photo26_qa`, `photo26_performance`, `photo26_release`) tienen acceso directo a `/mnt/c` y pueden invocar binarios `.exe` de Windows (`cmd.exe`, `powershell.exe`, `cmake.exe`, `ninja.exe`, `g++.exe`, `ctest.exe`, `windeployqt.exe`, `photo26-bootstrap.exe`).
   - *Nota sobre el sandbox de `antigravity-native`*: El harness `antigravity-native` ejecuta su herramienta `run_command` dentro de un contenedor `bwrap` sin `/mnt/c` ni red por defecto. Por diseño de arquitectura multiagente, las tareas de compilación Windows, ejecución de pruebas Windows y empaquetado se delegan a `photo26_release`, `photo26_implementer`, `photo26_qa` y `photo26_performance` (`codex-native`).
2. **Compilación de ejecutables Windows x64 (`.exe`)**:
   - **Verificada**. Se generó `photo26-bootstrap.exe` (`PE32+ executable (GUI) x86-64, for MS Windows`) y `photo26-bootstrap-tests.exe` (`PE32+ executable (console) x86-64, for MS Windows`) en modo `Release`.
3. **Ejecución de aplicaciones gráficas Windows desde WSL2**:
   - **Verificada**. Tanto `photo26-bootstrap-tests.exe` (que instancia `BootstrapWindow`, espera a `QTest::qWaitForWindowExposed(&window)` y simula clic real con `QTest::mouseClick`) como `photo26-bootstrap.exe` (`--self-test` e `--idle-ms 1500`) se ejecutaron sobre el subsistema gráfico de Windows 11 desde WSL2 con código de salida `0`.
   - *Limitación de inspección visual humana*: La verificación funcional de exposición de ventana, título `"Photo26"`, clic de botón y cambio de etiqueta se probó programáticamente con `QtTest` y `--self-test` sobre el plugin nativo `qwindows.dll` de Windows 11, pero un agente autónomo sin captura visual del escritorio anfitrión no realiza inspección ocular de píxeles en pantalla.

---

## 5. Mediciones de Rendimiento del Bootstrap (Fase 6)

Todas las mediciones siguientes son **mediciones reales verificadas** sobre el hardware anfitrión (AMD Ryzen 7 9800X3D, Windows 11 x64 Build 26200):

| Métrica | Valor Real Medido | Condiciones de Medición |
| :--- | :--- | :--- |
| **Tiempo de configuración CMake** | **4,195 ms** (4.20 s) | `cmake.exe -G Ninja -DCMAKE_BUILD_TYPE=Release` (primera configuración limpia) |
| **Tiempo de compilación Release** | **10,498 ms** (10.50 s) | `cmake.exe --build . --config Release` (12 pasos Ninja, incluyendo `AUTOMOC` de 3 targets) |
| **Tiempo de ejecución de CTest** | **280 ms** (test) / **520 ms** (proceso `ctest.exe`) | `photo26-bootstrap-tests.exe` (`1/1 Passed`) |
| **Tamaño del ejecutable (`photo26-bootstrap.exe`)** | **124,801 bytes** (`121.88 KiB`) | Binario `PE32+ GUI x86-64` compilado en modo `Release` |
| **Tamaño del ejecutable de tests (`photo26-bootstrap-tests.exe`)** | **103,822 bytes** (`101.39 KiB`) | Binario `PE32+ console x86-64` compilado en modo `Release` |
| **Tamaño total del paquete portable (`dist/bootstrap/`)** | **35,248,120 bytes** (`33.615 MiB`) | 32 archivos: 1 ejecutable `.exe`, 24 `.dll` (Qt 6.8.3 + MinGW runtime + plugins) y 7 archivos de licencias/SBOM |
| **Tiempo de arranque (Startup Time)** | **502 ms** (`self-test`) / **509 ms** (`idle`) | Desde el inicio de `main()` hasta ventana `BootstrapWindow` mostrada y cola de eventos inicial procesada en Windows 11 con `PATH` limpio |
| **Memoria en reposo — Working Set (`WorkingSetSize`)** | **36,700,160 bytes** (`35,840 KiB` / `35.00 MiB`) | Medido con API Win32 `GetProcessMemoryInfo` tras 1,500 ms en reposo (`--idle-ms 1500`) |
| **Memoria en reposo — Private Commit (`PrivateUsage`)** | **7,057,408 bytes** (`6,892 KiB` / `6.73 MiB`) | Medido con API Win32 `GetProcessMemoryInfo` tras 1,500 ms en reposo (`--idle-ms 1500`) |
| **Memoria tras interacción (`--self-test`)** | Working Set: **35,516 KiB** / Private Usage: **7,176 KiB** | Medido tras `window.show()` + `actionButton()->click()` |

---

## 6. Problemas Encontrados y Soluciones Aplicadas

1. **`sudo` no interactivo en WSL2 requiere contraseña (`sudo: a password is required`)**:
   - *Impacto*: No se pueden instalar paquetes globales con `apt` sin intervención del usuario.
   - *Solución aplicada*: Se instalaron `cmake 4.4.4`, `ninja 1.13.2`, `conan 2.34.0` y `aqtinstall 3.3.0` en espacio de usuario (`~/.local/bin`) mediante `uv tool install`, y se instaló el SDK completo de **Qt 6.8.3 (`win64_mingw`)**, **MinGW-w64 13.1.0**, **CMake 3.30.5** y **Ninja 1.12.1** en `C:\Users\jblaz\.photo26-sdk\Qt` sin requerir `sudo` ni elevación UAC.
   - *Comando opcional para el usuario* (si desea instalar paquetes de sistema en Ubuntu 24.04):
     ```bash
     sudo apt update && sudo apt install -y cmake ninja-build pkg-config mingw-w64 clang qt6-base-dev \
       libvips-dev libjpeg-turbo8-dev libpng-dev libwebp-dev libtiff-dev libheif-dev libexiv2-dev liblcms2-dev
     ```
2. **Rendimiento de extracción y compilación sobre DrvFS (`/mnt/c` y rutas UNC `\\wsl.localhost\...`)**:
   - *Impacto*: La extracción de miles de archivos pequeños a través del puente 9p/DrvFS entre WSL2 y NTFS es notablemente más lenta que en el sistema de archivos nativo, y `cmd.exe` no admite rutas UNC como directorio de trabajo actual.
   - *Solución aplicada*: Para invocar herramientas Windows desde WSL2, se recomienda usar PowerShell o fijar el directorio de trabajo en `C:\...` (por ejemplo, construir en `C:\Users\jblaz\AppData\Local\Temp\photo26-bootstrap-build` y sincronizar artefactos a `/home/jblaz/dev/photo26/bootstrap/build` y `/home/jblaz/dev/photo26/dist/bootstrap`).
3. **Interrupción de PowerShell ante advertencias en `stderr` de `windeployqt.exe`**:
   - *Impacto*: `windeployqt.exe` emite en `stderr` el aviso no fatal `Warning: Cannot find any version of the dxcompiler.dll and dxil.dll`. Si PowerShell se ejecuta con `$ErrorActionPreference = 'Stop'`, aborta prematuramente la copia de DLLs.
   - *Solución aplicada*: Ejecutar `windeployqt.exe` desde `cmd.exe /c` o con `$ErrorActionPreference = 'Continue'` y verificar siempre la integridad de las DLLs desplegadas.
