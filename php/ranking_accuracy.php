<?php

require_once 'common.php';

list($qty, $desc) = get_query_params();

$stmt = "select 
    ud.title, 
    ud.playername, 
    cast(st.shots as real) / cast(st.shots_hit as real) * 100.0 as accuracy,
    pd.classnum
from game_stats st
inner join userdata as ud on ud.char_idx = st.char_idx  
inner join point_data pd on st.char_idx = pd.char_idx
";

// a bit of a low pass filter
$stmt .= "where frags > 100\n";

$stmt .= "order by accuracy ";
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
        "accuracy" => $row["accuracy"],
        "class" => classnum_to_class($row["classnum"])
    );
}

echo json_encode($ret_array);
