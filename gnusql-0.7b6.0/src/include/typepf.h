i4_t tmppagerown __P((i4_t cnt, sql_type_t *type));
i4_t tabrown  __P((Tabid *tabid));
i4_t pagerown __P((Tabid *tabid));
i4_t colrowval __P((Tabid *tabid,i4_t col));
i4_t depthind __P((Indid *indid));
i4_t depthspind __P((Tabid *tabid));
i4_t leathind __P((Indid *indid));
i4_t estsort __P((Tabid *tabid, i4_t rowcnt,i4_t clmcnt,sql_type_t *list));

/*
i4_t indsel __P((struct ClmInfo *clm));
i4_t estwait __P((Tabid *tabid,i4_t clmcnt,i4_t rowcnt, struct ClmInfo *list));
i4_t selcond __P((Tabid *tabid,i4_t cnt,struct ClmInfo *list));
*/





