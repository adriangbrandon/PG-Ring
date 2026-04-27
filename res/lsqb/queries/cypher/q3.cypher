MATCH (country:Country)
MATCH (pA:Person)-[e1:IS_LOCATED_IN]->(cityA:City)-[e2:IS_PART_OF]->(country)
MATCH (pB:Person)-[e3:IS_LOCATED_IN]->(cityB:City)-[e4:IS_PART_OF]->(country)
MATCH (pC:Person)-[e5:IS_LOCATED_IN]->(cityC:City)-[e6:IS_PART_OF]->(country)
MATCH (pA)-[k1:KNOWS]-(pB)-[k2:KNOWS]-(pC)-[k3:KNOWS]-(pA)
RETURN country, pA, id(e1), cityA, id(e2), pB, id(e3), cityB, id(e4), pC, id(e5), cityC, id(e6), id(k1), id(k2), id(k3);
