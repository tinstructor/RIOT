#include <stdlib.h>
#include <stdio.h>

#include "olsr.h"
#include "list.h"
#include "debug.h"
#include "util.h"
#include "routing.h"
#include "constants.h"

/* sorted list, only for faster access
 * Keeps yet unroutable nodes, so we don't have to traverse the entire list
 */
struct free_node {
	struct free_node* next;
	struct olsr_node* node;
	uint8_t hops; // for sorting only
};

static struct free_node* _pending_head = 0;
static bool _update_pending = false;

void add_free_node(struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(_pending_head, node, (int (*)(void *, void *)) olsr_node_cmp);
	if (n == NULL) {
		uint8_t hops = node->distance;
		n = simple_list_add_before(&_pending_head, hops);
	}

	if (n == NULL) {
		printf("ERROR: out of memory in %s\n", __FUNCTION__);
		return;
	}

	n->node = node;

	node->next_addr = netaddr_free(node->next_addr);	/* empty next_addr marks route as pending */
	_update_pending = true;
}

bool remove_free_node(struct olsr_node* node) {
	struct free_node* n = simple_list_find_cmp(_pending_head, node, (int (*)(void *, void *)) olsr_node_cmp);
	if (n == NULL)
		return false;
	return simple_list_remove(&_pending_head, n);
}

void fill_routing_table(void) {
	struct free_node* head = _pending_head;

	if (_pending_head == NULL || !_update_pending)
		return;

	_update_pending = false;
	DEBUG("update routing table");

	struct free_node* fn;
	bool noop = false;	/* when in an iteration there was nothing removed from free nodes */
	while (head && !noop) {
		noop = true;	/* if no nodes could be removed in an iteration, abort */
		struct free_node *prev;
		char skipped;
		simple_list_for_each_safe(head, fn, prev, skipped) {
			/* chose shortest route from the set of availiable routes */
			metric_t min_mtrc = METRIC_MAX;
			struct olsr_node* node = NULL; /* chosen route */
			struct nhdp_node* flood_mpr = NULL;
			struct alt_route* route; /* current other_route */
			simple_list_for_each(fn->node->other_routes, route) {

				/* the node is actually a neighbor of ours */
				if (netaddr_cmp(route->last_addr, get_local_addr()) == 0) {
#ifdef ENABLE_HYSTERESIS					
					/* don't use pending nodes */
					if (fn->node->pending)
						continue;
#endif
					min_mtrc = route->link_metric;
					node = fn->node;
					continue;
				}

				/* see if we can find a better route */
				struct olsr_node* _tmp = get_node(route->last_addr);
				if (_tmp == NULL || _tmp->addr == NULL || _tmp->next_addr == NULL)
					continue;

#ifdef ENABLE_HYSTERESIS
				/* ignore pending nodes */
				if (_tmp->distance == 1 && _tmp->pending)
					continue;
#endif
				/* flooding MPR selection */
				if (_tmp->type == NODE_TYPE_NHDP) { // is this reliable?
					if (flood_mpr == NULL) {
						flood_mpr = h1_deriv(_tmp);
						flood_mpr->mpr_neigh_flood++;
					/* flood_neighbors is counted on adding/removing links, no need to +1 */
					} else if (flood_mpr->flood_neighbors < h1_deriv(_tmp)->flood_neighbors) {
						flood_mpr->mpr_neigh_flood--;
						flood_mpr = h1_deriv(_tmp);
						flood_mpr->mpr_neigh_flood++;
					}
				}

				if (_tmp->path_metric + fn->node->link_metric > min_mtrc)
					continue;

				/* try to minimize MPR count */
				if (_tmp->type == NODE_TYPE_NHDP && min_mtrc == _tmp->path_metric + route->link_metric) {
					/* a direct neighbor might be reached over an additional hop, the true MPR */
					if (netaddr_cmp(_tmp->next_addr, _tmp->addr) != 0) {
						struct nhdp_node* old_mpr = h1_deriv(get_node(_tmp->next_addr));
						if (old_mpr->mpr_neigh_route < h1_deriv(_tmp)->mpr_neigh_route + 1)
							continue;
					}

					/* use the neighbor with the most 2-hop neighbors */
					if (h1_deriv(node)->mpr_neigh_route < h1_deriv(_tmp)->mpr_neigh_route + 1)
						continue;
				}

				node = _tmp;
				min_mtrc = _tmp->path_metric + route->link_metric;
			} /* for each other_route */

			if (flood_mpr != NULL)
				fn->node->flood_mpr = h1_super(flood_mpr)->addr;
			else
				fn->node->flood_mpr = NULL;

			/* We found a valid route */
			if (node == fn->node) {
				DEBUG("%s (%s) is a 1-hop neighbor",
					netaddr_to_str_s(&nbuf[0], fn->node->addr), fn->node->name);
				noop = false;
				fn->node->next_addr = netaddr_use(fn->node->addr);
				fn->node->path_metric = fn->node->link_metric;
				fn->node->distance = 1;
				fn->node->lost = 0;

				pop_other_route(fn->node, get_local_addr());
				simple_list_for_each_remove(&head, fn, prev);

			} else if (node != NULL) {
				DEBUG("%s (%s) -> %s (%s) -> […] -> %s",
					netaddr_to_str_s(&nbuf[0], fn->node->addr), fn->node->name,
					netaddr_to_str_s(&nbuf[1], node->addr), node->name,
					netaddr_to_str_s(&nbuf[2], node->next_addr));
				DEBUG("%d = %d", fn->node->distance, node->distance + 1);

				noop = false;

				/* update routing MPR information */
				if (node->type == NODE_TYPE_NHDP) {
					struct nhdp_node* mpr = h1_deriv(get_node(node->next_addr));
					mpr->mpr_neigh_route++;
				}

				fn->node->distance = node->distance + 1;
				fn->node->path_metric = min_mtrc;
				fn->node->next_addr = netaddr_use(node->next_addr);

				pop_other_route(fn->node, node->addr);
				simple_list_for_each_remove(&head, fn, prev);
			} else
				DEBUG("don't yet know how to route %s", netaddr_to_str_s(&nbuf[0], fn->node->addr));
		}
	}

	_pending_head = head;

#ifdef DEBUG
	while (head != NULL) {
		DEBUG("Could not find next hop for %s (%s), should be %s (%d hops)",
			netaddr_to_str_s(&nbuf[0], head->node->addr), head->node->name,
			netaddr_to_str_s(&nbuf[1], head->node->last_addr), head->node->distance);

		head = head->next;
	}
#endif
}
