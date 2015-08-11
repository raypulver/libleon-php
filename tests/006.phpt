--TEST--
should serialize a float
--FILE--
<?php
$payload = -0.5;
if (leon_decode(leon_encode($payload)) !== $payload) echo 'fail';
else echo 'pass';
?>
--EXPECT--
pass
