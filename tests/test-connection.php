#!/usr/bin/php -e
<?php

error_reporting(E_ALL);

$dbus = dbus_bus_get(DBUS_BUS_SESSION);

$m = new DBusMessage(DBUS_MESSAGE_TYPE_METHOD_CALL);
echo "1";
$m->setDestination("org.kde.VisualNotifications");
echo "2";
$m->setPath("/VisualNotifications");
$m->setInterface("org.kde.VisualNotifications");
$m->setMember("Notify");
$m->setAutoStart(true);
$m->appendArgs("DBus test app",0,0,Null,"Summary","Text body",array(),array(),0);

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
print_r($dbus);
print_r($tmp);
?>
