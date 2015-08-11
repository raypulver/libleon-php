--TEST--
should serialize through a channel
--FILE--
<?php
$payload = array();
$template = array();
$template['c'] = LEON_STRING;
$template['d'] = array();
$template['d'][] = array();
$template['d'][0]['a'] = LEON_CHAR;
$template['d'][0]['b'] = LEON_BOOLEAN;
$channel = new LEON\Channel($template);
$obj = array();
$obj['c'] = "woop";
$obj['d'] = array();
$obj['d'][] = array();
$obj['d'][0]['a'] = 125;
$obj['d'][0]['b'] = TRUE;
$obj['d'][] = array();
$obj['d'][1]['a'] = 124;
$obj['d'][1]['b'] = FALSE;
if ($channel->decode($channel->encode($obj)) != $obj) echo 'fail';
else echo 'pass';
--EXPECT--
pass
