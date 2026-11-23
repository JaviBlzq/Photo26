# Photo26 — Instrucciones de Compilación, Pruebas y Empaquetado (`build-instructions.md`)

**Fecha:** 2026-11-04  
**Target principal:** Windows 10/11 x64 (`x86_64-w64-mingw32`, PE32+ GUI)  
**Estándar de lenguaje:** C++20 (`-std=c++20`)

---

## 1. Rutas del Toolchain Instalado

El toolchain de compilación para Windows x64 está instalado en espacio de usuario (sin requerir permisos de administrador) bajo `C:\Users\jblaz\.photo26-sdk\Qt` (accesible desde WSL2 en `/mnt/c/Users/jblaz/.photo26-sdk/Qt`):

| Herramienta | Ruta Windows | Ruta WSL2 | Versión |
| :--- | :--- | :--- | :--- |
| **Qt 6 SDK (`win64_mingw`)** | `C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64` | `/mnt/c/Users/jblaz/.photo26-sdk/Qt/6.8.3/mingw_64` | `6.8.3` |
| **Compilador C++ (`g++.exe`)** | `C:\Users\jblaz\.photo26-sdk\Qt\Tools\mingw1310_64\bin\g++.exe` | `/mnt/c/Users/jblaz/.photo26-sdk/Qt/Tools/mingw1310_64/bin/g++.exe` | MinGW-w64 GCC `13.1.0` |
| **CMake (Windows)** | `C:\Users\jblaz\.photo26-sdk\Qt\Tools\CMake_64\bin\cmake.exe` | `/mnt/c/Users/jblaz/.photo26-sdk/Qt/Tools/CMake_64/bin/cmake.exe` | `3.30.5` |
| **CTest (Windows)** | `C:\Users\jblaz\.photo26-sdk\Qt\Tools\CMake_64\bin\ctest.exe` | `/mnt/c/Users/jblaz/.photo26-sdk/Qt/Tools/CMake_64/bin/ctest.exe` | `3.30.5` |
| **Ninja (Windows)** | `C:\Users\jblaz\.photo26-sdk\Qt\Tools\Ninja\ninja.exe` | `/mnt/c/Users/jblaz/.photo26-sdk/Qt/Tools/Ninja/ninja.exe` | `1.12.1` |
| **`windeployqt.exe`** | `C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64\bin\windeployqt.exe` | `/mnt/c/Users/jblaz/.photo26-sdk/Qt/6.8.3/mingw_64/bin/windeployqt.exe` | `6.8.3` |
| **CMake / Ninja (Linux)** | — | `/home/jblaz/.local/bin/cmake` / `/home/jblaz/.local/bin/ninja` | `4.4.4` / `1.13.2` |

---

## 2. Estructura del Proyecto Bootstrap

```text
/home/jblaz/dev/photo26/
├── bootstrap/
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── bootstrap_window.hpp
│   │   ├── bootstrap_window.cpp
│   │   └── main.cpp
│   ├── tests/
│   │   └── test_bootstrap_window.cpp
│   └── build/
│       ├── photo26-bootstrap.exe
│       ├── photo26-bootstrap-tests.exe
│       ├── configure.log
│       ├── build.log
│       ├── test.log
│       ├── windeployqt.log
│       ├── self-test-report.json
│       └── idle-report.json
└── dist/
    └── bootstrap/
        ├── photo26-bootstrap.exe
        ├── Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Svg.dll, Qt6Network.dll
        ├── libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll
        ├── platforms/qwindows.dll
        ├── styles/qmodernwindowsstyle.dll
        ├── imageformats/{qgif,qicns,qico,qjpeg,qsvg,qtga,qtiff,qwbmp,qwebp}.dll
        ├── iconengines/qsvgicon.dll
        ├── generic/qtuiotouchplugin.dll
        ├── networkinformation/qnetworklistmanager.dll
        ├── tls/{qcertonlybackend,qschannelbackend}.dll
        └── licenses/
            ├── THIRD-PARTY-NOTICES.txt
            ├── LGPL-3.0.txt
            ├── GCC-RUNTIME-LIBRARY-EXCEPTION.txt
            ├── MINGW-W64-RUNTIME.txt
            ├── WINPTHREADS-COPYING.txt
            └── qt-sbom/{qtbase-6.8.3.spdx,qtsvg-6.8.3.spdx}
```

---

## 3. Compilación paso a paso (Windows x64 Release)

Desde WSL2 (o directamente en PowerShell en Windows), puedes compilar el proyecto ejecutando:

```powershell
powershell.exe -NoProfile -Command '
  $sdk = "C:\Users\jblaz\.photo26-sdk\Qt"
  $env:PATH = "$sdk\Tools\mingw1310_64\bin;$sdk\6.8.3\mingw_64\bin;$sdk\Tools\CMake_64\bin;$sdk\Tools\Ninja;C:\Windows\System32;C:\Windows"
  $src = "\\wsl.localhost\Ubuntu-24.04\home\jblaz\dev\photo26\bootstrap"
  $bld = "C:\Users\jblaz\AppData\Local\Temp\photo26-bootstrap-build"

  Remove-Item -Recurse -Force $bld -ErrorAction SilentlyContinue
  cmake.exe -S $src -B $bld -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$sdk\6.8.3\mingw_64" `
    -DCMAKE_CXX_COMPILER="$sdk\Tools\mingw1310_64\bin\g++.exe" `
    -DCMAKE_MAKE_PROGRAM="$sdk\Tools\Ninja\ninja.exe"

  cmake.exe --build $bld --config Release
'
```

