
CREATE VIEW VIEW1 (i, s) -- updatable view
       AS (SELECT k1, t1
           FROM TBL0
           WHERE k1 > 7)

CREATE VIEW VIEW2 -- (k1, t1) -- updatable view
       AS (SELECT k1, t1
           FROM TBL0
           WHERE k1 < 7)
       WITH CHECK OPTION

CREATE VIEW VIEW3 (v1, v2) -- nonupdatable view
       AS (SELECT DISTINCT TBL0.k1 + 5, t1
           FROM TBL0, TBL1
           WHERE TBL1.k1 > 5
             AND t1 > 'S'
             AND TBL0.k1 = TBL1.k1)

CREATE VIEW VIEW4 (v1, v2, v3) -- nonupdatable view
       AS (SELECT MAX (t1), k1, COUNT (*)
           FROM TBL0
           WHERE k1 > 5
             AND t1 > 'S'
           GROUP BY k1
           HAVING MIN (t1) > 'SY')



