--TEST--
should serialize a double
--FILE--
<?php
      $payload = -232.222;
      $channel = new LEON\Channel(LEON_DOUBLE);
      if ($payload !== $channel->decode($channel->encode($payload))) {
        echo 'fail';
      } else {
        echo 'pass';
      }
?>
--EXPECT--
pass
