ALTER TABLE mailaccounts RENAME TO wasmailaccounts;

CREATE TABLE mailaccounts( 
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    type INTEGER NOT NULL,
    name VARCHAR,
    emailaddress VARCHAR,
    status INTEGER,
    signature VARCHAR);

INSERT INTO mailaccounts (id, type, name, emailaddress, status, signature) 
    SELECT id, type, name, emailaddress, status, signature FROM wasmailaccounts;

DROP TABLE wasmailaccounts;
