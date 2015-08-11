--TEST--
should serialize a regexp
--FILE--
<?php
      $regexp = new LEON\RegExp('^$');
      if (leon_decode(leon_encode($regexp))->pattern != '^$') {
        echo 'fail';
      } else {
        echo 'pass';
      }
?>
--EXPECT--
pass
