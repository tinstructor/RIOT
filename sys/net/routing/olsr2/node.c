#include <stdio.h>

#include "node.h"
#include "list.h"
#include "debug.h"

#include "common/netaddr.h"

static struct netaddr_rc local_addr;
static struct avl_tree olsr_head;

#ifdef ENABLE_NAME
char* local_name;
#endif

static void _decrease_mpr_neigh(struct olsr_node* node) {
	struct nhdp_node* n1 = h1_deriv(get_node(node->last_addr));

	/* update MPR information */
	if (h1_super(n1)->type == NODE_TYPE_NHDP) {

		if (n1 != NULL && n1->mpr_neigh_route > 0)
			n1->mpr_neigh_route--;

		struct nhdp_node* n1_f = h1_deriv(get_node(node->flood_mpr));

		if (n1_f != NULL && n1_f->mpr_neigh_flood > 0)
			n1->mpr_neigh_flood--;
	}
}

static int _addr_cmp(const void* a, const void* b) {
	return memcmp(a, b, NETADDR_MAX_LENGTH);
}

int olsr_node_cmp(struct olsr_node* a, struct olsr_node* b) {
	return netaddr_cmp(a->addr, b->addr);
}

void node_init(void) {
	local_addr._refs = 1;
	avl_init(get_olsr_head(), _addr_cmp, false);
}

struct netaddr* get_local_addr() {
	return (struct netaddr*) &local_addr;
}

struct avl_tree* get_olsr_head(void) {
	return &olsr_head;
}

struct olsr_node* get_node(struct netaddr* addr) {
	struct olsr_node *n; // for typeof
	if (addr == NULL)
		return NULL;
	return avl_find_element(get_olsr_head(), addr, n, node);
}

void add_other_route(struct olsr_node* node, struct netaddr* last_addr, uint8_t distance, metric_t metric, uint8_t vtime) {
	/* make sure the route is not already the default route */
	if (node->last_addr != NULL && netaddr_cmp(node->last_addr, last_addr) == 0) {
		node->expires = time_now() + vtime;
		return;
	}

	struct alt_route* route = simple_list_find_memcmp(node->other_routes, last_addr);
	if (route != NULL) {
		route->expires = time_now() + vtime;
		route->link_metric = metric;
		return;
	}

	route = simple_list_add_head(&node->other_routes);

	if (route == NULL) {
		printf("ERROR: out of memory in %s\n", __FUNCTION__);
		return;
	}

	route->last_addr = netaddr_reuse(last_addr);
	route->expires = time_now() + vtime;
	route->link_metric = metric;

	/* if we add a route for the first time, increment flood_neighbors */
	if (distance == 2) {
		struct nhdp_node* n1 = h1_deriv(get_node(last_addr));
		if (n1 != NULL)
			n1->flood_neighbors++;
	}

}

void remove_default_node(struct olsr_node* node) {
	if (node->last_addr) {
		_decrease_mpr_neigh(node);

		struct nhdp_node* mpr = h1_deriv(get_node(node->last_addr));
		if (mpr != NULL)
			mpr->flood_neighbors--;

		node->last_addr = netaddr_free(node->last_addr);
	}

	node->next_addr = netaddr_free(node->next_addr);
}

/*
 * moves the default route of node to other_routes
 */
void push_default_route(struct olsr_node* node) {
	struct netaddr* last_addr = node->last_addr;

	if (node->last_addr == NULL)
		return;

	_decrease_mpr_neigh(node);
	struct alt_route* route = simple_list_find_memcmp(node->other_routes, last_addr);

	/* don't add route if it already exists - this should never happen, right? */
	if (route != NULL) {
		node->last_addr = netaddr_free(node->last_addr);
		return;
	}

	route = simple_list_add_head(&node->other_routes);

	if (route == NULL) {
		printf("ERROR: out of memory in %s\n", __FUNCTION__);
		return;
	}

	route->expires = node->expires;
	route->last_addr = node->last_addr;
	route->link_metric = node->link_metric;
	node->last_addr = NULL;
}

void pop_other_route(struct olsr_node* node, struct netaddr* last_addr) {
	char skipped;
	struct alt_route *route, *prev;
	simple_list_for_each_safe(node->other_routes, route, prev, skipped) {
		if (netaddr_cmp(route->last_addr, last_addr))
			continue;

		node->last_addr = route->last_addr;
		node->expires = route->expires;
		node->link_metric = route->link_metric;
		simple_list_for_each_remove(&node->other_routes, route, prev);
		break;
	}
}

void remove_other_route(struct olsr_node* node, struct netaddr* last_addr) {
	char skipped;
	struct alt_route *route, *prev;
	simple_list_for_each_safe(node->other_routes, route, prev, skipped) {
		if (netaddr_cmp(route->last_addr, last_addr))
			continue;

		struct nhdp_node* mpr = h1_deriv(get_node(route->last_addr));
		if (mpr != NULL)
			mpr->flood_neighbors--;

		netaddr_free(route->last_addr);
		simple_list_for_each_remove(&node->other_routes, route, prev);
		break;
	}
}
