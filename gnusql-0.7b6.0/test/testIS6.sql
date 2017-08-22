ALTER SCHEMA AUTHORIZATION INFORMATION_SCHEMA

CREATE VIEW CHECK_CONSTRAINTS
       AS ( SELECT
             UNTABID, NCOLS
       FROM  DEFINITION_SCHEMA.SYSCHCONSTR
       )


GRANT SELECT ON CHECK_CONSTRAINTS       TO PUBLIC