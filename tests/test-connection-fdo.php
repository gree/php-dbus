#!/usr/bin/php -e
<?php
/**
 * Tests dbus, uses freedesktop standard notification spec
 * http://developer.gnome.org/notification-spec/
 *
 * Usage $0 notify
 * Usage $0 introspect
 */

error_reporting(E_ALL);

/** Arguments for cmdline php 
*/
function arguments($argv) {
    $_ARG = array();
    array_shift($argv); //skip argv[0] !
    foreach ($argv as $arg) {
      if (preg_match('/--([^=]+)=(.*)/',$arg,$reg)) {
        $_ARG[$reg[1]] = $reg[2];
      } elseif(preg_match('/--([^=]+)/',$arg,$reg)){
      	$_ARG[$reg[1]] = true;
      } elseif(preg_match('/^-([a-zA-Z0-9])/',$arg,$reg)) {
            $_ARG[$reg[1]] = true;
      } else {
            $_ARG['input'][]=$arg;
      }
    }
  return $_ARG;
}


$dbus = dbus_bus_get(DBUS_BUS_SESSION);

$m = new DBusMessage(DBUS_MESSAGE_TYPE_METHOD_CALL);
$m->setDestination("org.freedesktop.Notifications");

$cli_args = arguments($argv);
if (empty($cli_args['input'])) {
	$cli_args['input'] = array();
}

switch ($cli_args['input'][0]){
case 'notify':
	$m->setPath("/org/freedesktop/Notifications");
	$m->setInterface("org.freedesktop.Notifications");
	$m->setAutoStart(true);
	$m->setMember("Notify");
	// Application Name
	$m->appendArgs("DBus test app");
	// Replaces ID
	$m->appendArg1(0, 'u');
	// Notification Icon, Summary, Body, Actions
	$m->appendArgs('', 'Summary', 'Text body', array());
	// Hints
	$m->appendArg1(array(), 'a{sv}');
	// Expiration Timeout
	$m->appendArg1(0);
	break;

case 'introspect':
	$m->setPath("/org/freedesktop/Notifications");
	$m->setInterface("org.freedesktop.DBus.Introspectable");
	$m->setAutoStart(true);
	$m->setMember("Introspect");
	break;

default:
	echo "Please specify the type of test\n";
	exit(1);
}

try {
	$r = $dbus->sendWithReplyAndBlock($m);
	echo "b\n";
	$tmp= $r->getArgs();
	echo "c";
} catch (Exception $e){
	echo "Exception:";
	print_r($e);
}
print "d";
print_r($tmp);
