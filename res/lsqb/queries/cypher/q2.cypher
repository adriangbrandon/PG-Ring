MATCH
    (personA:Person)-[e1:KNOWS]-(personB:Person),
    (personA)<-[e2:HAS_CREATOR]-(comment:Comment)-[e3:REPLY_OF]->(post:Post)-[e4:HAS_CREATOR]->(personB)
RETURN personA, id(e1), personB, id(e2), comment, id(e3), post, id(e4);
