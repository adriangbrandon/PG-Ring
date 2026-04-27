MATCH
  (person1:Person)-[e1:Person_knows_Person]->(person2:Person),
  (person1)<-[e2:Message_hasCreator_Person]-(comment:Message)-[e3:Comment_replyOf_Post]->(Post:Message)-[e4:Message_hasCreator_Person]->(person2)
RETURN person1, id(e1), person2, id(e2), comment, id(e3), Post, id(e4)
LIMIT 1000;
