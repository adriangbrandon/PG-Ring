MATCH (tag1:Tag)<-[e1:Message_hasTag_Tag]-(message:Message)<-[e2:Message_replyOf_Message]-(comment:Message)-[e3:Message_hasTag_Tag]->(tag2:Tag)
WHERE id(tag1) <> id(tag2)
RETURN tag1, id(e1), message, id(e2), comment, id(e3), tag2
LIMIT 1000;
