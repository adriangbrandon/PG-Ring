import sys

def calcular_media(archivo, columna=0):
    suma = 0.0
    contador = 0

    with open(archivo, 'r') as f:
        for linea in f:
            partes = linea.strip().split('\t')

            try:
                valor = float(partes[columna])
                suma += valor
                contador += 1
            except (ValueError, IndexError):
                # Ignora filas con datos no numéricos o columnas faltantes
                continue

    if contador == 0:
        print("No hay datos válidos para calcular la media")
    else:
        print(suma / contador)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python media_tsv.py archivo.tsv [columna]")
        sys.exit(1)

    archivo = sys.argv[1]

    # Columna opcional (por defecto 0 = primera columna)
    columna = int(sys.argv[2]) if len(sys.argv) > 2 else 0

    calcular_media(archivo, columna)
