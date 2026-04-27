import sys

def calcular_mediana(archivo, columna=0, timeout=600):
    valores = []

    with open(archivo, 'r') as f:
        for linea in f:
            partes = linea.strip().split('\t')

            try:
                valor = float(partes[columna])
                # Si el valor es timeout o mayor, usar timeout
                if valor >= timeout:
                    valores.append(timeout)
                else:
                    valores.append(valor)
            except (ValueError, IndexError):
                # Si hay error, asumir timeout
                valores.append(timeout)

    if len(valores) == 0:
        print("No hay datos para calcular la mediana")
        return

    # Ordenar valores
    valores.sort()
    n = len(valores)

    # Calcular mediana
    if n % 2 == 0:
        mediana = (valores[n // 2 - 1] + valores[n // 2]) / 2
    else:
        mediana = valores[n // 2]

    print(mediana)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python mediana.py archivo.tsv [columna] [timeout]")
        sys.exit(1)

    archivo = sys.argv[1]
    columna = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    timeout = float(sys.argv[3]) if len(sys.argv) > 3 else 600

    calcular_mediana(archivo, columna, timeout)
