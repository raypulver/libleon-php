--TEST--
should serialize NaN
--FILE--
<?php
  if (leon_decode(leon_encode(new LEON\NaN())) instanceof LEON\NaN) { echo 'pass'; }
  else { echo 'fail'; }
?>
--EXPECT--
pass
