/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "gsqltrn.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

result_t *
init_comp_1(init_params_t *argp, gss_client_t *clnt)
{
	static result_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, INIT_COMP,
		(xdrproc_t) xdr_init_params_t, (caddr_t) argp,
		(xdrproc_t) xdr_result_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

result_t *
compile_1(stmt_info_t *argp, gss_client_t *clnt)
{
	static result_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, COMPILE,
		(xdrproc_t) xdr_stmt_info_t, (caddr_t) argp,
		(xdrproc_t) xdr_result_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

result_t *
del_segment_1(seg_del_t *argp, gss_client_t *clnt)
{
	static result_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, DEL_SEGMENT,
		(xdrproc_t) xdr_seg_del_t, (caddr_t) argp,
		(xdrproc_t) xdr_result_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

result_t *
link_cursor_1(link_cursor_t *argp, gss_client_t *clnt)
{
	static result_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, LINK_CURSOR,
		(xdrproc_t) xdr_link_cursor_t, (caddr_t) argp,
		(xdrproc_t) xdr_result_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

result_t *
load_module_1(string_t *argp, gss_client_t *clnt)
{
	static result_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, LOAD_MODULE,
		(xdrproc_t) xdr_string_t, (caddr_t) argp,
		(xdrproc_t) xdr_result_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

result_t *
execute_stmt_1(insn_t *argp, gss_client_t *clnt)
{
	static result_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, EXECUTE_STMT,
		(xdrproc_t) xdr_insn_t, (caddr_t) argp,
		(xdrproc_t) xdr_result_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

result_t *
db_create_1(void *argp, gss_client_t *clnt)
{
	static result_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, DB_CREATE,
		(xdrproc_t) xdr_void, (caddr_t) argp,
		(xdrproc_t) xdr_result_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

result_t *
retry_1(void *argp, gss_client_t *clnt)
{
	static result_t clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, RETRY,
		(xdrproc_t) xdr_void, (caddr_t) argp,
		(xdrproc_t) xdr_result_t, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

int *
is_rpc_ready_1(void *argp, gss_client_t *clnt)
{
	static int clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (gss_client_call(clnt, IS_RPC_READY,
		(xdrproc_t) xdr_void, (caddr_t) argp,
		(xdrproc_t) xdr_int, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
