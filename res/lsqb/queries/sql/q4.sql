SELECT Message_hasTag_Tag.TagId, Message_hasTag_Tag.MessageId, Message_hasCreator_Person.hasCreator_PersonId,
Person_likes_Message.PersonId, Comment_replyOf_Message.CommentId
FROM Message_hasTag_Tag
JOIN Message_hasCreator_Person
  ON Message_hasTag_Tag.MessageId = Message_hasCreator_Person.MessageId
JOIN Comment_replyOf_Message 
  ON Comment_replyOf_Message.ParentMessageId = Message_hasTag_Tag.MessageId
JOIN Person_likes_Message
  ON Person_likes_Message.MessageId = Message_hasTag_Tag.MessageId
LIMIT 1000;
