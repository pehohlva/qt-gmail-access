CREATE TABLE deletedmessages( 
    id INTEGER PRIMARY KEY NOT NULL,
    parentaccountid INTEGER NOT NULL,
    serveruid VARCHAR,
    frommailbox VARCHAR,
    parentfolderid INTEGER,
    FOREIGN KEY (parentaccountid) REFERENCES mailaccounts(id),
    FOREIGN KEY (parentfolderid) REFERENCES mailfolders(id));
