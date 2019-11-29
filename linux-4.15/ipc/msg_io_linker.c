#include <linux/shm.h>
#include <linux/lockdep.h>
#include <linux/security.h>
#include <linux/ipc_namespace.h>
#include <linux/ipc.h>
#include <net/hccrpc/rpc.h>

#include "ipc_handler.h"
#include "util.h"


struct kmem_cache *msq_object_cachep;

static struct msg_queue *create_local_msq(struct ipc_namespace *ns,
					  struct msg_queue *received_msq)
{
	struct msg_queue *msq;
	int retval;

	msq = ipc_rcu_alloc(sizeof(*msq));
	if (!msq)
		return ERR_PTR(-ENOMEM);

	*msq = *received_msq;
	retval = security_msg_queue_alloc(msq);
	if (retval)
		goto err_putref;

	/*
	 * ipc_reserveid() locks msq
	 */
	retval = local_ipc_reserveid(&msg_ids(ns), &msq->q_perm, ns->msg_ctlmni);
	if (retval)
		goto err_security_free;

	msq->is_master = 0;
	INIT_LIST_HEAD(&msq->q_messages);
	INIT_LIST_HEAD(&msq->q_receivers);
	INIT_LIST_HEAD(&msq->q_senders);

	msq->q_perm.hccops = msg_ids(ns).hccops;
	local_msg_unlock(msq);

	return msq;

err_security_free:
	security_msg_queue_free(msq);
err_putref:
	ipc_rcu_putref(msq);
	return ERR_PTR(retval);
}

static void delete_local_msq(struct ipc_namespace *ns, struct msg_queue *local_msq)
{
	struct msg_queue *msq;

	msq = local_msq;

	security_msg_queue_free(msq);

	ipc_rmid(&msg_ids(ns), &msq->q_perm);

	local_msg_unlock(msq);

	ipc_rcu_putref(msq);
}
static void update_local_msq (struct msg_queue *local_msq,
			      struct msg_queue *received_msq)
{
	/* local_msq->q_perm = received_msq->q_perm;*/
	local_msq->q_stime = received_msq->q_stime;
	local_msq->q_rtime = received_msq->q_rtime;
	local_msq->q_ctime = received_msq->q_ctime;
	local_msq->q_cbytes = received_msq->q_cbytes;
	local_msq->q_qnum = received_msq->q_qnum;
	local_msq->q_qbytes = received_msq->q_qbytes;
	local_msq->q_lspid = received_msq->q_lspid;
	local_msq->q_lrpid = received_msq->q_lrpid;

	/* Do not modify the list_head else you will loose
	   information on master node */
}


int msq_alloc_object (struct master_obj * obj_entry,
		      struct master_set * set,
		      objid_t objid)
{
	msq_object_t *msq_object;

	msq_object = kmem_cache_alloc(msq_object_cachep, GFP_KERNEL);
	if (!msq_object)
		return -ENOMEM;

	msq_object->local_msq = NULL;
	obj_entry->object = msq_object;

	return 0;
}

