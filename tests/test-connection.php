#!/usr/bin/php -e
<?php

error_reporting(E_ALL);

$dbus = dbus_bus_get(DBUS_BUS_SESSION);

$m = new DBusMessage(DBUS_MESSAGE_TYPE_METHOD_CALL);
$m->setDestination("org.kde.VisualNotifications");
/*
$m->setPath("/VisualNotifications");
$m->setInterface("org.kde.VisualNotifications");
$m->setAutoStart(true);
$m->setMember("Notify");
$m->appendArgs("DBus test app",0,0,0,"Summary","Text body",array(),array(),0);
*/

$m->setPath("/VisualNotifications");
$m->setInterface("org.freedesktop.DBus.Introspectable");
$m->setAutoStart(true);
$m->setMember("Introspect");

echo "a\n";
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
?>
