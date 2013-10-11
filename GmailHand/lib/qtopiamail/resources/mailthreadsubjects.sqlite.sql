CREATE TABLE mailthreadsubjects (
    threadid INTEGER,
    subjectid INTEGER,
    FOREIGN KEY (threadid) REFERENCES mailthreads(id),
    FOREIGN KEY (subjectid) REFERENCES mailsubjects(id));
