CREATE TABLE mailfoldercustom ( 
    id INTEGER NOT NULL,
    name VARCHAR NOT NULL,
    value VARCHAR NOT NULL,
    PRIMARY KEY (id, name),
    FOREIGN KEY (id) REFERENCES mailfolders(id));
