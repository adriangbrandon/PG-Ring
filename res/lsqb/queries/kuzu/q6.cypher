MATCH (person1:Person)-[e1:Person_knows_Person]->(person2:Person)-[e2:Person_knows_Person]->(person3:Person)-[e3:Person_hasInterest_Tag]->(tag:Tag)
WHERE id(person1) <> id(person3)
RETURN person1, id(e1), person2, id(e2), person3, id(e3), tag
LIMIT 1000;
