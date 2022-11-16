<?php
require_once 'config.php';

//echo var_dump(
//    array(
//    "connport" => $CL_CONNECTIONLESS_PORT,
//    "Q2SERVER_IP" => $Q2SERVER_IP,
//    "Q2SERVER_PORT", $Q2SERVER_PORT,
//    )
//);

$sck = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
socket_bind($sck, "0.0.0.0", $CL_CONNECTIONLESS_PORT);

// 1 sec timeout
socket_set_option($sck, SOL_SOCKET, SO_RCVTIMEO, array("sec" => 3, "usec" => 0));

$s = "\xff\xff\xff\xffstatus";
socket_sendto($sck, $s, 10, 0, $Q2SERVER_IP, $Q2SERVER_PORT);

$recv_data = "";
$received = socket_recvfrom(
    $sck,
    $recv_data,
    1500,
    0,
    $Q2SERVER_IP,
    $Q2SERVER_PORT
);

if (!$received)
    die (json_encode(array("status" => "offline")));

// echo $recv_data;

$lines = preg_split("/\n/", $recv_data);
$keys = preg_split("/\\\\/", $lines[1]);

$cvars = array();
for ($i = 1; $i < count($keys); $i += 2) {
    $cvars[] = array($keys[$i] => $keys[$i + 1]);
}

$players = array();

for ($i = 2; $i < count($lines) - 1; $i++) {
    $psplit = explode(" ", $lines[$i]);
    $score = $psplit[0];
    $ping = $psplit[1];
    $name = join("", array_slice($psplit, 2));

    $name = trim($name, " \t\n\r\0\x0B\"");

    $players[] = array(
        "score" => $score,
        "ping" => $ping,
        "name" => $name
    );
}

echo json_encode(array(
    "status" => "online",
    "cvars" => $cvars,
    "players" => $players
));

// done
socket_close($sck);

