dnl $Id$
dnl config.m4 for extension leon

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(leon, for leon support,
dnl Make sure that the comment is aligned:
dnl [  --with-leon             Include leon support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(leon, whether to enable leon support, [  --enable-leon           Enable leon support])
if test "$PHP_LEON" != "no"; then
  AC_DEFINE(HAVE_LEON, 1, [Whether you have leon])
  PHP_SUBST(LEON_SHARED_LIBADD)
  PHP_NEW_EXTENSION(leon, leon.c encoder.c endianness.c channel.c object_layout_index.c parser.c string_index.c type_check.c hash_array.c classes.c string_buffer.c swizzle.c, $ext_shared,, -funsigned-char -std=c99 -lphp5)
fi
