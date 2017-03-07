ALTER SCHEMA AUTHORIZATION INFORMATION_SCHEMA

CREATE VIEW SCHEMATA  ( SCHEMA_OWNER )
       AS (SELECT
          OWNER
          FROM DEFINITION_SCHEMA.SYSTABLES
          WHERE OWNER = USER )
          
GRANT SELECT ON SCHEMATA                TO PUBLIC

