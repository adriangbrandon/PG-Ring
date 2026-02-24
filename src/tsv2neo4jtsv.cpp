//
// Created by adrian on 24/2/26.
//

#include "tsv/tsv_parser.hpp"
#include "query/constant_utils.hpp"
#include <set>
#include <sstream>
#include <map>

// Función para formatear fechas para Neo4j
std::string format_datetime_for_neo4j(const std::string& date_str) {
    // Neo4j acepta formato ISO 8601: YYYY-MM-DDTHH:MM:SS[.sss][Z|±HH:MM]
    // Nuestras fechas vienen como: +1830-10-04T00:00:00Z
    // Necesitamos eliminar el signo inicial si es positivo
    std::string cleaned = tsv_helper::fix_date_cypher(date_str);

    // Si comienza con '+', lo eliminamos (Neo4j no lo necesita para fechas positivas)
    if (!cleaned.empty() && cleaned[0] == '+') {
        return cleaned.substr(1);
    }

    return cleaned;
}


// Función para extraer todas las propiedades únicas de los nodos y sus tipos
void collect_node_properties(const std::string& nodes_file,
                             std::set<std::string>& all_properties,
                             std::map<std::string, std::string>& types) {
    std::ifstream nodes(nodes_file);
    std::string line;

    while (std::getline(nodes, line)) {
        auto node = tsv_helper::parse_node(line);
        for (const auto& prop : node.properties) {
            all_properties.insert(prop.key);

            // Detectar si es una fecha
            int64_t data;
            if (ring::query::constant::is_date(prop.value, data)) {
                if (types.find(prop.key) != types.end() && types[prop.key] != "datetime") {
                    std::cerr << "Warning: Property '" << prop.key << "' has mixed types. Detected both datetime and " << types[prop.key] << ". Defaulting to string." << std::endl;
                    types[prop.key] = "string"; // Si ya se detectó otro tipo, lo marcamos como string
                } else {
                    types[prop.key] = "datetime";
                }
            } else if (ring::query::constant::is_double(prop.value, data)) {
                if (types.find(prop.key) != types.end() && types[prop.key] != "double") {
                    std::cerr << "Warning: Property '" << prop.key << "' has mixed types. Detected both datetime and " << types[prop.key] << ". Defaulting to string." << std::endl;
                    types[prop.key] = "double"; // Si ya se detectó otro tipo, lo marcamos como string
                } else {
                    types[prop.key] = "double";
                }
            } else if (ring::query::constant::is_integer(prop.value, data)) {
                if (types.find(prop.key) != types.end() && types[prop.key] != "long") {
                    std::cerr << "Warning: Property '" << prop.key << "' has mixed types. Detected both datetime and " << types[prop.key] << ". Defaulting to string." << std::endl;
                    types[prop.key] = "string"; // Si ya se detectó otro tipo, lo marcamos como string
                } else {
                    types[prop.key] = "long";
                }
            }else {
                types[prop.key] = "string";
            }
        }
    }
}

// Función para extraer todas las propiedades únicas de las aristas y sus tipos
void collect_edge_properties(const std::string& edges_file,
                             std::set<std::string>& all_properties,
                             std::map<std::string, bool>& is_datetime) {
    std::ifstream edges(edges_file);
    std::string line;

    while (std::getline(edges, line)) {
        auto edge = tsv_helper::parse_edge(line);
        for (const auto& prop : edge.properties) {
            all_properties.insert(prop.key);

            // Detectar si es una fecha
            int64_t date_value;
            if (ring::query::constant::is_date(prop.value, date_value)) {
                is_datetime[prop.key] = true;
            }
        }
    }
}

// Función para escribir el archivo TSV de nodos (formato Neo4j)
void write_nodes_tsv(const std::string& nodes_file,
                     const std::string& output_file,
                     const std::set<std::string>& all_properties,
                     const std::map<std::string, std::string>& types) {
    std::ifstream nodes(nodes_file);
    std::ofstream ofs(output_file);
    std::string line;

    // Escribir encabezado
    // Formato: qid:ID\t:LABEL\tprop1:datetime\tprop2\t...
    ofs << "qid:ID\t:LABEL";
    for (const auto& prop : all_properties) {
        ofs << "\t" << prop;
        // Agregar tipo :datetime si la propiedad es una fecha
        auto it = types.find(prop);
        if (it != types.end()) {
            ofs << ":" << it->second;
        }
    }
    ofs << "\n";

    // Escribir datos de nodos
    while (std::getline(nodes, line)) {
        auto node = tsv_helper::parse_node(line);

        // qid (identificador único)
        ofs << node.variable;

        // Labels (separados por ;)
        ofs << "\t";
        if (!node.labels.empty()) {
            for (size_t i = 0; i < node.labels.size(); ++i) {
                ofs << node.labels[i];
                if (i < node.labels.size() - 1) ofs << ";";
            }
        }

        // Propiedades (en el orden de all_properties)
        for (const auto& prop_name : all_properties) {
            ofs << "\t";
            // Buscar si el nodo tiene esta propiedad
            for (const auto& prop : node.properties) {
                if (prop.key == prop_name) {
                    // Verificar si es datetime y formatear apropiadamente
                    auto it = types.find(prop_name);
                    if (it != types.end()) {
                        if (it->second == "datetime") {
                            int64_t date_val;
                            if (ring::query::constant::is_date(prop.value, date_val)) {
                                ofs << format_datetime_for_neo4j(prop.value);
                            } else {
                                ofs << prop.value;
                            }
                        } if (it->second == "string") {
                            if (prop.value[0] == '"' && prop.value.back() == '"') {
                                //remove existing quotes to avoid double quoting
                                ofs << prop.value.substr(1, prop.value.length() - 2);
                            } else {
                                ofs << prop.value;
                            }
                        }

                    } else {
                        ofs << prop.value;
                    }
                    break;
                }
            }
            // Si no tiene la propiedad, dejar vacío
        }
        ofs << "\n";
    }
}

