#ifndef LEON_CONVERSIONS_H
#define LEON_CONVERSIONS_H

union char_to_unsigned {
  unsigned char byte;
  char signed_byte;
};

union unsigned_short_to_bytes {
  unsigned char bytes[2];
  unsigned short value;
};

union short_to_bytes {
  unsigned char bytes[2];
  short value;
};

union unsigned_int_to_bytes {
  unsigned char bytes[4];
  unsigned int value;
};

union int_to_bytes {
  unsigned char bytes[4];
  int value;
};

union to_unsigned_short {
  unsigned short val;
  unsigned char bytes[2];
};

union to_short {
  short val;
  unsigned char bytes[2];
};

union to_unsigned_int {
  unsigned int val;
  unsigned char bytes[4];
};

union to_int {
  int val;
  unsigned char bytes[4];
};

union to_float {
  float val;
  unsigned char bytes[4];
};

union to_double {
  double val;
  unsigned char bytes[8];
};

#endif
