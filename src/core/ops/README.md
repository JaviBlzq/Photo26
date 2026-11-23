# Operaciones de photo26-core

Implementación C++20 con Qt 6.8.3 Core, Gui y Concurrent. No depende de Widgets,
`io/`, red, modelos descargados ni servicios externos. Los resultados son imágenes
nuevas; el original no se modifica. Los métodos de guardado usan SafeFileWriter.

## Contratos y decisiones

- `rotateQuadrants90` recibe **grados múltiplos de 90** (incluido -90), conforme
  a los ejemplos del contrato. Las rotaciones positivas siguen la convención
  de QTransform. Los recortes se intersecan con la imagen y el ajuste de relación
  de aspecto reduce el rectángulo alrededor de su centro.
- Resize: cuando se indican ancho y alto con conservación de aspecto, se encaja
  dentro de ambos límites. Las dimensiones finales se redondean al entero más
  próximo y nunca son menores que uno. FitMaxDimensions siempre conserva aspecto.
  Bilineal, Catmull-Rom (Keys a=-0.5) y Lanczos-3 usan pasadas separables, soporte
  ensanchado al reducir y extensión del píxel de borde. El cálculo usa RGBA
  premultiplicado de 32 bits flotantes por componente. El orden de las pasadas
  minimiza el tamaño del búfer intermedio; se conserva la profundidad de 16 bits
  y el formato premultiplicado/no premultiplicado del origen. Los formatos
  indexados se convierten a ARGB32. Nearest usa el remuestreador rápido de Qt.
- Combinación: horizontal/vertical/grid usan celdas comunes; Mosaic usa filas de
  altura común y anchuras proporcionales. KeepOriginalSize no remuestrea; una
  celda forzada menor recorta por su límite. FillCellCrop escala y recorta;
  FitProportionally encaja. Ninguno de estos modos estira la imagen.
- Color: si existe un perfil diferente de sRGB, se convierte primero a sRGB.
  Para imágenes sin perfil se asume sRGB. Orden: exposición, brillo/contraste,
  gamma, saturación, gris, sepia, inversión, blanco/negro.
  EV usa la función de transferencia sRGB para multiplicar en luz lineal por
  `2^EV`. Contraste usa `((100+contraste)/100)^2` alrededor de 0.5; brillo añade
  `brillo/100`. Gamma usa `v^(1/gamma)`. Se usan los coeficientes Rec.709 exactos
  indicados en el contrato. Sepia usa la matriz 0.393/0.769/0.189,
  0.349/0.686/0.168 y 0.272/0.534/0.131. El resultado es RGBA64 y conserva alfa.
  Floyd-Steinberg recorre filas de izquierda a derecha y pondera el error por alfa.
- Compresión: requiere un plugin Qt de escritura para el formato. JPEG lossless
  se rechaza. WebP lossless usa calidad 100, según el contrato del plugin Qt.
  El objetivo de bytes es aproximado: busca calidad 5..100 y reduce dimensiones
  si hace falta; un objetivo inferior a la cabecera mínima del formato no puede
  garantizarse. `originalSizeBytes=0` produce ahorro 0 porque no se conoce la
  codificación original. JPEG/BMP se componen sobre el fondo especificado.
- Variantes web: el ancho solicitado aparece en el nombre; actualSize indica
  el tamaño real cuando se impide ampliar. Al existir solo anchos en esta API,
  las variantes siempre mantienen aspecto, también con keepAspectRatio=false.
  Eliminar metadatos elimina textos y resolución; conserva el perfil de color.
  Se rechazan nombres duplicados y rutas que escapen del directorio de salida.
- Texto: requiere QGuiApplication (también funciona con plataforma offscreen),
  pero nunca QApplication ni Widgets. Las marcas de imagen no necesitan fuentes.
  Los bordes y el padding amplían el lienzo. El radio recorta las esquinas externas.
- Sprites: celdas forzadas menores reducen proporcionalmente el sprite. Las
  coordenadas JSON describen el rectángulo dibujado, sin spacing ni padding.
