MATCH (c:Country)<-[e1:City_isPartOf_Country]-(cty:City)<-[e2:Person_isLocatedIn_City]-(p:Person)<-[e3:Forum_hasMember_Person]-(f:Forum)-[e4:Forum_containerOf_Message]->(m1:Message)<-[e5:Message_replyOf_Message]-(m2:Message)-[e6:Message_hasTag_Tag]->(t:Tag)-[e7:Tag_hasType_TagClass]->(tc:TagClass)
RETURN c, id(e1), cty, id(e2), p, id(e3), f, id(e4), m1, id(e5), m2, id(e6), t, id(e7), tc
LIMIT 1000;
