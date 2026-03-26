#!/bin/bash
# Script para descargar y convertir datos LSQB a formato Ring TSV

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== LSQB to Ring TSV Converter ===${NC}"
echo

# Verificar argumentos
if [ $# -lt 1 ]; then
    echo "Uso: $0 <scale_factor> [output_prefix]"
    echo
    echo "Scale factors disponibles:"
    echo "  - 0.003 : ~51 personas (muy pequeño, para pruebas)"
    echo "  - 1     : ~11,000 personas"
    echo "  - 3     : ~33,000 personas"
    echo "  - 10    : ~110,000 personas"
    echo "  - 30    : ~330,000 personas"
    echo "  - 100   : ~1,100,000 personas"
    echo
    echo "Ejemplo:"
    echo "  $0 0.003"
    echo "  $0 1 lsqb-sf1"
    exit 1
fi

SCALE_FACTOR=$1
OUTPUT_PREFIX=${2:-"lsqb-sf${SCALE_FACTOR}"}
DATA_DIR="/tmp/lsqb-data"
LSQB_DIR="${DATA_DIR}/social-network-sf${SCALE_FACTOR}-projected-fk"

echo -e "${YELLOW}Scale factor:${NC} ${SCALE_FACTOR}"
echo -e "${YELLOW}Output prefix:${NC} ${OUTPUT_PREFIX}"
echo

# Verificar si existe el ejecutable
if [ ! -f "./build/lsqb2tsv" ]; then
    echo -e "${RED}Error: No se encuentra ./build/lsqb2tsv${NC}"
    echo "Por favor compila primero:"
    echo "  mkdir -p build && cd build && cmake .. && make lsqb2tsv"
    exit 1
fi

# Verificar si ya existen los datos
if [ -d "${LSQB_DIR}" ]; then
    echo -e "${GREEN}✓${NC} Los datos ya existen en ${LSQB_DIR}"
else
    echo -e "${YELLOW}Descargando datos LSQB...${NC}"

    # Crear directorio temporal
    mkdir -p "${DATA_DIR}"
    cd "${DATA_DIR}"

    # Clonar repositorio si no existe
    if [ ! -d "lsqb" ]; then
        echo "Clonando repositorio LSQB..."
        git clone --depth 1 https://github.com/ldbc/lsqb.git
    fi

    cd lsqb

    # Descargar datos
    echo "Descargando scale factor ${SCALE_FACTOR}..."
    export MAX_SF=${SCALE_FACTOR}
    bash scripts/download-projected-fk-data-sets.sh

    # Verificar que se descargaron
    if [ ! -d "${LSQB_DIR}" ]; then
        echo -e "${RED}Error: No se pudieron descargar los datos${NC}"
        exit 1
    fi

    echo -e "${GREEN}✓${NC} Datos descargados correctamente"
fi

# Volver al directorio de ring-pg
cd - > /dev/null

# Convertir datos
echo
echo -e "${YELLOW}Convirtiendo datos a formato Ring TSV...${NC}"
./build/lsqb2tsv "${LSQB_DIR}" "${OUTPUT_PREFIX}"

# Verificar archivos de salida
if [ -f "${OUTPUT_PREFIX}-nodes.tsv" ] && [ -f "${OUTPUT_PREFIX}-edges.tsv" ]; then
    echo
    echo -e "${GREEN}✓ Conversión completada exitosamente!${NC}"
    echo
    echo "Archivos generados:"
    ls -lh "${OUTPUT_PREFIX}-nodes.tsv" "${OUTPUT_PREFIX}-edges.tsv"
    echo
    echo "Estadísticas:"
    NODE_COUNT=$(wc -l < "${OUTPUT_PREFIX}-nodes.tsv")
    EDGE_COUNT=$(wc -l < "${OUTPUT_PREFIX}-edges.tsv")
    echo "  Nodos: ${NODE_COUNT}"
    echo "  Aristas: ${EDGE_COUNT}"
    echo
    echo "Siguiente paso - construir índices Ring:"
    echo "  ./build/tsv2ids ${OUTPUT_PREFIX}"
    echo "  ./build/build-index ${OUTPUT_PREFIX}"
else
    echo -e "${RED}Error: No se generaron los archivos de salida${NC}"
    exit 1
fi

