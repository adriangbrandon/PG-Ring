MATCH (t:Tag)<-[e1:Message_hasTag_Tag]-(message:Message)-[e2:Message_hasCreator_Person]->(creator:Person),
  (message)<-[e3:Person_likes_Message]-(liker:Person),
  (message)<-[e4:Message_replyOf_Message]-(comment:Message)
RETURN t, id(e1), message, id(e2), creator, id(e3), liker, id(e4), comment 
LIMIT 1000;
