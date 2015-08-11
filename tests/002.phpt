--TEST--
should serialize basic objects
--FILE--
<?php
  use LEON\Channel;
  use LEON\RegExp;
  use LEON\Date;
  use LEON\Undefined;
  class Dummy {
    public $woop = null;
    public $doop = null;
    function __construct() {
      $args = func_get_args();
      $val = $args[0];
      $do = $args[1];
      $this->woop = $val;
      $this->doop = $do;
    }
  }
  $bounce = leon_decode(leon_encode(new Dummy('woop', 'doop')));
  $pass = 1;
  if ($bounce['woop'] != 'woop') {
    $pass = 0;
  }
  if ($bounce['doop'] != 'doop') {
    $pass = 0;
  }
  if (!$pass) print 'fail';
  else print 'pass';
?>
--EXPECT--
pass
