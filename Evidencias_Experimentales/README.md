# Evidencias experimentales de la PCB USV

Este directorio está destinado a conservar las **evidencias originales de las pruebas funcionales de la tarjeta de control de navegación USV**, de forma separada del código fuente del microcontrolador.

## Inventario preparado (22 de octubre de 2026)

- 39 archivos originales: registros CSV, figuras, un archivo XLSX y paquetes ZIP complementarios.
- 12 grupos organizados por la nomenclatura de ensayo (P03, P04, P05, P06, P07, P08, P12, P13/P14, P16 y P17, incluidos registros adicionales).
- El paquete listo para incorporar se denomina `Evidencias_Experimentales_USV_Organizadas.zip`.
- En el paquete se incluyen `INDICE_EVIDENCIAS.csv` (rutas y huellas SHA-256) y un README detallado.

**Estado actual:** esta página prepara el destino; los registros binarios se incorporarán cuando se suba el ZIP. No se debe interpretar la presencia de este README como publicación completa de las evidencias.

## Importación automática de un solo ZIP

1. Descargue el paquete `Evidencias_Experimentales_USV_Organizadas.zip` preparado junto con el trabajo.
2. En GitHub, abra esta carpeta, seleccione **Add file → Upload files** y cargue ese único ZIP aquí mismo, sin cambiarle el nombre.
3. Confirme la carga directamente en la rama `main`.
4. El flujo `Importar evidencias USV` comprobará que las entradas estén bajo esta carpeta, extraerá los archivos y comparará su SHA-256 con el índice. Si todo coincide, creará una confirmación con las evidencias organizadas y eliminará el ZIP temporal.
5. Verifique el resultado en la pestaña **Actions**. Si el flujo está deshabilitado o no tiene permiso de escritura, la carga permanecerá pendiente hasta que se resuelva.

## Integridad y alcance científico

Los archivos originales no deben modificarse. La clasificación por código de prueba es organizativa y no equivale a certificar una prueba como exitosa. Las conclusiones científicas deben seguir sustentadas en el análisis de cada archivo, el artículo y los anexos.

**Repositorio público:** los registros GNSS de P05 contienen o pueden contener fechas y coordenadas reales. El responsable del repositorio autorizó su publicación y debe verificar el tratamiento institucional de los datos operacionales.
