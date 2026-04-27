MATCH (t:Tag)<-[e1:HAS_TAG]-(message:Message)-[e2:HAS_CREATOR]->(creator:Person),
  (message)<-[e3:LIKES]-(liker:Person),
  (message)<-[e4:REPLY_OF]-(comment:Comment)
RETURN t, id(e1), message, id(e2), creator, id(e3), liker, id(e4), comment;
