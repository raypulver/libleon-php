<?php
$time = 1437149199000;
$date = new LEON\Date($time);
if (leon_decode(leon_encode($date))->timestamp != $time) echo 'fail';
else echo 'pass';
?>
