CREATE TABLE mailmessageidentifiers (
    id INTEGER,
    identifier VARCHAR,
    FOREIGN KEY (id) REFERENCES mailmessages(id));
