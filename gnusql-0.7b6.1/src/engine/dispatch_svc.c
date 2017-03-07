/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <rpc/pmap_clnt.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif
#define main adm_rpc_start
#define RPC_SVC_FG

static void
sql_disp_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	union {
		init_arg create_transaction_1_arg;
		opq is_ready_1_arg;
		int kill_all_1_arg;
		opq trn_kill_1_arg;
		int disp_finit_1_arg;
		opq copy_lj_1_arg;
		int change_params_1_arg;
	} argument;
	char *result;
	xdrproc_t _xdr_argument, _xdr_result;
	char *(*local)(char *, struct svc_req *);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply (transp, (xdrproc_t) xdr_void, (char *)NULL);
		return;

	case CREATE_TRANSACTION:
		_xdr_argument = (xdrproc_t) xdr_init_arg;
		_xdr_result = (xdrproc_t) xdr_res;
		local = (char *(*)(char *, struct svc_req *)) create_transaction_1_svc;
		break;

	case IS_READY:
		_xdr_argument = (xdrproc_t) xdr_opq;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) is_ready_1_svc;
		break;

	case KILL_ALL:
		_xdr_argument = (xdrproc_t) xdr_int;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) kill_all_1_svc;
		break;

	case TRN_KILL:
		_xdr_argument = (xdrproc_t) xdr_opq;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) trn_kill_1_svc;
		break;

	case DISP_FINIT:
		_xdr_argument = (xdrproc_t) xdr_int;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) disp_finit_1_svc;
		break;

	case COPY_LJ:
		_xdr_argument = (xdrproc_t) xdr_opq;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) copy_lj_1_svc;
		break;

	case CHANGE_PARAMS:
		_xdr_argument = (xdrproc_t) xdr_int;
		_xdr_result = (xdrproc_t) xdr_int;
		local = (char *(*)(char *, struct svc_req *)) change_params_1_svc;
		break;

	default:
		svcerr_noproc (transp);
		return;
	}
	memset ((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		svcerr_decode (transp);
		return;
	}
	result = (*local)((char *)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) _xdr_result, result)) {
		svcerr_systemerr (transp);
	}
	if (!svc_freeargs (transp, (xdrproc_t) _xdr_argument, (caddr_t) &argument)) {
		fprintf (stderr, "%s", "unable to free arguments");
		exit (1);
	}
	return;
}

int
main (int argc, char **argv)
{
	register SVCXPRT *transp;

	pmap_unset (SQL_DISP, SQL_DISP_ONE);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create udp service.");
		exit(1);
	}
	if (!svc_register(transp, SQL_DISP, SQL_DISP_ONE, sql_disp_1, IPPROTO_UDP)) {
		fprintf (stderr, "%s", "unable to register (SQL_DISP, SQL_DISP_ONE, udp).");
		exit(1);
	}

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL) {
		fprintf (stderr, "%s", "cannot create tcp service.");
		exit(1);
	}
	if (!svc_register(transp, SQL_DISP, SQL_DISP_ONE, sql_disp_1, IPPROTO_TCP)) {
		fprintf (stderr, "%s", "unable to register (SQL_DISP, SQL_DISP_ONE, tcp).");
		exit(1);
	}

	svc_run ();
	fprintf (stderr, "%s", "svc_run returned");
	exit (1);
	/* NOTREACHED */
}