<?php

require_once 'common.php';

list($qty, $desc) = get_query_params();

$order_kdr = isset($_GET["kdr"]);

$stmt = "select 
    ud.title, 
    ud.playername, 
    st.frags,
    st.fragged,
    cast(st.frags as real) / cast(st.fragged as real) as kdr,
    pd.classnum
from game_stats st
inner join userdata as ud on ud.char_idx = st.char_idx  
inner join point_data pd on st.char_idx = pd.char_idx
";

if ($order_kdr)
    $stmt .= "order by kdr ";
else
    $stmt .= "order by st.frags ";

if ($desc) $stmt .= "desc";

$stmt .= "\n";

$stmt .= "limit :lim";

$prepared_stmt = $_db->prepare($stmt);
$prepared_stmt->execute(['lim' => $qty]);

$ret_array = array();
foreach ($prepared_stmt->fetchAll() as $row) {
    $ret_array[] = array(
        "title" => $row["title"],
        "playername" => $row["playername"],
        "frags" => $row["frags"],
        "fragged" => $row["fragged"],
        "kdr" => $row["kdr"],
        "class" => classnum_to_class($row["classnum"])
    );
}

echo json_encode($ret_array);