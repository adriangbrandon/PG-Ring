MATCH (c:Country)<-[e1:IS_PART_OF]-(cty:City)<-[e2:IS_LOCATED_IN]-(p:Person)<-[e3:HAS_MEMBER]-(f:Forum)-[e4:CONTAINER_OF]->(pst:Post)<-[e5:REPLY_OF]-(com:Comment)-[e6:HAS_TAG]->(t:Tag)-[e7:HAS_TYPE]->(tc:TagClass)
RETURN c, id(e1), cty, id(e2), p, id(e3), f, id(e4), pst, id(e5), com, id(e6), t, id(e7), tc
LIMIT 1000;