> **Consejo de rendimiento en WSL2:** Configurar el árbol de compilación intermedio (`-B`) en una ruta NTFS local (`C:\Users\jblaz\AppData\Local\Temp\photo26-bootstrap-build`) evita la latencia de metadatos de archivos sobre el protocolo 9p (`\\wsl.localhost\...`), logrando tiempos de configuración de **4.20 s** y de compilación limpia de **10.50 s**. Posteriormente, el árbol compilado se sincroniza a `/home/jblaz/dev/photo26/bootstrap/build/`.

---

## 4. Ejecución de Pruebas Automatizadas (CTest / QtTest)

### 4.1. Suite unitaria y de interfaz con `Qt6::Test`
```powershell
powershell.exe -NoProfile -Command '
  $sdk = "C:\Users\jblaz\.photo26-sdk\Qt"
  $env:PATH = "$sdk\Tools\mingw1310_64\bin;$sdk\6.8.3\mingw_64\bin;$sdk\Tools\CMake_64\bin;C:\Windows\System32;C:\Windows"
  ctest.exe --test-dir "\\wsl.localhost\Ubuntu-24.04\home\jblaz\dev\photo26\bootstrap\build" --output-on-failure
'
```

Salida verificada:
```text
Test project C:/Users/jblaz/AppData/Local/Temp/photo26-bootstrap-build
    Start 1: photo26_bootstrap_ui_test
1/1 Test #1: photo26_bootstrap_ui_test ........   Passed    0.28 sec

100% tests passed, 0 tests failed out of 1
```

### 4.2. Autoverificación y medición de rendimiento del ejecutable portable (con `PATH` limpio)
El ejecutable `photo26-bootstrap.exe` incorpora dos banderas opcionales para validación automatizada sin alterar su comportamiento normal cuando se inicia sin argumentos:

- **Autoverificación funcional (`--self-test`)**:
  Muestra la ventana principal, procesa la cola de eventos de Windows, pulsa programáticamente el botón `"Verificar entorno"`, comprueba que el título sea `"Photo26"` y que el contador de clics sea `1`, y exporta las métricas reales de arranque y memoria:
  ```powershell
  powershell.exe -NoProfile -Command '
    $env:PATH = "C:\Windows\System32;C:\Windows"
    & "\\wsl.localhost\Ubuntu-24.04\home\jblaz\dev\photo26\dist\bootstrap\photo26-bootstrap.exe" `
      --self-test --report-file "\\wsl.localhost\Ubuntu-24.04\home\jblaz\dev\photo26\bootstrap\build\self-test-report.json"
  '
  ```
- **Medición en reposo (`--idle-ms <N>`)**:
  Muestra la ventana, procesa eventos y permanece en reposo `N` milisegundos antes de muestrear `WorkingSetSize` y `PrivateUsage` mediante `GetProcessMemoryInfo`:
  ```powershell
  powershell.exe -NoProfile -Command '
    $env:PATH = "C:\Windows\System32;C:\Windows"
    & "\\wsl.localhost\Ubuntu-24.04\home\jblaz\dev\photo26\dist\bootstrap\photo26-bootstrap.exe" `
      --idle-ms 1500 --report-file "\\wsl.localhost\Ubuntu-24.04\home\jblaz\dev\photo26\bootstrap\build\idle-report.json"
  '
  ```

---

## 5. Generación del Paquete Portable (`/home/jblaz/dev/photo26/dist/bootstrap/`)

Para regenerar la distribución portable desde cero:

```bash
rm -rf /home/jblaz/dev/photo26/dist/bootstrap
mkdir -p /home/jblaz/dev/photo26/dist/bootstrap/licenses
cp /home/jblaz/dev/photo26/bootstrap/build/photo26-bootstrap.exe /home/jblaz/dev/photo26/dist/bootstrap/
```

Y desplegar las dependencias con `windeployqt.exe`:

```powershell
cmd.exe /d /c "set PATH=C:\Users\jblaz\.photo26-sdk\Qt\Tools\mingw1310_64\bin;C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64\bin;%PATH% && C:\Users\jblaz\.photo26-sdk\Qt\6.8.3\mingw_64\bin\windeployqt.exe --force --release --compiler-runtime --no-translations --no-system-d3d-compiler --no-opengl-sw \\wsl.localhost\Ubuntu-24.04\home\jblaz\dev\photo26\dist\bootstrap\photo26-bootstrap.exe"
```

> **Importante:** Al invocar `windeployqt.exe` desde PowerShell, no uses `$ErrorActionPreference = 'Stop'`, ya que `windeployqt.exe` emite por `stderr` un aviso informativo sobre `dxcompiler.dll` y `dxil.dll` (no requeridos por Qt Widgets) que PowerShell interpretaría como excepción si se configura en modo estricto.
