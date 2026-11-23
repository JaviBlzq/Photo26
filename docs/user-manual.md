# Manual de Usuario de Photo26 v1.0 (`user-manual.md`)

**Versión del producto:** Photo26 v1.0 (`1.0.0`)  
**Plataforma:** Windows 10 y Windows 11 x64 (Edición Portable de Escritorio)  
**Motor y tecnología:** C++20 nativo + Qt 6.8.3 Widgets (`win64_mingw`)  

---

## 1. Introducción a Photo26 v1.0

**Photo26 v1.0** es una caja de herramientas fotográfica y de imagen digital de escritorio **100% local, gratuita y sin conexión (offline)** diseñada para **Windows 10 y Windows 11 (x64)**. Inspirada funcionalmente en la filosofía modular y directa de suites como **PDF24**, Photo26 reúne en una única estación de trabajo ligera las **20 herramientas esenciales (`F01` a `F20`)** para convertir, recortar, redimensionar, combinar, ajustar el color, comprimir, optimizar para web, generar favicons, auditar o limpiar metadatos, procesar lotes, estampar marcas de agua, rotular, animar GIFs, exportar a PDF, empaquetar hojas de sprites, extraer paletas, comparar imágenes, eliminar fondos y renombrar colecciones masivamente.

### Principios Fundamentales para el Usuario
- **Privacidad absoluta (100% local y sin telemetría):** Ninguna fotografía sale jamás de tu equipo. No existen cuentas de usuario, inicios de sesión, marcas de agua impuestas, límites diarios artificiales ni telemetría analítica o llamadas a servidores externos.
- **Edición no destructiva por defecto:** Photo26 trabaja sobre copias en memoria. Tus archivos originales en disco nunca se modifican ni se sobrescriben de forma silenciosa.
- **Escritura atómica segura:** Todas las operaciones de guardado utilizan volcado transaccional (`SafeFileWriter` sobre `QSaveFile`), garantizando que un corte de energía o una cancelación nunca deje archivos corruptos o truncados en tu disco duro.
- **Portabilidad real:** El paquete de distribución de Photo26 v1.0 se ejecuta directamente desde cualquier carpeta o memoria USB en Windows 10/11 x64 sin requerir instaladores ni permisos de administrador.

---

## 2. Interfaz Principal, Navegación y Experiencia de Uso

