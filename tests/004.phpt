--TEST--
should serialize signed numbers
--FILE--
<?php
  if (leon_decode(leon_encode(-500)) !== -500) echo 'fail';
  else echo 'pass';
?>
--EXPECT--
pass
