#include <iostream>
#include "include/query/constant_utils.hpp"

int main() {
    std::string test_date = "-34000-00-00T00:00:00Z";
    int64_t result;

    if (ring::query::constant::is_date(test_date, result)) {
        std::cout << "✓ Fecha detectada: " << test_date << std::endl;
        std::cout << "  Valor int64: " << result << std::endl;

        // Convertir de vuelta para verificar
        std::string back = ring::query::constant::int64_to_date(result);
        std::cout << "  Convertido de vuelta: " << back << std::endl;
    } else {
        std::cout << "✗ Fecha NO detectada: " << test_date << std::endl;
    }

    // Probar otras fechas
    std::cout << "\nProbando otras fechas:" << std::endl;

    std::string dates[] = {
        "+2024-12-25T12:30:45Z",
        "2024-12-25T12:30:45Z",
        "-0001-01-01T00:00:00Z",
        "ZONED_DATETIME('-34000-00-00T00:00:00Z')"
    };

    for (const auto& date : dates) {
        if (ring::query::constant::is_date(date, result)) {
            std::cout << "✓ " << date << " -> " << result << std::endl;
        } else {
            std::cout << "✗ " << date << " (no detectada)" << std::endl;
        }
    }

    return 0;
}