- Paletas: k-means++ con semilla 26, hasta 65.536 muestras, RGB y pesos alfa;
  excluye píxeles totalmente transparentes. Puede devolver menos colores si no
  existen suficientes colores distintos. Dominancias normalizadas sobre las
  muestras visibles. La exportación PNG dibuja bandas de anchura uniforme.
- Comparación: imágenes de distinto tamaño se alinean arriba a la izquierda en
  un lienzo transparente común, sin escalarlas. MAE y RMSE se expresan en 0..255
  sobre RGBA premultiplicado (cuatro componentes), incluyendo diferencias de
  alfa. PSNR usa pico 255 y vale infinito si son idénticas. El slider muestra A
  a la izquierda y B a la derecha; overlay dibuja B sobre A con la opacidad dada.
- Fondo: el método con nombre heredado AutoBorderFloodAndGrabCut implementa
  **segmentación por distribución Lab de los cuatro bordes y flood-fill exterior**,
  no GrabCut. Estima hasta 12 modos de color de borde y elimina los poco frecuentes.
  Feathering usa un filtro gaussiano separable sobre alfa. No es segmentación
  semántica ni resuelve de forma general pelo, transparencias o fondos complejos.
  ChromaKey actúa globalmente por distancia Lab. LuminanceThreshold elimina
  luminancia cercana a keyColor con coeficientes Rec.709. Se conserva el alfa
  original multiplicándolo por la máscara. ONNX devuelve false; proporcionar
  un modelo produce UnsupportedFormat en esta compilación. El motor predeterminado
  funciona íntegramente sin modelos ni descargas.
- Renombrado: `{date}` usa la fecha local de modificación del archivo en formato
  yyyy-MM-dd. Se rechazan nombres Windows inválidos, duplicados y destinos que
  sean otros orígenes del mismo lote. No se implementan ciclos de renombrado.
  Cada copia se confirma atómicamente antes de eliminar el origen si se solicita
  mover. Los conjuntos de varios archivos **no son transacciones**: un error de
  disco posterior puede dejar salidas anteriores confirmadas.
- Lotes: cola de índices, número acotado de trabajadores (por defecto hasta cuatro),
  lectura individual y liberación de imagen al terminar cada archivo. No precarga
  el lote. La cancelación se comprueba entre archivos, antes del guardado y entre
  codificación y escritura; no interrumpe una llamada activa a un codec. El saver
  personalizado es responsable de sus propias garantías de escritura y puede
  invocarse concurrentemente. Los callbacks de progreso se serializan y se ejecutan
  en hilos trabajadores; la UI debe reenviarlos a su hilo. Los errores por archivo
  y excepciones del saver se registran sin detener el resto del lote.
- Presets: `BatchProcessor::presetToJson` / `presetFromJson` y funciones libres
  equivalentes usan QJsonObject, schemaVersion=1, todos los campos de opciones
  y marca de imagen PNG en base64. Se validan tipos, enumeraciones y rangos.
- Para cumplir el aislamiento de inclusiones, batch_rename_op.hpp contiene los
  mismos bloques protegidos de declaraciones resize/color/watermark. Al cambiar
  esas interfaces hay que mantener ambos bloques sincronizados. No incorpora
  cabeceras de otras operaciones ni de `io/`.

## Verificación reproducible

Desde WSL, ejecutar `python3 src/core/ops/tests/run_validation.py`. El script usa
CMake y Ninja del SDK aislado, Qt 6.8.3 exacto, y configura PATH de MinGW/Qt **antes**
de lanzar el compilador. Genera todo dentro de `ops/tests/`. Compila conjuntamente
los nueve módulos y la implementación existente de SafeFileWriter, con
`-Wall -Wextra -Werror`, y ejecuta CTest offscreen. No modifica common/ ni la
configuración CMake del proyecto principal.

`tests/ops_tests.cpp` cubre geometría, remuestreo y alfa, precisión de 16 bits,
color y difusión, composiciones, codecs/objetivo de bytes, variantes, texto,
atlas/JSON, paletas/exportaciones, comparación, conectividad de fondo, nombres,
escritura, cancelación, aislamiento de errores y serialización de presets.
La integración con el resto de photo26-core y la validación independiente de QA
corresponden a la compilación/revisión general del proyecto.
