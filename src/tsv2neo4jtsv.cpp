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
                             std::map<std::string, std::string>& types) {
    std::ifstream edges(edges_file);
    std::string line;

    while (std::getline(edges, line)) {
        auto edge = tsv_helper::parse_edge(line);
        for (const auto& prop : edge.properties) {
            all_properties.insert(prop.key);

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

// Función para escribir archivo Cypher con UNWIND (nodos)
void write_nodes_cypher(const std::string& nodes_file,
                        const std::string& output_file,
                        const std::map<std::string, std::string>& types) {
    std::ifstream nodes(nodes_file);
    std::ofstream ofs(output_file);
    std::string line;

    // Leer y procesar nodos en lotes
    const int BATCH_SIZE = 1000;
    std::vector<std::string> batch;
    int total = 0;

    ofs << "// Importación de nodos generada automáticamente\n";
    ofs << "// Total de propiedades: " << types.size() << "\n\n";

    while (std::getline(nodes, line)) {
        auto node = tsv_helper::parse_node(line);

        // Construir mapa de propiedades en formato Cypher
        std::ostringstream props;
        props << "{qid: '" << node.variable << "'";

        // NO agregamos _labels como propiedad, los generaremos directamente

        // Agregar todas las propiedades
        for (const auto& prop : node.properties) {

            props << ", " << prop.key << ": ";

            // Detectar tipo y formatear
            auto it = types.find(prop.key);
            if (it != types.end()) {
                if (it->second == "datetime") {
                    std::string formatted = format_datetime_for_neo4j(prop.value);
                    props << "datetime('" << formatted << "')";
                }else {
                    props << prop.value;
                }
            }
        }
        props << "}";

        // Construir labels como string para el query
        std::string labels_str;
        if (!node.labels.empty()) {
            for (size_t i = 0; i < node.labels.size(); ++i) {
                labels_str += ":" + node.labels[i];
            }
        }

        // Formato: {props: {...}, labels: ':Label1:Label2'}
        std::ostringstream row_obj;
        row_obj << "{props: " << props.str() << ", labels: '" << labels_str << "'}";

        batch.push_back(row_obj.str());

        // Escribir batch cuando alcance el tamaño
        if (batch.size() >= BATCH_SIZE) {
            ofs << "UNWIND [\n";
            for (size_t i = 0; i < batch.size(); ++i) {
                ofs << "  " << batch[i];
                if (i < batch.size() - 1) ofs << ",";
                ofs << "\n";
            }
            ofs << "] AS row\n";
            ofs << "CREATE (n {qid: row.props.qid})\n";
            ofs << "SET n = row.props;\n\n";

            total += batch.size();
            batch.clear();

            if (total % 10000 == 0) {
                std::cout << "  Generated " << total << " nodes...\r" << std::flush;
            }
        }
    }

    // Escribir batch restante
    if (!batch.empty()) {
        ofs << "UNWIND [\n";
        for (size_t i = 0; i < batch.size(); ++i) {
            ofs << "  " << batch[i];
            if (i < batch.size() - 1) ofs << ",";
            ofs << "\n";
        }
        ofs << "] AS row\n";
        ofs << "CREATE (n {qid: row.props.qid})\n";
        ofs << "SET n = row.props;\n\n";

        total += batch.size();
    }

    std::cout << "  Generated " << total << " nodes total.\n";
}

// Función para escribir archivo Cypher con UNWIND (relaciones)
void write_edges_cypher(const std::string& edges_file,
                        const std::string& output_file,
                        const std::map<std::string, std::string>& types) {
    std::ifstream edges(edges_file);
    std::ofstream ofs(output_file);
    std::string line;

    const int BATCH_SIZE = 1000;

    // Agrupar relaciones por tipo
    std::map<std::string, std::vector<std::string>> batches_by_type;
    int total = 0;

    ofs << "// Importación de relaciones generada automáticamente\n\n";

    while (std::getline(edges, line)) {
        auto edge = tsv_helper::parse_edge(line);

        // Construir objeto de relación (sin el tipo, se agrupa por tipo)
        std::ostringstream rel;
        rel << "{start: '" << edge.from << "'";
        rel << ", end: '" << edge.to << "'";

        // Propiedades
        if (!edge.properties.empty()) {
            rel << ", props: {";
            bool first = true;
            for (const auto& prop : edge.properties) {

                if (!first) rel << ", ";
                first = false;

                rel << prop.key << ": ";

                auto it = types.find(prop.key);
                if (it != types.end()) {
                    if (it->second == "datetime") {
                        std::string formatted = format_datetime_for_neo4j(prop.value);
                        rel << "datetime('" << formatted << "')";
                    }else {
                        rel << prop.value;
                    }
                }
            }
            rel << "}";
        } else {
            rel << ", props: {}";
        }
        rel << "}";

        batches_by_type[edge.type].push_back(rel.str());
        total++;

        if (total % 10000 == 0) {
            std::cout << "  Collected " << total << " relationships...\r" << std::flush;
        }
    }

    std::cout << "\n  Writing relationship queries by type...\n";

    // Escribir queries agrupados por tipo
    for (const auto& [rel_type, batch] : batches_by_type) {
        std::cout << "  - Type " << rel_type << ": " << batch.size() << " relationships\n";

        // Escribir en lotes de BATCH_SIZE
        for (size_t start = 0; start < batch.size(); start += BATCH_SIZE) {
            size_t end = std::min(start + BATCH_SIZE, batch.size());

            ofs << "UNWIND [\n";
            for (size_t i = start; i < end; ++i) {
                ofs << "  " << batch[i];
                if (i < end - 1) ofs << ",";
                ofs << "\n";
            }
            ofs << "] AS row\n";
            ofs << "MATCH (start {qid: row.start})\n";
            ofs << "MATCH (end {qid: row.end})\n";
            ofs << "CREATE (start)-[r:" << rel_type << "]->(end)\n";
            ofs << "SET r = row.props;\n\n";
        }
    }

    std::cout << "  Generated " << total << " relationships total.\n";
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
                        } else if (it->second == "string") {
                            if (prop.value[0] == '"' && prop.value.back() == '"') {
                                //remove existing quotes to avoid double quoting
                                ofs << prop.value.substr(1, prop.value.length() - 2);
                            } else {
                                ofs << prop.value;
                            }
                        }else {
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

// Función para escribir el archivo TSV de relaciones (formato Neo4j)
void write_edges_tsv(const std::string& edges_file,
                    const std::string& output_file,
                    const std::set<std::string>& all_properties,
                    const std::map<std::string, std::string>& types) {
    std::ifstream edges(edges_file);
    std::ofstream ofs(output_file);
    std::string line;

    // Escribir encabezado
    // Formato: :START_ID\t:END_ID\t:TYPE\tprop1:datetime\tprop2\t...
    ofs << ":START_ID\t:END_ID\t:TYPE";
    for (const auto& prop : all_properties) {
        ofs << "\t" << prop;
        // Agregar tipo :datetime si la propiedad es una fecha
        auto it = types.find(prop);
        if (it != types.end()) {
            ofs << ":" << it->second;
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
                    auto it = types.find(prop_name);
                    if (it != types.end()) {
                        if (it->second == "datetime") {
                            std::string formatted = format_datetime_for_neo4j(prop.value);
                            ofs << "datetime('" << formatted << "')";
                        }else {
                            ofs  << prop.value;
                        }
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

    if (argc < 2 || argc > 3) {
        std::cout << "Usage: " << argv[0] << " <tsv_prefix> [--cypher]" << std::endl;
        std::cout << "Reads <tsv_prefix>-nodes.tsv and <tsv_prefix>-edges.tsv" << std::endl;
        std::cout << std::endl;
        std::cout << "Output modes:" << std::endl;
        std::cout << "  Default: Generates TSV files for neo4j-admin import" << std::endl;
        std::cout << "    - <tsv_prefix>-neo4j-nodes.tsv" << std::endl;
        std::cout << "    - <tsv_prefix>-neo4j-edges.tsv" << std::endl;
        std::cout << std::endl;
        std::cout << "  --cypher: Generates Cypher UNWIND statements" << std::endl;
        std::cout << "    - <tsv_prefix>-nodes.cypher" << std::endl;
        std::cout << "    - <tsv_prefix>-edges.cypher" << std::endl;
        std::cout << std::endl;
        std::cout << "TSV import command:" << std::endl;
        std::cout << "  neo4j-admin database import full \\" << std::endl;
        std::cout << "    --delimiter=TAB \\" << std::endl;
        std::cout << "    --nodes=<tsv_prefix>-neo4j-nodes.tsv \\" << std::endl;
        std::cout << "    --relationships=<tsv_prefix>-neo4j-edges.tsv \\" << std::endl;
        std::cout << "    neo4j" << std::endl;
        std::cout << std::endl;
        std::cout << "Cypher import command:" << std::endl;
        std::cout << "  cat <tsv_prefix>-nodes.cypher | docker exec -i <container> cypher-shell -u neo4j -p <pass>" << std::endl;
        std::cout << "  cat <tsv_prefix>-edges.cypher | docker exec -i <container> cypher-shell -u neo4j -p <pass>" << std::endl;
        return 0;
    }

    std::string tsv_prefix = argv[1];
    bool use_cypher = (argc == 3 && std::string(argv[2]) == "--cypher");

    std::string nodes_file = tsv_prefix + "-nodes.tsv";
    std::string edges_file = tsv_prefix + "-edges.tsv";

    std::string nodes_output, edges_output;
    if (use_cypher) {
        nodes_output = tsv_prefix + "-nodes.cypher";
        edges_output = tsv_prefix + "-edges.cypher";
        std::cout << "Converting TSV to Cypher UNWIND format..." << std::endl;
    } else {
        nodes_output = tsv_prefix + "-neo4j-nodes.tsv";
        edges_output = tsv_prefix + "-neo4j-edges.tsv";
        std::cout << "Converting TSV to Neo4j TSV format..." << std::endl;
    }

    // Recopilar todas las propiedades únicas de los nodos
    std::cout << "Collecting node properties..." << std::endl;
    std::set<std::string> node_properties;
    std::map<std::string, std::string> node_types;
    collect_node_properties(nodes_file, node_properties, node_types);
    std::cout << "Found " << node_properties.size() << " unique node properties" << std::endl;

    // Escribir archivo de nodos
    if (use_cypher) {
        std::cout << "Writing nodes Cypher to " << nodes_output << "..." << std::endl;
        write_nodes_cypher(nodes_file, nodes_output, node_types);
    } else {
        std::cout << "Writing nodes TSV to " << nodes_output << "..." << std::endl;
        write_nodes_tsv(nodes_file, nodes_output, node_properties, node_types);
    }

    // Recopilar todas las propiedades únicas de las aristas
    std::cout << "Collecting edge properties..." << std::endl;
    std::set<std::string> edge_properties;
    std::map<std::string, std::string> edge_types;
    collect_edge_properties(edges_file, edge_properties, edge_types);
    std::cout << "Found " << edge_properties.size() << " unique edge properties" << std::endl;

    // Escribir archivo de aristas
    if (use_cypher) {
        std::cout << "Writing edges Cypher to " << edges_output << "..." << std::endl;
        write_edges_cypher(edges_file, edges_output, edge_types);
    } else {
        std::cout << "Writing edges TSV to " << edges_output << "..." << std::endl;
        write_edges_tsv(edges_file, edges_output, edge_properties, edge_types);
    }

    std::cout << "Conversion completed successfully!" << std::endl;
    std::cout << std::endl;

    if (use_cypher) {
        std::cout << "To import into Neo4j:" << std::endl;
        std::cout << "  cat " << nodes_output << " | docker exec -i <container> cypher-shell -u neo4j -p <password>" << std::endl;
        std::cout << "  cat " << edges_output << " | docker exec -i <container> cypher-shell -u neo4j -p <password>" << std::endl;
        std::cout << "\nNote: No APOC plugin required - uses pure Cypher" << std::endl;
    } else {
        std::cout << "To import into Neo4j, use:" << std::endl;
        std::cout << "  neo4j-admin database import full \\" << std::endl;
        std::cout << "    --delimiter=TAB \\" << std::endl;
        std::cout << "    --nodes=" << nodes_output << " \\" << std::endl;
        std::cout << "    --relationships=" << edges_output << " \\" << std::endl;
        std::cout << "    neo4j" << std::endl;
    }

    return 0;
}

