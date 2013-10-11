ALTER TABLE mailmessageidentifiers RENAME TO wasmailmessageidentifiers;

CREATE TABLE mailmessageidentifiers( 
    id INTEGER,
    identifier VARCHAR,
    FOREIGN KEY (id) REFERENCES mailmessages(id));

INSERT INTO mailmessageidentifiers (id, identifier)
    SELECT id, identifier FROM wasmailmessageidentifiers;

DROP TABLE wasmailmessageidentifiers;
