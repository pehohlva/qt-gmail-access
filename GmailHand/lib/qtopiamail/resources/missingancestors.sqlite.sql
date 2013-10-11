CREATE TABLE missingancestors (
    messageid INTEGER PRIMARY KEY,
    subjectid INTEGER,
    state INTEGER DEFAULT 0,
    FOREIGN KEY (messageid) REFERENCES mailmessages(id),
    FOREIGN KEY (subjectid) REFERENCES mailsubjects(id));
