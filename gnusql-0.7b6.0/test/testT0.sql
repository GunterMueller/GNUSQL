CREATE TABLE TBL0 
(
	k1 int            NOT NULL,
	t1 char(40)       DEFAULT 'abba'  CHECK (t1 > 'B'),
	UNIQUE (k1),
        CHECK (k1 > 0 AND t1 < 'Z')
)


CREATE TABLE TBL2 
(
	k2 smallint      NOT NULL PRIMARY KEY,
	t2 int 
)

CREATE TABLE TBL1 
(
	 k1 int             DEFAULT 5  CHECK (k1 > 0) NOT NULL
				REFERENCES TBL0 (k1)
	,k2 SMALLINT        DEFAULT 3 NOT NULL
	,taila real
	,tailb char(20)     DEFAULT USER  NOT NULL
	,UNIQUE (k1,tailb)
	,FOREIGN KEY (k2)   REFERENCES TBL2
)
 


GRANT SELECT, UPDATE (k1, t1)  ON TBL0
      TO vera
 
