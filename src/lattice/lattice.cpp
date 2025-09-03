#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include "box.hpp"
#include "lattice.h"
#include "point.hpp"
#include "transform.hpp"

namespace pivot {

#define BOX_INST(n) template struct box<n>;
#define POINT_INST(n) template class point<n>;
#define TRANSFORM_INST(n) template class transform<n>;

BOX_INST(2)
POINT_INST(2)
TRANSFORM_INST(2)

} // namespace pivot
