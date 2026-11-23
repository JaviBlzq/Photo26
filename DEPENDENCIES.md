# Dependencias reproducibles

`vcpkg.json` fija el registro mediante `builtin-baseline`. El triplet no forma
parte del formato del manifiesto: se selecciona al instalar o configurar.

```powershell
vcpkg install --triplet x64-windows
vcpkg install --triplet x64-mingw-dynamic
```

El manifiesto incluye Qt 6 (`qtbase`, `qtsvg`, `qtimageformats`) y
ONNX Runtime (`onnxruntime`) para dejar documentado el conjunto previsto aunque
el bootstrap solo enlaza Core, Gui, Widgets y Test. `pdfium` es el backend PDF
principal en vcpkg.

`conanfile.txt` es la alternativa con versiones exactas. Usa `poppler` como
backend PDF equivalente porque su receta es la opción portable habitual en
Conan. Los perfiles de host deben fijar arquitectura, compilador, runtime y
tipo de build; deben guardarse junto al proyecto cuando se decida la matriz de
compiladores soportada.

No se deben mezclar paquetes de ambos gestores en una misma configuración de
CMake.