Al iniciar `photo26.exe`, la aplicación se organiza en dos vistas principales gestionadas por [`MainWindow`](file:///home/jblaz/dev/photo26/src/desktop/main_window.hpp): el **Lanzador Principal de Tarjetas por Categorías** y el **Espacio de Trabajo Interactivo (Workspace)**.

### 2.1. Pantalla Principal de Tarjetas por Categorías
Implementada mediante [`ToolCardGridWidget`](file:///home/jblaz/dev/photo26/src/desktop/widgets/tool_card_grid.hpp), la pantalla de inicio presenta las herramientas organizadas visualmente en **4 grandes bloques funcionales**:
1. **Conversión y optimización:** Convertir formato (`F01`), Comprimir imagen (`F06`), Optimizar para Web (`F07`), Generador de Favicon e ICO (`F08`) y Procesamiento por lotes (`F10`).
2. **Edición y geometría:** Recortar y enderezar (`F02`), Redimensionar y rotar (`F03`), Ajustes de color y filtros (`F05`), Eliminar fondo / Transparencia (`F18`) y Marca de agua (`F11`).
3. **Composición y documentos:** Combinar y collage (`F04`), Texto, marcos y bordes (`F12`), Imágenes a PDF (`F14`), Animación GIF / Secuencia (`F13`) y Hoja de Sprites / Spritesheet (`F15`).
4. **Metadatos y utilidades:** Inspector y limpieza de metadatos (`F09`), Extractor de paleta de colores (`F16`), Comparador visual de imágenes (`F17`) y Renombrado masivo inteligente (`F19`).

La rejilla es **auto-adaptativa**: ajusta automáticamente el número de columnas (`1`, `2`, `3` o `4` columnas) según la anchura de la ventana (`580 px`, `920 px`, `1340 px`).

### 2.2. Búsqueda Rápida Instantánea
En la esquina superior derecha de la pantalla de inicio se ubica el **buscador en tiempo real**:
- Al escribir cualquier término (por ejemplo, `webp`, `recortar`, `exif`, `fondo`, `pdf`, `gif` o `lote`), la cuadrícula filtra al instante las tarjetas por título, subtítulo, categoría o identificador interno.
- Las categorías sin coincidencias se ocultan dinámicamente para ofrecer una vista limpia.

### 2.3. Arrastrar y Soltar (Drag & Drop) Directo
Photo26 soporta **Drag & Drop nativo desde el Explorador de Windows** en tres niveles:
1. **Sobre una tarjeta específica de la pantalla de inicio:** Carga inmediatamente los archivos arrastrados y abre directamente esa herramienta con los archivos ya listos en el espacio de trabajo.
2. **Sobre el banner superior de la pantalla de inicio:** Carga los archivos en el espacio de trabajo y abre la herramienta de conversión de formatos (`F01`).
3. **Sobre el lienzo central del espacio de trabajo ([`InteractiveCanvasWidget`](file:///home/jblaz/dev/photo26/src/desktop/widgets/interactive_canvas.hpp)) o la ventana principal:** Añade o actualiza la lista de imágenes cargadas en la sesión activa.

### 2.4. Temas Claro y Oscuro y Escalado High-DPI
- **Temas visuales ([`ThemeManager`](file:///home/jblaz/dev/photo26/src/desktop/widgets/theme_manager.hpp)):** Puedes alternar en cualquier momento desde la barra superior entre el tema **Oscuro Moderno (`DarkModern`)**, calibrado para reducir la fatiga visual durante el retoque fotográfico, y el tema **Claro Limpio (`LightClean`)**. La preferencia seleccionada se guarda automáticamente entre sesiones.
- **Escalado DPI nítido:** Todos los iconos de herramientas y controles se renderizan vectorialmente en tiempo de ejecución con soporte completo para pantallas HiDPI (`devicePixelRatio` 100%, 125%, 150%, 200%+).
- **Fondo damero de transparencia:** El lienzo central muestra un patrón damero (*checkerboard*) para distinguir con claridad las zonas transparentes (canal alfa) del fondo del lienzo.

### 2.5. Atajos de Teclado Globales
Photo26 incorpora atajos estándar de productividad en toda la ventana de trabajo:

| Atajo de Teclado | Acción en Photo26 v1.0 |
| :--- | :--- |
| **`Ctrl+O`** | Abre el diálogo nativo de Windows para cargar una o varias imágenes en el espacio de trabajo. |
| **`Ctrl+S`** | Ejecuta **«Guardar como…»** para exportar la imagen procesada actual mediante diálogo nativo de Windows. |
| **`Ctrl+Shift+S`** | Ejecuta **«Guardar archivos…»** para exportar múltiples resultados hacia una carpeta de destino. |
| **`Esc`** | Regresa a la **Pantalla Principal de Tarjetas** desde cualquier herramienta activa (o cancela el modo modal activo). |
| **`Ctrl++`** | Acerca el zoom (`Zoom In`) sobre la vista previa central. |
| **`Ctrl+-`** | Aleja el zoom (`Zoom Out`) sobre la vista previa central. |
| **`Ctrl+0`** | Restablece el zoom al **100%** o ajusta la imagen a la ventana (`Fit to Window`). |
| **`Espacio + Arrastrar`** (o botón central del ratón) | Desplaza el lienzo (*Pan*) libremente cuando la imagen está ampliada. |

---

## 3. Guía Paso a Paso de las 20 Funcionalidades (`F01` a `F20`)

A continuación se detalla el funcionamiento verificable y paso a paso de cada una de las 20 herramientas integradas en **Photo26 v1.0**.

---

### `F01` — Conversión de Formatos (`convert`)
Permite transformar imágenes individuales o colecciones entre los formatos soportados por el registro [`FormatRegistry`](file:///home/jblaz/dev/photo26/src/core/io/format_registry.hpp) y el motor [`ImageIo`](file:///home/jblaz/dev/photo26/src/core/io/image_io.hpp).

- **Capacidades reales de lectura y escritura:**
  - **Lectura y escritura completas:** `JPG/JPEG`, `PNG`, `WebP`, `BMP`, `TIFF`, `GIF` (con codificador `GIF89a` propio), `ICO` (multi-resolución), `PPM`, `PGM`, `PBM`, `XBM`, `XPM` y formatos adicionales provistos por plugins Qt (`TGA`, `WBMP`, `ICNS`).
  - **Solo lectura (rasterización vectorial):** `SVG` y `SVGZ`. Photo26 rasteriza gráficos vectoriales mediante `QSvgRenderer` a las dimensiones en píxeles que indiques; no vectoriza mapas de bits ni incrusta bitmaps falsos dentro de un archivo `.svg`.
  - **Solo escritura (exportación documental):** `PDF` (mediante `F14` / `PdfExporter`).
  - **Detección honesta de `HEIC`, `HEIF` y `AVIF`:** El motor detecta sus cabeceras `ISOBMFF` (`ftyp`) e informa con transparencia de si el entorno de ejecución dispone del plugin/códec del sistema, además de mostrar en pantalla la tabla completa de capacidades reales de cada formato.
- **Pasos de uso:**
  1. Abre **Convertir formato** y carga una o más imágenes.
  2. Selecciona el **Formato de destino** y el **Espacio de color** (`Conservar original`, `sRGB`, `Adobe RGB`, `Display P3` o `ProPhoto RGB`, gestionado por [`IccManager`](file:///home/jblaz/dev/photo26/src/core/color/icc_manager.hpp)).
  3. Si el archivo de entrada es un vector **SVG/SVGZ**, define el **Ancho y Alto de rasterización** deseados en píxeles (o deja `0` para usar su tamaño intrínseco).
  4. Si conviertes una imagen con transparencia (alfa) hacia un formato opaco como `JPEG` o `BMP`, elige el **Color de fondo de aplanado** (blanco por defecto).
  5. Pulsa **«Guardar como…»** (`Ctrl+S`) para un archivo o **«Guardar archivos…»** (`Ctrl+Shift+S`) para todo el conjunto.

---

### `F02` — Recorte y Transformación Geométrica (`crop`)
Permite encuadrar fotografías, aplicar proporciones estándar o personalizadas, rotar en cuadrantes o ángulos libres y voltear mediante [`CropTransformOp`](file:///home/jblaz/dev/photo26/src/core/ops/crop_transform.hpp).

- **Capacidades principales:**
  - **Relaciones de aspecto:** Libre (`Free`), `1:1` (Cuadrado), `3:4`, `4:3`, `16:9`, `9:16`, `2:3`, `3:2`, `21:9` y **Personalizada (`Custom`)** definiendo ancho y alto relativos.
  - **Regla de los tercios:** Superposición visual interactiva de las 4 líneas guía (`RuleOfThirdsLines`) sobre el lienzo central con tiradores en esquinas y bordes.
  - **Rotación y volteo:** Rotación exacta por múltiplos de `90°` (`-90°`, `90°`, `180°`, `270°`), rotación en **ángulo libre** (`-180°` a `+180°`) con color de relleno configurable y **volteo horizontal / vertical** (`Flip H` / `Flip V`).
- **Pasos de uso:**
  1. Selecciona una proporción en el desplegable o ajusta directamente el rectángulo sobre el lienzo interactivo (también puedes introducir `X`, `Y`, `Ancho` y `Alto` exactos en píxeles).
  2. Activa o desactiva la casilla **Mostrar regla de los tercios** para ayudarte en la composición.
  3. Aplica giros de `90°`, ángulo libre o volteos según necesites y exporta el resultado con **«Guardar como…»**.

---

### `F03` — Redimensionado de Alta Precisión (`resize`)
Escala imágenes con precisión matemática en coma flotante de 32 bits por canal (con alfa premultiplicado para evitar halos oscuros en bordes transparentes) y preserva imágenes de 16 bits por canal (`RGBA64`), respaldado por [`ResizeOp`](file:///home/jblaz/dev/photo26/src/core/ops/resize_op.hpp).

- **Modos de escalado:**
  - **Por píxeles (`ByPixels`):** Define ancho y/o alto exactos. Si **Mantener relación de aspecto** está activo, la imagen se encaja proporcionalmente dentro de ambos límites.
  - **Por porcentaje (`ByPercentage`):** Escala uniforme desde el `1%` en adelante.
  - **Encajar en dimensiones máximas (`FitMaxDimensions`):** Garantiza que la imagen resultante no supere una caja máxima (por ejemplo, `1920 × 1080` px) manteniendo siempre su proporción original.
- **Protección «Evitar ampliaciones» (`preventUpscaling`):** Si está activada y la imagen original es más pequeña que las dimensiones solicitadas, Photo26 conserva su tamaño original para evitar pixelado artificial.
- **Algoritmos de interpolación:**
  - **Lanczos-3** (predeterminado, máxima nitidez fotográfica con filtro separable de soporte ensanchado en reducciones).
  - **Bicúbico** (Catmull-Rom con $a = -0.5$).
  - **Bilineal** (suavizado rápido).
  - **Vecino más próximo (`Nearest`)** (ideal para *pixel art* y gráficos retro sin difuminar bordes).

---

### `F04` — Combinación de Imágenes y Collage (`combine`)
Une múltiples imágenes en un único lienzo mediante [`CombineOp`](file:///home/jblaz/dev/photo26/src/core/ops/combine_op.hpp) sin deformar ni estirar nunca el aspecto original.

- **Disposiciones (`CombineLayout`):**
  - **Horizontal (`Horizontal`):** Tira continua de izquierda a derecha.
  - **Vertical (`Vertical`):** Columna continua de arriba abajo.
  - **Cuadrícula (`Grid`):** Rejilla ordenada configurando el número de columnas (`gridColumns`).
  - **Mosaico (`Mosaic`):** Filas equilibradas de altura común donde cada imagen conserva su anchura proporcional exacta sin recortes ni deformaciones.
- **Políticas de ajuste sin deformación (`CombineScalePolicy`):**
  - **Ajustar proporcionalmente (`FitProportionally`):** Escala cada imagen dentro de su celda manteniendo intacta su relación de aspecto.
  - **Rellenar celda recortando (`FillCellCrop`):** Escala proporcionalmente hasta cubrir toda la celda y recorta el excedente centrado.
  - **Conservar tamaño original (`KeepOriginalSize`):** No remuestrea la imagen.
- **Personalización:** Control de espaciado entre celdas (`spacingPx`), margen exterior (`marginPx`), alineación (`Inicio`, `Centro`, `Fin`) y fondo sólido o transparente.

---

### `F05` — Ajustes de Color, Tonos y Filtros (`color`)
Tubería cromática de alta precisión en [`ColorAdjustOp`](file:///home/jblaz/dev/photo26/src/core/ops/color_adjust_op.hpp) que convierte automáticamente perfiles distintos de sRGB a espacio estándar sRGB antes de operar y devuelve salida en alta profundidad conservando el canal alfa intacto.

- **Orden determinista del pipeline y controles disponibles:**
  1. **Exposición (`exposureEv`):** Ajuste fotográfico en pasos de luz lineal ($\text{factor} = 2^{\text{EV}}$ sobre la curva de transferencia sRGB).
  2. **Brillo (`brightness`) y Contraste (`contrast`):** Deslizadores de `-100` a `+100`.
  3. **Corrección Gamma (`gamma`):** Función potencial $v^{1/\gamma}$ para levantar o comprimir tonos medios.
  4. **Saturación (`saturation`):** Desde desaturación hasta realce cromático intenso.
  5. **Escala de grises (`grayscale`):** Conversión luma de alta fidelidad según el estándar **ITU-R BT.709 (Rec.709)** ($Y = 0.2126 R + 0.7152 G + 0.0722 B$).
  6. **Efecto Sepia (`sepia`):** Matriz clásica de virado cálido con control de **intensidad** (`0.0` a `1.0`).
  7. **Invertir colores (`invert`):** Negativo fotográfico de los canales RGB preservando la opacidad alfa original.
  8. **Blanco y Negro binario (`blackAndWhite`):** Umbral configurable (`0..255`) con opción de **tramado por difusión de error Floyd-Steinberg** (`bwDither`) ponderado por alfa.

---

### `F06` — Compresión Inteligente de Imágenes (`compress`)
Reduce el peso en disco de tus imágenes con métricas reales en vivo mediante [`CompressWebOp::compressInMemory`](file:///home/jblaz/dev/photo26/src/core/ops/compress_web_op.hpp).

- **Opciones de compresión:**
  - Selección de formato de salida (`JPEG`, `WebP`, `PNG`).
  - **Calidad manual (`1` a `100`)** o modo **Sin pérdida (`Lossless`)** (soportado en `PNG` y `WebP`; si se solicita *lossless* sobre `JPEG`, Photo26 lo rechaza honestamente informando de que el estándar JPEG base es con pérdida).
  - **Tamaño máximo objetivo aproximado en KiB (`targetMaxBytes`):** Si defines un techo en KiB, el motor busca automáticamente el nivel óptimo de calidad (`5..100`) y, si el archivo aún supera el objetivo, reduce progresivamente las dimensiones hasta cumplir el presupuesto de peso (dentro de los límites físicos de cabecera del formato).
- **Panel de métricas en vivo:** Muestra el tamaño original, el tamaño comprimido resultante en bytes/KiB, el **porcentaje de ahorro (`savingsPercent`)**, la calidad elegida y las dimensiones finales.

---

### `F07` — Optimizador para Web y Diseño Responsivo (`web`)
Genera de una sola vez el conjunto de variantes responsivas listas para etiquetas HTML `<picture>` y `srcset` mediante [`CompressWebOp::generateWebVariants`](file:///home/jblaz/dev/photo26/src/core/ops/compress_web_op.hpp).

- **Variantes responsivas estándar:** Casillas de selección rápida para los 5 anchos estándar de diseño web moderno: **`320 px`**, **`640 px`**, **`960 px`**, **`1280 px`** y **`1920 px`**.
- **Características clave:**
  - Conserva siempre la relación de aspecto original e impide ampliaciones artificiales (`preventUpscaling`: si la imagen original mide `800 px`, la variante `1280w` conserva `800 px` reales indicando su `actualSize`).
  - **Limpieza de metadatos web (`stripMetadata`):** Elimina textos y etiquetas EXIF/XMP/IPTC innecesarios para ahorrar peso, conservando el perfil de color para una visualización fiel en navegadores.
  - **Patrón de nombres seguro:** Configurable mediante plantillas como `{name}-{width}w.{ext}` (con validación contra colisiones y bloqueo de rutas fuera de la carpeta de destino).

---

### `F08` — Generador de Favicon e Iconos de Aplicación (`favicon`)
Crea paquetes completos de identidad visual para escritorio, navegadores web y aplicaciones progresivas (PWA) mediante [`IcoCodec`](file:///home/jblaz/dev/photo26/src/core/io/ico_codec.hpp).

- **Salidas generadas en un solo paso (`exportFaviconBundle`):**
  1. **`favicon.ico` multi-resolución:** Contenedor binario Windows ICO (`ICONDIR` / `ICONDIRENTRY`) que empaqueta internamente las vistas escaladas de `16×16`, `32×32` y `48×48` (y soporte hasta `256×256` en PNG de 32 bits con alfa).
  2. **Colección de iconos PNG (`16..512 px`):** Exportación automática de `favicon-16x16.png`, `favicon-32x32.png`, `favicon-48x48.png`, `apple-touch-icon.png` (`180×180`), `icon-192x192.png` y `icon-512x512.png`.
  3. **Manifiesto Web PWA (`site.webmanifest`):** Archivo JSON estándar preconfigurado con el nombre de tu aplicación (`appName`, `appShortName`), colores de tema y referencias a los iconos generados.
- **Modos de encaje (`IconFitMode`):** Ajustar proporcionalmente (`ContainProportional`), cubrir recortando (`CoverCrop`) o estirar (`Stretch`), más control de **margen interior (`paddingPercent`)** y fondo transparente o color sólido.

---

### `F09` — Gestor, Inspector y Limpiador de Metadatos (`metadata`)
Motor integral y transparente ([`MetadataEngine`](file:///home/jblaz/dev/photo26/src/core/metadata/metadata_engine.hpp)) que manipula directamente los contenedores `JPEG`, `PNG`, `WebP` y `TIFF` sin recodificar los datos de imagen comprimidos cuando se realiza limpieza o inyección directa.

#### 1. Distinción clara: Propiedades del archivo en disco frente a Metadatos incrustados
La interfaz separa visualmente:
- **Propiedades del sistema de archivos (`FileSystemProperties`):** Nombre de archivo, ruta absoluta, tamaño exacto en bytes, fecha ISO de creación y modificación en disco, ancho, alto, profundidad de bits (`bitDepth`), presencia de canal alfa (`hasAlpha`) y formato real detectado por firma mágica (`detectedFormat`). **Estas propiedades pertenecen al archivo en el sistema operativo o a la geometría del bitmap**, por lo que no se «borran» al limpiar metadatos fotográficos.
- **Metadatos incrustados (`ImageMetadata`):** Segmentos y bloques internos **EXIF** (CIPA DC-008), **XMP** (ISO 16684-1), **IPTC IIM** (Record 2), coordenadas **GPS** y perfil de color **ICC**.

#### 2. Los 15 Campos Editables de Metadatos
Puedes consultar y editar directamente desde el panel lateral o en lote los **15 campos normalizados**:
1. **Autor (`author`):** Artista / Creador (`EXIF Artist`, `XMP dc:creator`, `IPTC By-line`).
2. **Copyright (`copyright`):** Aviso legal de derechos de autor (`EXIF Copyright`, `XMP dc:rights`, `IPTC CopyrightNotice`).
3. **Título (`title`):** Título del documento o fotografía (`EXIF DocumentName`, `XMP dc:title`, `IPTC ObjectName`).
4. **Descripción (`description`):** Leyenda o resumen (`EXIF ImageDescription`, `XMP dc:description`, `IPTC Caption/Abstract`).
5. **Fecha de captura (`captureDate`):** Fecha/hora original de toma (`EXIF DateTimeOriginal`, `XMP exif:DateTimeOriginal`, `IPTC DateCreated`).
6. **Fecha de modificación (`modificationDate`):** Fecha de edición (`EXIF DateTime`, `XMP xmp:ModifyDate`).
7. **Marca y modelo de cámara (`cameraMakeModel`):** Equipo de captura (`EXIF Make/Model`, `XMP tiff:Model`).
8. **Modelo de objetivo (`lensModel`):** Lente utilizada (`EXIF LensModel`, `XMP aux:Lens`).
9. **Software (`software`):** Aplicación de creación/procesado (`EXIF Software`, `XMP xmp:CreatorTool`).
10. **Orientación EXIF (`orientation`):** Valor entero `1..8` (con soporte de aplicación geométrica mediante `applyExifOrientation`).
11. **Resolución horizontal (`dpiX`):** Puntos por pulgada en eje X (`1..1,000,000`).
12. **Resolución vertical (`dpiY`):** Puntos por pulgada en eje Y (`1..1,000,000`).
13. **Palabras clave (`keywords`):** Lista de etiquetas separadas por comas (`XMP dc:subject`, `IPTC Keywords`).
14. **Licencia (`license`):** Términos de uso (`XMP xmpRights:UsageTerms`, chunk `License` en PNG).
15. **Coordenadas GPS (`gps`):** Latitud (`-90°..+90°`), Longitud (`-180°..+180°`) y Altitud opcional en metros (`EXIF GPS IFD` y `XMP exif:GPS*`).

*(Además, el inspector muestra el nombre y tamaño del **Perfil de color ICC** incrustado y las etiquetas adicionales detectadas en `extraTags`).*

#### 3. Limpieza Total o Selectiva por Privacidad
- **Limpieza total (`stripAllMetadata`):** Elimina de una pasada EXIF, XMP, IPTC, GPS e ICC.
- **Limpieza selectiva (`stripSelectedMetadata`):** Marca mediante casillas independientes qué bloques deseas suprimir (**Quitar EXIF**, **Quitar XMP**, **Quitar IPTC**, **Quitar GPS**, **Quitar perfil ICC**) manteniendo intactos los demás y sin recomprimir el flujo de píxeles en `JPEG`, `PNG`, `WebP` y `TIFF`.

#### 4. Copia de Metadatos entre Imágenes y Edición por Lotes
- **Copiar metadatos entre archivos (`copyMetadataBetweenFiles`):** Extrae los metadatos incrustados de una fotografía de origen y los inyecta en otra imagen de destino.
- **Actualización por lotes (`batchUpdateMetadata`):** Aplica un subconjunto de campos modificados (por ejemplo, `author`, `copyright` y `license`) a todas las imágenes cargadas en la sesión, fusionándolos con los metadatos individuales existentes de cada archivo.

#### 5. Exportación e Importación JSON con Esquema Documentado `photo26.metadata.v1`
Photo26 permite exportar los metadatos a un archivo `.json` e importarlos de vuelta con validación estricta de tipos y rangos (`ErrorCode::SchemaValidationError` ante datos malformados):

```json
{
  "schema": "photo26.metadata.v1",
  "author": "Estudio Photo26",
  "copyright": "© 2026 Estudio Photo26",
  "title": "Amanecer en la sierra",
  "description": "Fotografía panorámica en formato horizontal",
  "captureDate": "2026:11:04 07:30:00",
  "modificationDate": "2026:11:04 12:00:00",
  "cameraMakeModel": "Canon EOS R5",
  "lensModel": "RF 24-70mm F2.8 L IS USM",
  "software": "Photo26 v1.0",
  "orientation": 1,
  "dpiX": 300,
  "dpiY": 300,
  "keywords": ["paisaje", "sierra", "amanecer"],
  "license": "CC BY-NC 4.0",
  "colorProfileName": "sRGB IEC61966-2.1",
  "iccProfileBytes": "<base64-del-perfil-icc>",
  "hasExif": true,
  "hasXmp": true,
  "hasIptc": true,
  "hasGps": true,
  "hasIcc": true,
  "gps": {
    "latitude": 40.416775,
    "longitude": -3.703790,
    "altitude": 650.0,
    "isValid": true
  },
  "extraTags": {},
  "fileProps": {
    "fileName": "foto.jpg",
    "filePath": "C:/Fotos/foto.jpg",
    "fileSizeBytes": 2458112,
    "createdIso": "2026-11-04T07:30:00.000",
    "modifiedIso": "2026-11-04T12:00:00.000",
    "width": 4000,
    "height": 2667,
    "bitDepth": 24,
    "hasAlpha": false,
    "detectedFormat": "jpg"
  }
}
```

#### 6. Advertencias de Compatibilidad de Formato
El método `checkFormatCompatibilityWarnings` avisa de forma proactiva y honesta en la interfaz cuando el contenedor de destino tiene limitaciones técnicas:
- En **PNG y WebP**, el bloque binario heredado `IPTC IIM` no forma parte del contenedor nativo, por lo que Photo26 informa de que Autor, Título, Descripción y Palabras clave se conservan íntegramente en el paquete **XMP** (y chunks textuales `iTXt`/`tEXt` en PNG).
- En **GIF e ICO**, los contenedores no admiten bloques EXIF/XMP/IPTC/ICC sin romper la estructura multipágina o animada, informando de ello sin corromper los fotogramas.
- Cuando un texto contiene caracteres Unicode fuera de ASCII, Photo26 advierte de que las etiquetas `EXIF ASCII` sustituyen caracteres no ASCII por `?` mientras que **XMP conserva el texto Unicode completo**.

---

### `F10` — Procesamiento por Lotes (`batch`)
Automatiza tuberías completas sobre decenas o cientos de fotografías con consumo de RAM constante gracias al motor en streaming [`BatchProcessor`](file:///home/jblaz/dev/photo26/src/core/ops/batch_rename_op.hpp).

- **Etapas combinables en una misma ejecución:**
  1. **Redimensionado (`enableResize`):** Aplicando cualquier configuración de `ResizeOptions`.
  2. **Ajustes de color (`enableColorAdjust`):** Aplicando filtros o correcciones de `ColorAdjustOptions`.
  3. **Marca de agua (`enableWatermark`):** Estampando texto o logotipo de `WatermarkOptions`.
  4. **Renombrado secuencial (`renameOptions`):** Aplicando patrones de `MassRenameOptions`.
  5. **Conversión de formato, control de calidad/lossless y limpieza de metadatos (`outputFormat`, `quality`, `lossless`, `stripMetadata`).**
- **Presets JSON (`presetToJson` / `presetFromJson`):** Guarda tus configuraciones habituales de lote en archivos JSON (`schemaVersion: 1`, incluyendo marcas de agua de imagen codificadas en Base64) y recupéralas en un clic.
- **Ejecución robusta:**
  - **Barra de progreso en tiempo real** mediante `ProgressCallback`.
  - **Cancelación cooperativa inmediata** mediante `CancellationToken`: detiene la cola de trabajo sin dejar archivos temporales corruptos.
  - **Registro detallado de errores por archivo (`batchLogView`):** Si un archivo de entrada está dañado, el lote registra el fallo con su causa y continúa procesando el resto de imágenes sin abortar toda la sesión.

---

### `F11` — Marcas de Agua de Texto e Imagen (`watermark`)
Protege la autoría de tus imágenes superponiendo rótulos tipográficos o logotipos gráficos mediante [`WatermarkTextOp::applyWatermark`](file:///home/jblaz/dev/photo26/src/core/ops/watermark_text_op.hpp).

- **Marca de agua de texto:** Fuente tipográfica configurable (`Segoe UI` por defecto), tamaño en puntos, negrita, color con canal alfa y contorno de contraste (`textOutline` / `outlineColor`) para garantizar legibilidad sobre fondos claros y oscuros.
- **Marca de agua de imagen (logotipo):** Carga cualquier logotipo PNG con transparencia y ajusta su **escala porcentual (`scalePercent`)** respecto al lienzo.
- **Posicionamiento y estilo:**
  - **10 posiciones de anclaje (`AnchorPosition`):** Esquinas (`TopLeft`, `TopRight`, `BottomLeft`, `BottomRight`), centros laterales (`TopCenter`, `MiddleLeft`, `Center`, `MiddleRight`, `BottomCenter`) y modo **Mosaico repetido (`Tiled`)** en toda la imagen.
  - Control preciso de **opacidad (`0.0` a `1.0`)**, **márgenes X/Y** en píxeles y **ángulo de rotación (`rotationDegrees`)**.

---

### `F12` — Texto, Marcos y Bordes (`text_border`)
Añade rótulos editoriales, márgenes interiores, marcos decorativos y esquinas redondeadas mediante [`WatermarkTextOp::applyTextAndBorders`](file:///home/jblaz/dev/photo26/src/core/ops/watermark_text_op.hpp).

- **Bordes y expansión de lienzo:**
  - **Relleno interior (`paddingPx`)** y **Grosor de borde (`borderWidthPx`)** que amplían limpiamente el lienzo alrededor de la fotografía con colores personalizables (`borderColor` y `canvasBackground`).
  - **Radio de esquinas redondeadas (`cornerRadiusPx`):** Recorta suavemente con antialiasing las esquinas exteriores generando transparencia alfa (o el fondo elegido).
- **Rótulo tipográfico:** Texto superpuesto con selección de fuente, tamaño, negrita/cursiva, color de texto, caja de fondo semitransparente (`textBoxBackground`), posición de anclaje y margen.

---

### `F13` — Animaciones GIF: Creación y Extracción (`gif`)
Herramienta bidireccional para crear secuencias animadas **GIF89a** o descomponer archivos `.gif` existentes en fotogramas individuales mediante [`GifCodec`](file:///home/jblaz/dev/photo26/src/core/io/gif_codec.hpp).

- **Creación de GIF animado (`encodeAnimatedGif` / `saveAnimatedGif`):**
  - Carga múltiples imágenes en el espacio de trabajo para usarlas como secuencia de fotogramas.
  - Configura el **retardo entre fotogramas en milisegundos (`defaultDelayMs`)**, el **conteo de bucles (`loopCount`, donde `0` = bucle infinito `NETSCAPE2.0`)**, dimensiones objetivo opcionales y **tramado (`dither`)** en la cuantización a paleta de 256 colores por fotograma.
- **Extracción de fotogramas (`extractGifFrames` / `exportGifFramesToFolder`):**
  - Abre un archivo `.gif` animado existente para inspeccionar su número de fotogramas y retardos originales, y expórtalos como archivos individuales numerados (`frame_001.png`, `frame_002.png`, …) hacia cualquier carpeta.

---

### `F14` — Conversión de Imágenes a PDF (`pdf`)
Genera documentos PDF de una sola página o multipágina de forma 100% local mediante [`PdfExporter`](file:///home/jblaz/dev/photo26/src/core/io/pdf_exporter.hpp) sobre `QPdfWriter`.

- **Modos de uso:**
  - **Imagen única a PDF (`exportSingleImageToPdf`):** Convierte la imagen activa en una hoja PDF lista para imprimir o enviar.
  - **Múltiples imágenes a PDF multipágina (`exportImagesToPdf` / `exportImageFilesToPdf`):** Combina todas las imágenes cargadas en el espacio de trabajo en un único documento `.pdf` paginado en orden, con soporte de cancelación cooperativa y barra de progreso.
- **Parámetros de maquetación (`PdfExportOptions`):**
  - **Tamaño de página:** `A4`, `Letter` (Carta), `Legal`, `A3` o **Ajustar al tamaño exacto de cada imagen (`FitToImage`)**.
  - **Orientación:** Vertical (`Portrait`), Horizontal (`Landscape`) o **Automática según cada imagen (`AutoFromImage`)**.
  - **Márgenes en milímetros (`marginMm`)**, **resolución DPI (`72..1200`, `300 DPI` por defecto)**, ajuste proporcional sin deformación (`proportionalFit`), centrado en página y color de fondo de página.

---

### `F15` — Generador de Hojas de Sprites / Spritesheets (`spritesheet`)
Empaqueta una colección de fotogramas o iconos en un único atlas de textura junto con su mapa JSON de coordenadas exactas mediante [`SpritesheetOp`](file:///home/jblaz/dev/photo26/src/core/ops/spritesheet_op.hpp), ideal para motores de videojuegos y animaciones UI.

- **Parámetros del atlas (`SpritesheetOptions`):**
  - Número de **columnas (`columns`)** y **filas (`rows`)** (si se dejan en `0`, calcula automáticamente una rejilla equilibrada).
  - **Espaciado entre sprites (`spacingPx`)** y **margen perimetral (`paddingPx`)**.
  - Dimensiones de celda forzadas opcionales (`forcedCellWidth`, `forcedCellHeight`, reduciendo proporcionalmente sin deformar si el sprite es mayor) y color de fondo (transparente por defecto).
- **Doble salida sincronizada (`saveSpritesheetAndJson`):**
  - Exporta la imagen del atlas (`.png` / `.webp`) y muestra/guarda el **archivo JSON de coordenadas (`coordinatesJson`)** con el nombre, posición `x`, `y`, `width` y `height` exactos del rectángulo dibujado de cada fotograma.

---

### `F16` — Extractor de Paletas de Colores (`palette`)
Analiza cromáticamente cualquier imagen mediante agrupamiento **k-means++ determinista** (semilla fija `26`, hasta `65,536` muestras ponderadas por alfa, excluyendo píxeles totalmente transparentes) en [`PaletteExtractorOp`](file:///home/jblaz/dev/photo26/src/core/ops/palette_compare_bg_op.hpp).

- **Información extraída (`PaletteColor`):**
  - Selecciona el número de colores dominantes a extraer (por ejemplo, de `2` a `16` colores).
  - Muestra en una tabla interactiva cada muestra de color con su código **HEX (`#RRGGBB`)**, componentes **RGB (`0..255`)** y su **porcentaje de dominancia (`dominanceRatio`)** normalizado sobre los píxeles visibles.
- **4 Formatos de exportación (`PaletteExportFormat`):**
  1. **JSON (`Json`):** Lista estructurada con códigos HEX, RGB y proporción de dominancia.
  2. **Variables CSS (`CssVariables`):** Bloque `:root { --palette-color-1: #...; }` listo para pegar en hojas de estilo web.
  3. **Paleta GIMP (`GimpGpl`):** Archivo `.gpl` estándar compatible con GIMP, Inkscape, Krita y Aseprite.
  4. **Muestra visual PNG (`SwatchPng`):** Imagen generada (`renderPaletteSwatch`) con bandas verticales de color y sus códigos HEX rotulados.

---

### `F17` — Comparación Visual y Analítica de Imágenes (`compare`)
Compara dos imágenes cargadas en el espacio de trabajo (Imagen A e Imagen B) tanto visualmente como mediante métricas matemáticas objetivas con [`ImageCompareOp`](file:///home/jblaz/dev/photo26/src/core/ops/palette_compare_bg_op.hpp).

- **4 Modos de visualización (`ComparisonMode`):**
  1. **Lado a lado (`SideBySide`):** Presenta ambas imágenes contiguas en un mismo lienzo.
  2. **Deslizador Antes / Después (`SplitSlider`):** Cortina divisoria ajustable (`0.0` a `1.0`) que muestra la Imagen A a la izquierda y la Imagen B a la derecha (también disponible en cualquier herramienta desde el botón **Comparar Antes/Después** de la barra superior).
  3. **Superposición (`OverlayBlend`):** Mezcla la Imagen B sobre la Imagen A con opacidad regulable.
  4. **Diferencia visual (`VisualDifference`):** Mapa de calor de diferencias píxel a píxel que resalta cualquier artefacto de compresión o cambio sutil.
- **Métricas objetivas de calidad (`ComparisonMetrics`):**
  - **PSNR (`psnrDb`):** Relación señal/ruido pico en decibelios (dB, pico `255`; muestra `∞` si ambas imágenes son idénticas al bit).
  - **MAE (`meanAbsoluteError`):** Error absoluto medio en escala `0..255` sobre los 4 canales RGBA premultiplicados.
  - **RMSE (`rmse`):** Raíz del error cuadrático medio en escala `0..255`.
  - **Porcentaje de píxeles diferentes (`diffPixelPercent`)** y comparativa de dimensiones y peso en bytes.

---

### `F18` — Eliminación Local de Fondos y Transparencia (`bg_remove`)
Extrae fondos de fotografías de producto, logotipos, retratos sobre estudio o capturas de forma **100% local en CPU, sin enviar datos a la nube ni requerir descargas externas**, mediante [`BackgroundRemovalOp`](file:///home/jblaz/dev/photo26/src/core/ops/palette_compare_bg_op.hpp).

- **3 Métodos de segmentación local (`BgRemovalMethod`):**
  1. **Segmentación automática de bordes en espacio Lab (`AutoBorderFloodAndGrabCut`):** Analiza la distribución cromática CIE Lab de los cuatro bordes de la imagen (estimando hasta 12 modos de color de fondo) y ejecuta un llenado por inundación exterior (*exterior flood-fill*) conectado para aislar el objeto central sin borrar zonas internas del mismo color.
  2. **Clave de color / Chroma Key (`ChromaKeyColor`):** Elimina globalmente en toda la imagen los píxeles cuya distancia perceptual en espacio CIE Lab respecto al **Color clave (`keyColor`)** sea inferior a la tolerancia indicada.
  3. **Umbral de luminancia (`LuminanceThreshold`):** Elimina fondos claros u oscuros según su proximidad en luminancia Rec.709 al color clave seleccionado.
- **Acabado y exportación:**
  - **Tolerancia (`tolerance`):** Ajusta la sensibilidad del recorte cromático.
  - **Suavizado de bordes (`featherRadius`):** Aplica un filtro gaussiano separable sobre el canal alfa generado para obtener transiciones suaves sin bordes dentados.
  - **Salida:** Exporta directamente como **PNG/WebP con fondo transparente** o activa **Reemplazar por color sólido (`replaceWithSolidColor`)** para colocar un nuevo fondo uniforme (blanco, corporativo, etc.).

---

### `F19` — Renombrado Masivo Inteligente (`rename`)
Organiza colecciones fotográficas con vista previa instantánea y verificación preventiva de colisiones mediante [`MassRenameOp`](file:///home/jblaz/dev/photo26/src/core/ops/batch_rename_op.hpp).

- **Marcadores de patrón soportados (`pattern`):**
  - `{name}`: Nombre base original del archivo.
  - `{num}` o `{index}`: Contador secuencial formateado con relleno de ceros (`zeroPadding`, número inicial `startNumber` e incremento `numberStep`).
  - `{date}`: Fecha local de modificación del archivo en formato `yyyy-MM-dd`.
  - `{width}` y `{height}`: Ancho y alto reales de la imagen en píxeles.
  - `{ext}`: Extensión de salida.
- **Modificadores adicionales:** Añade **Prefijo (`prefix`)**, **Sufijo (`suffix`)** y **Buscar / Reemplazar texto (`findText` / `replaceText`)** sobre el nombre resultante.
- **Vista previa en vivo y detección de conflictos (`previewRename`):**
  - La tabla inferior muestra para cada archivo el **Nombre original**, el **Nombre propuesto** y su **Estado de validación**.
  - El motor detecta y bloquea automáticamente nombres ilegales en Windows (caracteres `<>:"/\|?*` o nombres reservados `CON`, `PRN`, `AUX`, `NUL`, `COM1..9`, `LPT1..9`), nombres duplicados dentro del mismo lote y colisiones con archivos existentes en la carpeta de destino.
  - Al ejecutar (`executeRenameOrCopy`), cada archivo se confirma atómicamente en disco antes de retirar el original.

---

### `F20` — Guardado y Exportación No Destructiva (`ExportBarWidget`)
Situada de forma permanente en la parte inferior del espacio de trabajo, la barra de exportación ([`ExportBarWidget`](file:///home/jblaz/dev/photo26/src/desktop/widgets/export_bar.hpp)) centraliza toda la salida de archivos de Photo26 bajo garantías estrictas de seguridad:

1. **«Guardar como…» (`Ctrl+S` — Archivo único):**
   - Abre el diálogo nativo de Windows (`QFileDialog::getSaveFileName`) preseleccionando la última carpeta utilizada.
   - Sugiere automáticamente un **nombre descriptivo en español** basado en el archivo original y la herramienta activa mediante [`SafeFileWriter::suggestOutputFileName`](file:///home/jblaz/dev/photo26/src/core/common/safe_file_writer.hpp) (por ejemplo: `retrato_convertida.webp`, `paisaje_recortada.png`, `producto_sin_fondo.png`, `foto_comprimida.jpg`), evitando sobrescribir tu archivo original.
   - Escribe el archivo de forma transaccional y atómica en el sistema de archivos NTFS.
2. **«Guardar archivos…» (`Ctrl+Shift+S` — Exportación múltiple a carpeta):**
   - Permite seleccionar una carpeta de destino y un patrón de nombres (`{name}_{suffix}.{ext}`), mostrando una tabla previa con detección de conflictos antes de escribir todos los archivos procesados, variantes web o fotogramas extraídos.
3. **«Guardar como ZIP…» (Empaquetado directo PKZIP):**
   - Empaqueta todos los resultados de una operación multi-archivo directamente en un contenedor estándar `.zip` (compresión DEFLATE + suma de verificación CRC-32 IEEE 802.3 mediante [`ZipWriter`](file:///home/jblaz/dev/photo26/src/core/io/zip_writer.hpp)) elegido con un diálogo «Guardar como…».
4. **Controles rápidos de salida:** Selector rápido de formato, deslizador de calidad (`1..100`), casilla *Sin pérdida (Lossless)*, casilla *Conservar metadatos* y botón **«Abrir carpeta»** para mostrar el último archivo exportado en el Explorador de Windows.
