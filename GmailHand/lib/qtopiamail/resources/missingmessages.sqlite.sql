CREATE TABLE missingmessages (
    id INTEGER,
    identifier VARCHAR,
    level INTEGER,
    PRIMARY KEY (id, identifier),
    FOREIGN KEY (id) REFERENCES mailmessages(id));
