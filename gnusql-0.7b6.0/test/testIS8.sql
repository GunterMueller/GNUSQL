ALTER SCHEMA AUTHORIZATION INFORMATION_SCHEMA

CREATE VIEW VIEW_COLUMN_USAGE
       AS ( SELECT
             COLNAME
       FROM  DEFINITION_SCHEMA.SYSCOLUMNS
       WHERE UNTABID 
             IN
           ( SELECT UNTABID
             FROM  DEFINITION_SCHEMA.SYSTABLES
             WHERE TABTYPE = 'V'
                   AND
                   OWNER = USER
           )
      ) 


GRANT SELECT ON VIEW_COLUMN_USAGE       TO PUBLIC

