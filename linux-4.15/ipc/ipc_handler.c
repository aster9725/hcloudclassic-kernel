#ifndef NO_IPC
#include <linux/ipc.h>
#include <linux/ipc_namespace.h>
#include <linux/nsproxy.h>
#include <linux/msg.h>

struct ipc_namespace *find_get_hcc_ipcns(void)
{
	struct hcc_namespace *hcc_ns;
	struct ipc_namespace *ipc_ns;

	hcc_ns = find_get_hcc_ns();
	if (!hcc_ns)
		goto error;

	if (!hcc_ns->root_nsproxy.ipc_ns)
		goto error_ipcns;

	ipc_ns = get_ipc_ns(hcc_ns->root_nsproxy.ipc_ns);

	put_hcc_ns(hcc_ns);

	return ipc_ns;

error_ipcns:
	put_hcc_ns(hcc_ns);
error:
	return NULL;
}


int hcc_ipc_get_maxid(struct ipc_ids* ids)
{
	ipcmap_object_t *ipc_map;
	int max_id;

	ipc_map = _get_object(ids->hccops->map_set, 0);
	max_id = ipc_map->alloc_map - 1;
	_put_object(ids->hccops->map_set, 0);

	return max_id;
}

int hcc_ipc_get_new_id(struct ipc_ids* ids)
{
	ipcmap_object_t *ipc_map, *max_id;
	int i = 1, id = -1, offset;

	max_id = _grab_object(ids->hccops->map_set, 0);

	while (id == -1) {
		ipc_map = _grab_object(ids->hccops->map_set, i);

		if (ipc_map->alloc_map != ULONG_MAX) {
			offset = find_first_zero_bit(&ipc_map->alloc_map,
						     BITS_PER_LONG);

			if (offset < BITS_PER_LONG) {

				id = (i-1) * BITS_PER_LONG + offset;
				set_bit(offset, &ipc_map->alloc_map);
				if (id >= max_id->alloc_map)
					max_id->alloc_map = id + 1;
			}
		}

		_put_object(ids->hccops->map_set, i);
		i++;
	}

	_put_object(ids->hccops->map_set, 0);

	return id;
}


int hcc_ipc_get_this_id(struct ipc_ids *ids, int id)
{
	ipcmap_object_t *ipc_map, *max_id;
	int i, offset, ret = 0;

	max_id = _grab_object(ids->hccops->map_set, 0);

	offset = id % BITS_PER_LONG;
	i = (id - offset)/BITS_PER_LONG +1;

	ipc_map = _grab_object(ids->hccops->map_set, i);

	if (test_and_set_bit(offset, &ipc_map->alloc_map)) {
		ret = -EBUSY;
		goto out_id_unavailable;
	}

	if (id >= max_id->alloc_map)
		max_id->alloc_map = id + 1;

out_id_unavailable:
	_put_object(ids->hccops->map_set, i);
	_put_object(ids->hccops->map_set, 0);

	return ret;
}

void hcc_ipc_rmid(struct ipc_ids* ids, int index)
{
	ipcmap_object_t *ipc_map, *max_id;
	int i, offset;


	i = 1 + index / BITS_PER_LONG;
	offset = index % BITS_PER_LONG;

	ipc_map = _grab_object(ids->hccops->map_set, i);

	BUG_ON(!test_bit(offset, &ipc_map->alloc_map));

	clear_bit(offset, &ipc_map->alloc_map);

	_put_object(ids->hccops->map_set, i);

	/* Check if max_id must be adjusted */

	max_id = _grab_object(ids->hccops->map_set, 0);

	if (max_id->alloc_map != index + 1)
		goto done;

	for (; i > 0; i--) {

		ipc_map = _grab_object(ids->hccops->map_set, i);
		if (ipc_map->alloc_map != 0) {
			for (; offset >= 0; offset--) {
				if (test_bit (offset, &ipc_map->alloc_map)) {
					max_id->alloc_map = 1 + offset +
						(i - 1) * BITS_PER_LONG;
					_put_object(
						ids->hccops->map_set, i);
					goto done;
				}
			}
		}
		offset = 31;
		_put_object(ids->hccops->map_set, i);
	}

	max_id->alloc_map = 0;
done:
	_put_object(ids->hccops->map_set, 0);

	return;
}


#endif