// Función para escribir el archivo TSV de relaciones (formato Neo4j)
void write_edges_tsv(const std::string& edges_file,
                    const std::string& output_file,
                    const std::set<std::string>& all_properties,
                    const std::map<std::string, bool>& is_datetime) {
    std::ifstream edges(edges_file);
    std::ofstream ofs(output_file);
    std::string line;

    // Escribir encabezado
    // Formato: :START_ID\t:END_ID\t:TYPE\tprop1:datetime\tprop2\t...
    ofs << ":START_ID\t:END_ID\t:TYPE";
    for (const auto& prop : all_properties) {
        ofs << "\t" << prop;
        // Agregar tipo :datetime si la propiedad es una fecha
        auto it = is_datetime.find(prop);
        if (it != is_datetime.end() && it->second) {
            ofs << ":datetime";
        }
    }
    ofs << "\n";

    // Escribir datos de aristas
    while (std::getline(edges, line)) {
        auto edge = tsv_helper::parse_edge(line);

        // START_ID
        ofs << edge.from;

        // END_ID
        ofs << "\t" << edge.to;

        // TYPE
        ofs << "\t" << edge.type;

        // Propiedades (en el orden de all_properties)
        for (const auto& prop_name : all_properties) {
            ofs << "\t";
            // Buscar si la arista tiene esta propiedad
            for (const auto& prop : edge.properties) {
                if (prop.key == prop_name) {
                    // Verificar si es datetime y formatear apropiadamente
                    auto it = is_datetime.find(prop_name);
                    if (it != is_datetime.end() && it->second) {
                        int64_t date_val;
                        if (ring::query::constant::is_date(prop.value, date_val)) {
                            ofs << format_datetime_for_neo4j(prop.value);
                        } else {
                            ofs << prop.value;
                        }
                    } else {
                        ofs << prop.value;
                    }
                    break;
                }
            }
            // Si no tiene la propiedad, dejar vacío
        }
        ofs << "\n";
    }
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <tsv_prefix>" << std::endl;
        std::cout << "Reads <tsv_prefix>-nodes.tsv and <tsv_prefix>-edges.tsv" << std::endl;
        std::cout << "Outputs <tsv_prefix>-neo4j-nodes.tsv and <tsv_prefix>-neo4j-edges.tsv" << std::endl;
        std::cout << std::endl;
        std::cout << "TSV files are compatible with neo4j-admin import:" << std::endl;
        std::cout << "  neo4j-admin database import full \\" << std::endl;
        std::cout << "    --delimiter=TAB \\" << std::endl;
        std::cout << "    --nodes=<tsv_prefix>-neo4j-nodes.tsv \\" << std::endl;
        std::cout << "    --relationships=<tsv_prefix>-neo4j-edges.tsv \\" << std::endl;
        std::cout << "    neo4j" << std::endl;
        return 0;
    }

    std::string tsv_prefix = argv[1];
    std::string nodes_file = tsv_prefix + "-nodes.tsv";
    std::string edges_file = tsv_prefix + "-edges.tsv";
    std::string nodes_tsv = tsv_prefix + "-neo4j-nodes.tsv";
    std::string edges_tsv = tsv_prefix + "-neo4j-edges.tsv";

    std::cout << "Converting TSV to Neo4j TSV format..." << std::endl;

    // Recopilar todas las propiedades únicas de los nodos
    std::cout << "Collecting node properties..." << std::endl;
    std::set<std::string> node_properties;
    std::map<std::string, std::string> types;
    collect_node_properties(nodes_file, node_properties, types);
    std::cout << "Found " << node_properties.size() << " unique node properties" << std::endl;

    // Escribir archivo TSV de nodos
    std::cout << "Writing nodes TSV to " << nodes_tsv << "..." << std::endl;
    write_nodes_tsv(nodes_file, nodes_tsv, node_properties, types);

    // Recopilar todas las propiedades únicas de las aristas
    std::cout << "Collecting edge properties..." << std::endl;
    std::set<std::string> edge_properties;
    std::map<std::string, bool> edge_is_datetime;
    collect_edge_properties(edges_file, edge_properties, edge_is_datetime);
    std::cout << "Found " << edge_properties.size() << " unique edge properties" << std::endl;

    // Escribir archivo TSV de aristas
    std::cout << "Writing edges TSV to " << edges_tsv << "..." << std::endl;
    write_edges_tsv(edges_file, edges_tsv, edge_properties, edge_is_datetime);

    std::cout << "Conversion completed successfully!" << std::endl;
    std::cout << std::endl;
    std::cout << "To import into Neo4j, use:" << std::endl;
    std::cout << "  neo4j-admin database import full \\" << std::endl;
    std::cout << "    --delimiter=TAB \\" << std::endl;
    std::cout << "    --nodes=" << nodes_tsv << " \\" << std::endl;
    std::cout << "    --relationships=" << edges_tsv << " \\" << std::endl;
    std::cout << "    neo4j" << std::endl;

    return 0;
}

