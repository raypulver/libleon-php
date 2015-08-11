<?php
      $payload = array();
      $payload['a'] = 1;
      $payload['b'] = 2;
      $pass = 1;
      $payload = leon_decode(leon_encode($payload));
      if ($payload['a'] !== 1) $pass = 0;
      if ($payload['b'] !== 2) $pass = 0;
      if (!$pass) echo 'fail';
      else echo 'pass';
?>
