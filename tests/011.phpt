--TEST--
should serialize undefined
--FILE--
<?php
if (leon_decode(leon_encode(new LEON\Undefined())) instanceof LEON\Undefined) echo 'pass';
else echo 'fail';
?>
--EXPECT--
pass
