ALTER SCHEMA AUTHORIZATION INFORMATION_SCHEMA

CREATE VIEW TABLE_PRIVILEGES
       AS ( SELECT
          GRANTOR, GRANTEE, UNTABID, TABAUTH          
       FROM  DEFINITION_SCHEMA.SYSTABAUTH
       WHERE  GRANTEE IN ('PUBLIC', USER )
            OR GRANTOR = USER
       )

GRANT SELECT ON TABLE_PRIVILEGES        TO PUBLIC
