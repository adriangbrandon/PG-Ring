MATCH (country:Country)
MATCH (person1:Person)-[e1:Person_isLocatedIn_City]->(city1:City)-[e2:City_isPartOf_Country]->(country)
MATCH (person2:Person)-[e3:Person_isLocatedIn_City]->(city2:City)-[e4:City_isPartOf_Country]->(country)
MATCH (person3:Person)-[e5:Person_isLocatedIn_City]->(city3:City)-[e6:City_isPartOf_Country]->(country)
MATCH (person1)-[e7:Person_knows_Person]->(person2)-[e8:Person_knows_Person]->(person3)-[e9:Person_knows_Person]->(person1)
RETURN country, person1, id(e1), city1, id(e2), person2, id(e3), city2, id(e4), person3, id(e5), city3, id(e6), id(e7), id(e8), id(e9)
LIMIT 1000;
