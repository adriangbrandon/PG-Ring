import re
import random
from collections import defaultdict

TARGET_SIZE = 1000
random.seed(42)


# =========================================================
# 1. NORMALIZACIÓN
# =========================================================
def normalize_query(query: str) -> str:
    q = query.lower().strip()

    q = re.sub(r"'[^']*'", "?", q)
    q = re.sub(r'"[^"]*"', "?", q)
    q = re.sub(r"\b\d+\b", "?", q)

    # marcar nodos con {qid:...}
    q = re.sub(
        r"\(\s*[^()]*?\{\s*qid\s*:\s*[^}]*\}\s*\)",
        "__CONST_NODE__",
        q
    )

    # eliminar resto propiedades
    q = re.sub(r"\{[^}]*\}", "", q)

    # eliminar variables nodos
    q = re.sub(r"\(\s*\w+\s*:", "(:", q)
    q = re.sub(r"\(\s*\w+\s*\)", "()", q)

    # eliminar variables relaciones
    q = re.sub(r"\[\s*\w+\s*:", "[:", q)
    q = re.sub(r"\[\s*\w+\s*\]", "[]", q)

    q = q.replace("__CONST_NODE__", "(c1)")

    q = re.split(r"\breturn\b", q)[0]
    q = re.sub(r"\s+", " ", q)

    return q.strip()


# =========================================================
# 2. REPRESENTACIÓN ESTRUCTURAL
# =========================================================
def structural_representation(normalized_query: str) -> str:
    q = normalized_query

    if "where" in q:
        parts = q.split("where")
        match_part = parts[0].strip()
        where_part = parts[1].strip()
    else:
        match_part = q.strip()
        where_part = ""

    # nodos
    def replace_node(match):
        content = match.group(1).strip()

        if content == "c1":
            return "(c1)"

        labels = re.findall(r":([a-zA-Z0-9_]+)", content)
        return f"(n{len(labels)})"

    match_part = re.sub(r"\(([^()]*)\)", replace_node, match_part)

    # relaciones
    def replace_rel(match):
        content = match.group(1)
        return "[r1]" if ":" in content else "[r0]"

    match_part = re.sub(r"\[([^\]]*)\]", replace_rel, match_part)

    # WHERE
    if where_part:
        num_conditions = len(re.findall(r"\band\b|\bor\b", where_part)) + 1
        num_ops = len(re.findall(r"=|>|<| in ", where_part))
        where_repr = f"WHERE[cond={num_conditions},ops={num_ops}]"
    else:
        where_repr = "WHERE[none]"

    return f"{match_part} {where_repr}"


# =========================================================
# 3. CARGA
# =========================================================
def load_queries(path):
    queries = []
    with open(path, "r", encoding="utf-8") as f:
        for i, line in enumerate(f, start=1):
            q = line.strip()
            if q:
                queries.append((i, q))
    return queries


# =========================================================
# 4. AGRUPACIÓN
# =========================================================
def group_queries(queries):
    groups = defaultdict(list)

    for line_number, q in queries:
        norm = normalize_query(q)
        structure = structural_representation(norm)
        groups[structure].append((line_number, q, structure))

    return groups


# =========================================================
# 5. SELECCIÓN
#    - mínimo 1 por grupo
#    - resto proporcional
#    - sin límite máximo
# =========================================================
def select_queries(groups, target_size):

    selected = []
    selected_count = defaultdict(int)

    # -------------------------
    # 1️⃣ mínimo 1 por grupo
    # -------------------------
    for structure, group in groups.items():
        choice = random.choice(group)
        selected.append(choice)
        selected_count[structure] += 1

    if len(selected) >= target_size:
        return selected, selected_count

    # -------------------------
    # 2️⃣ relleno proporcional
    # -------------------------
    remaining_slots = target_size - len(selected)
    total_queries = sum(len(g) for g in groups.values())

    for structure, group in groups.items():

        if remaining_slots <= 0:
            break

        proportion = len(group) / total_queries
        extra = int(proportion * remaining_slots)

        available = [x for x in group if x not in selected]
        extra = min(extra, len(available))

        if extra > 0:
            chosen = random.sample(available, extra)
            selected.extend(chosen)
            selected_count[structure] += extra
            remaining_slots = target_size - len(selected)

    # -------------------------
    # 3️⃣ si faltan, completar aleatoriamente
    # -------------------------
    if len(selected) < target_size:
        all_queries = []
        for g in groups.values():
            all_queries.extend(g)

        remaining = [x for x in all_queries if x not in selected]
        random.shuffle(remaining)

        for candidate in remaining:
            if len(selected) >= target_size:
                break
            selected.append(candidate)
            selected_count[candidate[2]] += 1

    return selected, selected_count


# =========================================================
# 6. PIPELINE
# =========================================================
def main(path):

    queries = load_queries(path)
    print("Total queries:", len(queries))

    groups = group_queries(queries)
    print("Unique structures:", len(groups))

    selected, selected_count = select_queries(groups, TARGET_SIZE)

    print("Selected:", len(selected))

    # ============================================
    # ORDENAR SELECCIONADAS POR TIPO (estructura)
    # igual que resumen_grupos (por tamaño grupo)
    # ============================================

    # ordenar estructuras por tamaño descendente
    sorted_structures = sorted(
        groups.items(),
        key=lambda x: len(x[1]),
        reverse=True
    )

    # crear ranking estructura -> posición
    structure_rank = {
        structure: rank
        for rank, (structure, _) in enumerate(sorted_structures)
    }

    # ordenar seleccionadas según ranking
    selected.sort(key=lambda x: structure_rank[x[2]])


    # 1️⃣ solo queries
    with open("gql.ok.1000.cypher", "w", encoding="utf-8") as f:
        for _, q, _ in selected:
            f.write(q + "\n")

    # 2️⃣ con línea y estructura
    with open("gql.ok.1000.lines", "w", encoding="utf-8") as f:
        for line_number, q, structure in selected:
            f.write(f"{line_number}\n")

    # 3️⃣ resumen ordenado por tamaño de grupo
    with open("summary.tsv", "w", encoding="utf-8") as f:
        f.write("structure\ttotal_dataset\tchosen\n")

        sorted_groups = sorted(
            groups.items(),
            key=lambda x: len(x[1]),
            reverse=True
        )

        for structure, group in sorted_groups:
            total_dataset = len(group)
            selected_for_group = selected_count.get(structure, 0)
            f.write(f"{structure}\t{total_dataset}\t{selected_for_group}\n")

    print("Generated files:")
    print("- gql.ok.1000.cypher")
    print("- gql.ok.1000.lines")
    print("- summary.tsv")


# =========================================================
if __name__ == "__main__":
    main("gql.ok.cypher")