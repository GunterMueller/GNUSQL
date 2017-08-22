
create table tbl1 
(
	k1 int not null primary key,
	t1 char(40) not null unique
)

create table tbl2 
(
	k2 smallint not null primary key,
	t2 int 
)

create table tbl0 
(
	k1 int       default 0 not null  --references tbl1(k1)
	,k2 SMALLINT  not null  --references tbl2(k2)  
	,taila float NOT NULL,
	tailb char(20) default USER,
	PRIMARY KEY (k1,k2)
)

