MATCH (person1:Person)-[e1:KNOWS]-(mutualFriend:Person)-[e2:KNOWS]-(person2:Person)-[e3:HAS_INTEREST]->(tag:Tag)
WHERE person1 <> person2
RETURN person1, id(e1), mutualFriend, id(e2), person2, id(e3), tag;
