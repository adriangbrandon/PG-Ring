MATCH (tag1:Tag)<-[e1:HAS_TAG]-(message:Message)<-[e2:REPLY_OF]-(comment:Comment)-[e3:HAS_TAG]->(tag2:Tag)
WHERE tag1 <> tag2
RETURN tag1, id(e1), message, id(e2), comment, id(e3), tag2;
