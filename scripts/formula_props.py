#
# Some functions to calculate properties of the LTL formulae
#

import spot


def number_of_nodes(f):
	"""Number of nodes in a Spot formula"""

	if f.is_tt() or f.is_ff():
		return 0

	return 1 + sum((number_of_nodes(g) for g in f))


def number_of_temporal_ops(f):
	"""Number of temporal operators in a Spot formula"""

	if f.is_leaf():
		return 0

	return (f.kind() not in (spot.op_And, spot.op_Or)) + sum((number_of_temporal_ops(g) for g in f))


def dag_size(f):
	"""Number of nodes in the DAG of the Spot formula"""

	return len(subformulas(f) - {spot.formula.tt(), spot.formula.ff()})


def subformulas(f):
	"""Set of all subformulas of a given Spot formula"""

	seen, pending = {f}, [f]

	while pending:
		g = pending.pop()

		for child in g:
			if child not in seen:
				seen.add(child)
				pending.append(child)

	return seen


def is_G(f):
	"""Whether a formula is a G node"""

	if f.kind() == spot.op_G or f.kind() == spot.op_W and f[1].is_ff():
		return f[0]
	elif f.kind() == spot.op_R and f[0].is_ff():
		return f[1]
	else:
		return None


def is_F(f):
	"""Whether a formula is a F node"""

	if f.kind() == spot.op_F or f.kind() == spot.op_M and f[1].is_tt():
		return f[0]
	elif f.kind() == spot.op_U and f[0].is_tt():
		return f[1]
	else:
		return None


def normalized_props13(f, inside=''):
	"""Check whether a formula is normalized (satisfies properties 1-3)"""

	kind = f.kind()

	if (g := is_G(f)) is not None and (h := is_F(g)) is not None:
		return inside == '' and normalized_props13(h, inside='g')

	elif kind == spot.op_U or kind == spot.op_F or kind == spot.op_M:
		return inside != 'w' and all((normalized_props13(g, inside=inside or 'u') for g in f))

	elif kind == spot.op_W or kind == spot.op_G or kind == spot.op_R:
		return inside != 'g' and all((normalized_props13(g, inside='w') for g in f))

	else:
		return all((normalized_props13(g, inside=inside) for g in f))


def normalized_GF(f):
	"""Check whether a formula is normalized (with GF for Pi_2)"""

	if f.kind() in (spot.op_And, spot.op_Or):
		return all((normalized_GF(g) for g in f))

	return f.is_syntactic_persistence() or \
		((g := is_G(f)) is not None and (h := is_F(g)) is not None
		 and h.is_syntactic_guarantee())


def normalized(f):
	"""Check whether a formula is normalized (in Delta_2)"""

	if f.is_syntactic_persistence() or f.is_syntactic_recurrence():
		return True

	elif f.kind() in (spot.op_And, spot.op_Or):
		return all((normalized(g) for g in f))

	return False
