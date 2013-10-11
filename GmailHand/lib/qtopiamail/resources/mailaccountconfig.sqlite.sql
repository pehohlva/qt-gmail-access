CREATE TABLE mailaccountconfig ( 
    id INTEGER NOT NULL,
    service VARCHAR NOT NULL,
    name VARCHAR NOT NULL,
    value VARCHAR NOT NULL,
    PRIMARY KEY (id, service, name),
    FOREIGN KEY (id) REFERENCES mailaccounts(id));
