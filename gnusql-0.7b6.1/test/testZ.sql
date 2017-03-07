
CREATE TABLE ZTBL0 
(
	 k1 int            --NOT NULL
	,t1 char(40)       DEFAULT 'abba'
	,UNIQUE (k1)
)


CREATE TABLE ZTBL1 
(
	k1 int           DEFAULT 5 NOT NULL 
	,k2 SMALLINT     DEFAULT 3 NOT NULL
	,taila real
	,tailb char(20)  DEFAULT USER  NOT NULL
	,UNIQUE (k1,tailb)
)
 

CREATE TABLE ZTBL2 
(
	k2 smallint      NOT NULL PRIMARY KEY,
	t2 int 
)

--GRANT SELECT, UPDATE (TABNAME, UNTABID) ON ZTBL10
--      TO PUBLIC
--WITH GRANT OPTION

GRANT SELECT, UPDATE (k1, k2)  ON ZTBL1
    TO kml, voin





