/**
 * @file tfspot.hh
 *
 * Convert to and from Spot formulae.
 */

#ifndef TFSPOT_HH
#define TFSPOT_HH

#include <spot/tl/formula.hh>

#include "tree.hh"

/**
 * Convert a syntax tree into a Spot formula.
 */
spot::formula to_spot(Node* tree);

/**
 * Convert a Spot formula into a syntax tree.
 */
Node* from_spot(const spot::formula& form);

#endif // TFSPOT_HH
