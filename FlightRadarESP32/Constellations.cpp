#include "Constellations.h"

static const StarPoint CONST_ORION[] = {
  { -18, -18, true }, { 18, -20, true }, { -8, -2, false },
  { 0, 0, true }, { 8, 2, false }, { -15, 20, true }, { 16, 18, true }
};

static const StarPoint CONST_DIPPER[] = {
  { -24, 8, true }, { -12, 1, true }, { 0, 3, true },
  { 11, 11, true }, { 21, 2, true }, { 13, -11, true }, { 2, -9, true }
};

static const StarPoint CONST_CASS[] = {
  { -24, 4, true }, { -12, -10, true }, { 0, 0, true },
  { 13, -13, true }, { 25, 1, true }
};

static const StarPoint CONST_CYGNUS[] = {
  { 0, -28, true }, { 0, -12, false }, { 0, 4, true },
  { 0, 22, true }, { -19, 1, true }, { 20, 1, true }
};

const ConstellationPattern CONSTELLATIONS[] = {
  { "ORION",  CONST_ORION,  sizeof(CONST_ORION)  / sizeof(CONST_ORION[0])  },
  { "DIPPER", CONST_DIPPER, sizeof(CONST_DIPPER) / sizeof(CONST_DIPPER[0]) },
  { "CASS",   CONST_CASS,   sizeof(CONST_CASS)   / sizeof(CONST_CASS[0])   },
  { "CYGNUS", CONST_CYGNUS, sizeof(CONST_CYGNUS) / sizeof(CONST_CYGNUS[0]) }
};

const int CONSTELLATION_COUNT = sizeof(CONSTELLATIONS) / sizeof(CONSTELLATIONS[0]);

const int CONST_POSITIONS[][2] = {
  { 58, 68 }, { 176, 72 }, { 62, 166 }, { 178, 158 }, { 120, 70 }, { 120, 176 }
};

const int CONST_POSITION_COUNT = sizeof(CONST_POSITIONS) / sizeof(CONST_POSITIONS[0]);
